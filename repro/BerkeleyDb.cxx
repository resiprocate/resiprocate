#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fcntl.h>
#include <cassert>
#include <cstdlib>

#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"

#include "repro/AbstractDb.hxx"
#include "repro/BerkeleyDb.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO
//#define USE_DBENV   // Required for transaction support

BerkeleyDb::BerkeleyDb()
{
   init(Data::Empty, Data::Empty);
}


BerkeleyDb::BerkeleyDb( const Data& dbPath, const Data& dbName )
{
   init(dbPath, dbName);
}


void
BerkeleyDb::init( const Data& dbPath, const Data& dbName )
{ 
   Data filePath(dbPath);

   // An empty path is how you specify the current working directory as a path
   if ( !filePath.empty() )
   {
#ifdef WIN32
      filePath += '\\';
#else
      filePath += '/';
#endif
   }

   if ( dbName.empty() )
   {
      DebugLog( << "No BerkeleyDb prefix specified - using default" );
      filePath += "repro";
   }
   else
   {
      filePath += dbName;
   }

   InfoLog( << "Using BerkeleyDb prefixed with " << filePath );

   mSane = true;

   // Zeroize handle arrays
   for (int i=0;i<MaxTable;i++)
   {
      mDb[i] = 0;
      mCursor[i] = 0;
      mTransaction[i] = 0;
   }

   // Create Environment
   int ret;
#ifdef USE_DBENV
   mEnv = new DbEnv(DB_CXX_NO_EXCEPTIONS);
   assert(mEnv);
   ret = mEnv->open(0, DB_CREATE |     // If the env does not exist, then create it
                       DB_INIT_LOCK |  // Initialize Locking (needed for transactions)
                       DB_INIT_LOG |   // Initialize Logging (needed for transactions)
                       DB_INIT_MPOOL | // Initialize the cache (needed for transactions)
                       DB_INIT_TXN |   // Initialize transactions
                       DB_RECOVER |    // Run normal recovery
                       DB_THREAD,      // Free-thread the env handle
                       0 /* mode */);
   if(ret != 0)
   {
      ErrLog( <<"Could not open environment: " << db_strerror(ret));
      mSane = false;
      return;
   }
   mEnv->txn_checkpoint(0, 0, 0);  // Note:  a checkpoint is run when this last is created and when it is destroyed
#else
   mEnv = 0;
#endif

   bool allowDuplicates = false;
   bool enableTransactions = false;
   for (int i=0;i<MaxTable;i++)
   {
      allowDuplicates = false;
      enableTransactions = false;
      // if the line bellow seems wrong, you need to check which version 
      // of db you have - it is likely an very out of date version 
      // still trying to figure this out so email fluffy if you have 
      // problems and include your version the DB_VERSION_STRING found 
      // in your db4/db.h file. 
      Data fileName( filePath );
      switch (i)
      {
         case UserTable:
            fileName += "_user.db"; break;
         case RouteTable:
            fileName += "_route.db"; break;
         case AclTable:
            fileName += "_acl.db"; break;
         case ConfigTable:
            fileName += "_config.db"; break;
         case StaticRegTable:
            fileName += "_staticreg.db"; break;
         case FilterTable:
            fileName += "_filter.db"; break;
         case SiloTable:
            fileName += "_silo.db"; 
            allowDuplicates = true;
            enableTransactions = true;
            break;
         default:
            assert(0);
      }

      mDb[i] = new Db(mEnv, DB_CXX_NO_EXCEPTIONS);
      assert(mDb[i]);

      if(allowDuplicates)
      {
         ret = mDb[i]->set_flags(DB_DUP);
         if(ret!=0)
         {
            ErrLog( <<"Could not set database " << fileName << " to allow duplicates: " << db_strerror(ret));
            mSane = false;
            return;
         }
      }
      
      DebugLog( << "About to open Berkeley DB: " << fileName );
      ret = mDb[i]->open(0,
                         fileName.c_str(),
                         0,
                         DB_BTREE,
#ifdef USE_DBENV
                         DB_CREATE | DB_THREAD | (enableTransactions ? DB_AUTO_COMMIT : 0),
#else
                         DB_CREATE | DB_THREAD,
#endif
                         0);
      if(ret!=0)
      {
         ErrLog( <<"Could not open database " << fileName << ": " << db_strerror(ret));
         mSane = false;
         return;
      }
      DebugLog( << "Opened Berkeley DB: " << fileName );
      
      mDb[i]->cursor(0, &mCursor[i], 0);
      assert(mCursor);
   }
}


BerkeleyDb::~BerkeleyDb()
{  
   for (int i=0;i<MaxTable;i++)
   {
      if(mCursor[i])
      {
         mCursor[i]->close();
         mCursor[i] = 0;
      }
      
      if(mTransaction[i])
      {
         dbRollbackTransaction((Table)i);
      }

      if(mDb[i])
      {
         mDb[i]->close(0);
         delete mDb[i]; 
         mDb[i] = 0;
      }
   }
   if(mEnv)
   {
      mEnv->txn_checkpoint(0, 0, 0);  // Note:  a checkpoint is run when this last is created and when it is destroyed
      delete mEnv;
   }
}


