#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fcntl.h>
#include "rutil/ResipAssert.h"
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

   // Create Environment
   int ret;
#ifdef USE_DBENV
   mEnv = new DbEnv(DB_CXX_NO_EXCEPTIONS);
   resip_assert(mEnv);
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

   bool enableTransactions = false;
   bool secondaryIndex = false;
   Data secondaryFileName;
   for (int i=0;i<MaxTable;i++)
   {
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
            fileName += "_user"; break;
         case RouteTable:
            fileName += "_route"; break;
         case AclTable:
            fileName += "_acl"; break;
         case ConfigTable:
            fileName += "_config"; break;
         case StaticRegTable:
            fileName += "_staticreg"; break;
         case FilterTable:
            fileName += "_filter"; break;
         case SiloTable:
            fileName += "_silo"; 
            enableTransactions = true;
            secondaryIndex = true;
            break;
         default:
            resip_assert(0);
      }

      if(!secondaryIndex)
      {
         fileName += ".db";
      }
      else
      {
         secondaryFileName = fileName;
         fileName += ".db";
         secondaryFileName += "_idx1.db";
      }

      mTableInfo[i].mDb = new Db(mEnv, DB_CXX_NO_EXCEPTIONS);
      resip_assert(mTableInfo[i].mDb);
      
      DebugLog( << "About to open Berkeley DB: " << fileName );
      ret = mTableInfo[i].mDb->open(0,
                         fileName.c_str(),
                         0,
                         DB_BTREE,
#ifdef USE_DBENV
                         DB_CREATE | DB_THREAD | (enableTransactions ? DB_AUTO_COMMIT : 0),
#else
                         DB_CREATE | DB_THREAD,
#endif
                         0);
      if(ret != 0)
      {
         ErrLog( <<"Could not open database " << fileName << ": " << db_strerror(ret));
         mSane = false;
         return;
      }

      // Open a cursor on the database
      ret = mTableInfo[i].mDb->cursor(0, &mTableInfo[i].mCursor, 0);
      if(ret != 0)
      {
         ErrLog( <<"Could not cursor on database " << fileName << ": " << db_strerror(ret));
         mSane = false;
         return;
      }
      resip_assert(mTableInfo[i].mCursor);

      DebugLog( << "Opened Berkeley DB: " << fileName );


      if(secondaryIndex)
      {
         mTableInfo[i].mSecondaryDb = new Db(mEnv, DB_CXX_NO_EXCEPTIONS);
         resip_assert(mTableInfo[i].mSecondaryDb);

         ret = mTableInfo[i].mSecondaryDb->set_flags(DB_DUP);
         if(ret!=0)
         {
            ErrLog( <<"Could not set database " << secondaryFileName << " to allow duplicates: " << db_strerror(ret));
            mSane = false;
            return;
         }
      
         DebugLog( << "About to open secondary Berkeley DB: " << secondaryFileName );
         ret = mTableInfo[i].mSecondaryDb->open(0,
                            secondaryFileName.c_str(),
                            0,
                            DB_BTREE,
#ifdef USE_DBENV
                            DB_CREATE | DB_THREAD | (enableTransactions ? DB_AUTO_COMMIT : 0),
#else
                            DB_CREATE | DB_THREAD,
#endif
                            0);
         if(ret != 0)
         {
            ErrLog( <<"Could not open secondary database " << secondaryFileName << ": " << db_strerror(ret));
            mSane = false;
            return;
         }

         // Associate Secondary Database with Primary
         mTableInfo[i].mSecondaryDb->set_app_private(this);  // retrievable from callback so we can have access to this BerkeleyDb instance
         ret = mTableInfo[i].mDb->associate(0, mTableInfo[i].mSecondaryDb, &getSecondaryKeyCallback, 0 /* flags */);
         if(ret != 0)
         {
            ErrLog( <<"Could not associate secondary database " << secondaryFileName << ": " << db_strerror(ret));
            mSane = false;
            return;
         }
         DebugLog( << "Opened secondary Berkeley DB: " << secondaryFileName );

         ret = mTableInfo[i].mSecondaryDb->cursor(0, &mTableInfo[i].mSecondaryCursor, 0);
         if(ret != 0)
         {
            ErrLog( <<"Could not secondary cursor on database " << secondaryFileName << ": " << db_strerror(ret));
            mSane = false;
            return;
         }
         resip_assert(mTableInfo[i].mSecondaryCursor);
      }
   }
}