bool
BerkeleyDb::dbWriteRecord( const Table table, 
                          const resip::Data& pKey, 
                          const resip::Data& pData )
{
   Dbt key((void*)pKey.data(), (::u_int32_t)pKey.size());
   Dbt data((void*)pData.data(), (::u_int32_t)pData.size());
   int ret;
   
   assert(mDb);
   ret = mDb[table]->put(mTransaction[table], &key, &data, 0);

   if(ret == 0 && mTransaction[table] == 0)
   {
      // If we are in a transaction, then it will sync on commit
      mDb[table]->sync(0);
   }
   return ret == 0;
}


bool 
BerkeleyDb::dbReadRecord( const Table table, 
                         const resip::Data& pKey, 
                         resip::Data& pData ) const
{ 
   Dbt key((void*)pKey.data(), (::u_int32_t)pKey.size());
   Dbt data;
   data.set_flags(DB_DBT_MALLOC);

   int ret;
   
   assert(mDb);
   ret = mDb[table]->get(mTransaction[table], &key, &data, 0);

   if (ret == DB_NOTFOUND)
   {
      // key not found 
      if (data.get_data())
      {
         free(data.get_data());
      }
      return false;
   }
   assert(ret != DB_KEYEMPTY);
   assert(ret == 0);
   Data result(reinterpret_cast<const char*>(data.get_data()), data.get_size());
   if (data.get_data())
   {
      free(data.get_data());
   }
   if(result.empty())
   {
      // this should never happen
      return false;
   }

   pData = result;
   
   return true;
}


void 
BerkeleyDb::dbEraseRecord(const Table table,
                          const resip::Data& pKey)
{ 
   Dbt key((void*) pKey.data(), (::u_int32_t)pKey.size());

   assert(mDb);
   mDb[table]->del(mTransaction[table], &key, 0);
   if(mTransaction[table] == 0)
   {
      // If we are in a transaction, then it will sync on commit
      mDb[table]->sync(0);
   }
}


resip::Data 
BerkeleyDb::dbNextKey(const Table table, 
                      bool first)
{ 
   Dbt key,data;
   int ret;
   
   assert(mDb);
   ret = mCursor[table]->get(&key, &data, first ? DB_FIRST : DB_NEXT);
   if (ret == DB_NOTFOUND)
   {
      return Data::Empty;
   }
   assert(ret == 0);
   
   Data d(reinterpret_cast<const char*>(key.get_data()), key.get_size());
   
   return d;
}


bool 
BerkeleyDb::dbNextRecord(const Table table,
                         const resip::Data& key,
                         resip::Data& data,
                         bool forUpdate,  // specifies to use DB_RMW flag to write lock reads
                         bool first)
{
   Dbt dbkey( (void*) key.data(), (::u_int32_t)key.size() );
   Dbt dbdata;
   int ret;
   
   assert(mDb);
   unsigned int flags = 0;
   if(key.empty())
   {
      flags = first ? DB_FIRST : DB_NEXT;
   }
   else
   {
      flags = first ? DB_SET : DB_NEXT_DUP;
   }

#ifdef USE_DBENV
   if(forUpdate)
   {
      flags |= DB_RMW;
   }
#endif

   ret = mCursor[table]->get(&dbkey, &dbdata, flags);
   if (ret == DB_NOTFOUND)
   {
      return false;
   }
   assert(ret == 0);
   data.copy(reinterpret_cast<const char*>(dbdata.get_data()), dbdata.get_size());

   return true;
}

bool 
BerkeleyDb::dbBeginTransaction(const Table table)
{
#ifdef USE_DBENV
   assert(mDb);
   assert(mTransaction[table] == 0);
   int ret = mDb[table]->get_env()->txn_begin(0 /* parent trans*/, &mTransaction[table], 0);
   if(ret != 0)
   {
      ErrLog( <<"Could not begin transaction: " << db_strerror(ret));
      return false;
   }

   // Open new Cursor - since cursors used in a transaction must be opened and closed within the transation
   if(mCursor[table])
   {
      mCursor[table]->close();
      mCursor[table] = 0;
   }
   ret = mDb[table]->cursor(mTransaction[table], &mCursor[table], 0);
   if(ret != 0)
   {
      ErrLog( <<"Could not open cursor for transaction: " << db_strerror(ret));
   }
#endif

   return true;
}

bool 
BerkeleyDb::dbCommitTransaction(const Table table)
{
   bool success = true;
#ifdef USE_DBENV
   assert(mDb);
   assert(mTransaction[table]);

   // Close the cursor - since cursors used in a transaction must be opened and closed within the transation
   if(mCursor[table])
   {
      mCursor[table]->close();
      mCursor[table] = 0;
   }

   int ret = mTransaction[table]->commit(0);
   mTransaction[table] = 0;
   if(ret != 0)
   {
      ErrLog( <<"Could not commit transaction: " << db_strerror(ret));
      success = false;
   }

   // Reopen a cursor for general use
   mDb[table]->cursor(0, &mCursor[table], 0);
#endif

   return success;
}

bool 
BerkeleyDb::dbRollbackTransaction(const Table table)
{
   bool success = true;
#ifdef USE_DBENV
   assert(mDb);
   assert(mTransaction[table]);

   // Close the cursor - since cursors used in a transaction must be opened and closed within the transation
   if(mCursor[table])
   {
      mCursor[table]->close();
      mCursor[table] = 0;
   }

   int ret = mTransaction[table]->abort();
   mTransaction[table] = 0;
   if(ret != 0)
   {
      success = false;
   }

   // Reopen a cursor for general use
   mDb[table]->cursor(0, &mCursor[table], 0);
#endif

   return success;
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 */