BerkeleyDb::~BerkeleyDb()
{  
   for (int i=0;i<MaxTable;i++)
   {
      if(mTableInfo[i].mSecondaryCursor)
      {
         mTableInfo[i].mSecondaryCursor->close();
         mTableInfo[i].mSecondaryCursor = 0;
      }

      if(mTableInfo[i].mCursor)
      {
         mTableInfo[i].mCursor->close();
         mTableInfo[i].mCursor = 0;
      }
      
      if(mTableInfo[i].mTransaction)
      {
         dbRollbackTransaction((Table)i);
      }

      // Secondary DB should be closed before primary
      if(mTableInfo[i].mSecondaryDb)
      {
         mTableInfo[i].mSecondaryDb->close(0);
         delete mTableInfo[i].mSecondaryDb; 
         mTableInfo[i].mSecondaryDb = 0;
      }

      if(mTableInfo[i].mDb)
      {
         mTableInfo[i].mDb->close(0);
         delete mTableInfo[i].mDb; 
         mTableInfo[i].mDb = 0;
      }
   }
   if(mEnv)
   {
      mEnv->txn_checkpoint(0, 0, 0);  // Note:  a checkpoint is run when this last is created and when it is destroyed
      delete mEnv;
   }
}


int 
BerkeleyDb::getSecondaryKeyCallback(Db *db, const Dbt *pkey, const Dbt *pdata, Dbt *skey)
{
   BerkeleyDb* bdb = (BerkeleyDb*)db->get_app_private();

   // Find associated table using db pointer
   Table table = MaxTable;
   for (int i=MaxTable-1; i >= 0; i--)  // search backwards, since tables at the end have the secondary indexes
   {
      if(bdb->mTableInfo[i].mSecondaryDb == db)
      {
         table = (Table)i;
         break;
      }
   }
   resip_assert(table != MaxTable);

   Data primaryKey(Data::Share, reinterpret_cast<const char*>(pkey->get_data()), pkey->get_size());
   Data primaryData(Data::Share, reinterpret_cast<const char*>(pdata->get_data()), pdata->get_size());
   void* secondaryKey;
   unsigned int secondaryKeyLen;
   int rc = bdb->getSecondaryKey(table, primaryKey, primaryData, &secondaryKey, &secondaryKeyLen);
   skey->set_data(secondaryKey);
   skey->set_size(secondaryKeyLen);
   return rc;
}


bool
BerkeleyDb::dbWriteRecord(const Table table, 
                          const resip::Data& pKey, 
                          const resip::Data& pData )
{
   Dbt key((void*)pKey.data(), (::u_int32_t)pKey.size());
   Dbt data((void*)pData.data(), (::u_int32_t)pData.size());
   int ret;
   
   resip_assert(mTableInfo[table].mDb);
   ret = mTableInfo[table].mDb->put(mTableInfo[table].mTransaction, &key, &data, 0);

   if(ret == 0 && mTableInfo[table].mTransaction == 0)
   {
      // If we are in a transaction, then it will sync on commit
      mTableInfo[table].mDb->sync(0);
      if(mTableInfo[table].mSecondaryDb)
      {
         mTableInfo[table].mSecondaryDb->sync(0);
      }
   }
   return ret == 0;
}


bool 
BerkeleyDb::dbReadRecord(const Table table, 
                         const resip::Data& pKey, 
                         resip::Data& pData) const
{ 
   Dbt key((void*)pKey.data(), (::u_int32_t)pKey.size());
   Dbt data;
   data.set_flags(DB_DBT_MALLOC);  // required for DB_THREAD flag use

   int ret;
   
   resip_assert(mTableInfo[table].mDb);
   ret = mTableInfo[table].mDb->get(mTableInfo[table].mTransaction, &key, &data, 0);

   if (ret == DB_NOTFOUND)
   {
      // key not found 
      if (data.get_data())
      {
         free(data.get_data());
      }
      return false;
   }
   resip_assert(ret != DB_KEYEMPTY);
   resip_assert(ret == 0);
   pData.copy(reinterpret_cast<const char*>(data.get_data()), data.get_size());
   if (data.get_data())
   {
      free(data.get_data());
   }
   if(pData.empty())
   {
      // this should never happen
      return false;
   }

   return true;
}


void 
BerkeleyDb::dbEraseRecord(const Table table,
                          const resip::Data& pKey,
                          bool isSecondaryKey) // allows deleting records from a table that supports secondary keying using a secondary key
{ 
   Dbt key((void*) pKey.data(), (::u_int32_t)pKey.size());

   Db* db = mTableInfo[table].mDb;
   if(isSecondaryKey && mTableInfo[table].mSecondaryDb)
   {
      db = mTableInfo[table].mSecondaryDb;
   }
   resip_assert(db);
   db->del(mTableInfo[table].mTransaction, &key, 0);
   if(mTableInfo[table].mTransaction == 0)
   {
      // If we are in a transaction, then it will sync on commit
      mTableInfo[table].mDb->sync(0);
      if(mTableInfo[table].mSecondaryDb)
      {
         mTableInfo[table].mSecondaryDb->sync(0);
      }
   }
}


resip::Data 
BerkeleyDb::dbNextKey(const Table table, 
                      bool first)
{ 
   Dbt key, data;
   int ret;
   
   resip_assert(mTableInfo[table].mDb);
   ret = mTableInfo[table].mCursor->get(&key, &data, first ? DB_FIRST : DB_NEXT);
   if (ret == DB_NOTFOUND)
   {
      return Data::Empty;
   }
   resip_assert(ret == 0);
   
   Data d(Data::Share, reinterpret_cast<const char*>(key.get_data()), key.get_size());
   return d;
}


bool 
BerkeleyDb::dbNextRecord(const Table table,
                         const resip::Data& key,
                         resip::Data& data,
                         bool forUpdate,  // specifies to use DB_RMW flag to write lock reads
                         bool first)
{
   Dbt dbkey((void*) key.data(), (::u_int32_t)key.size());
   Dbt dbdata;
   int ret;

   resip_assert(mTableInfo[table].mSecondaryCursor);
   if(mTableInfo[table].mSecondaryCursor == 0)
   {
      // Iterating across multiple records with a common key is only 
      // supported on Seconday databases where duplicate keys exist
      return false;
   }

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

   ret = mTableInfo[table].mSecondaryCursor->get(&dbkey, &dbdata, flags);
   if (ret == DB_NOTFOUND)
   {
      return false;
   }
   resip_assert(ret == 0);
   data.copy(reinterpret_cast<const char*>(dbdata.get_data()), dbdata.get_size());

   return true;
}

bool 
BerkeleyDb::dbBeginTransaction(const Table table)
{
#ifdef USE_DBENV
   // For now - we support transactions on the primary table only
   resip_assert(mDb);
   resip_assert(mTableInfo[table].mTransaction == 0);
   int ret = mTableInfo[table].mDb->get_env()->txn_begin(0 /* parent trans*/, &mTableInfo[table].mTransaction, 0);
   if(ret != 0)
   {
      ErrLog( <<"Could not begin transaction: " << db_strerror(ret));
      return false;
   }

   // Open new Cursors - since cursors used in a transaction must be opened and closed within the transaction
   if(mTableInfo[table].mCursor)
   {
      mTableInfo[table].mCursor->close();
      mTableInfo[table].mCursor = 0;
   }

   ret = mTableInfo[table].mDb->cursor(mTableInfo[table].mTransaction, &mTableInfo[table].mCursor, 0);
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
   resip_assert(mDb);
   resip_assert(mTableInfo[table].mTransaction);

   // Close the cursor - since cursors used in a transaction must be opened and closed within the transaction
   if(mTableInfo[table].mCursor)
   {
      mTableInfo[table].mCursor->close();
      mTableInfo[table].mCursor = 0;
   }

   int ret = mTableInfo[table].mTransaction->commit(0);
   mTableInfo[table].mTransaction = 0;
   if(ret != 0)
   {
      ErrLog( <<"Could not commit transaction: " << db_strerror(ret));
      success = false;
   }

   // Reopen a cursor for general use
   mTableInfo[table].mDb->cursor(0, &mTableInfo[table].mCursor, 0);
#endif

   return success;
}

bool 
BerkeleyDb::dbRollbackTransaction(const Table table)
{
   bool success = true;
#ifdef USE_DBENV
   resip_assert(mDb);
   resip_assert(mTableInfo[table].mTransaction);

   // Close the cursor - since cursors used in a transaction must be opened and closed within the transaction
   if(mTableInfo[table].mCursor)
   {
      mTableInfo[table].mCursor->close();
      mTableInfo[table].mCursor = 0;
   }

   int ret = mTableInfo[table].mTransaction->abort();
   mTableInfo[table].mTransaction = 0;
   if(ret != 0)
   {
      success = false;
   }

   // Reopen a cursor for general use
   mTableInfo[table].mDb->cursor(0, &mTableInfo[table].mCursor, 0);
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
