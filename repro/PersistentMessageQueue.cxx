#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//#include <fcntl.h>
//#include "rutil/ResipAssert.h"
//#include <cstdlib>

#include "rutil/Data.hxx"
#include "rutil/FileSystem.hxx"
#include "rutil/Logger.hxx"

#include "repro/PersistentMessageQueue.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

#ifndef DISABLE_BERKELEYDB_USE

struct Transaction 
{     
   Transaction() : mDbTxn(0) 
   { 
   }

   void init(DbEnv * dbenv)
   {
      // let caller catch exceptions
      dbenv->txn_begin(NULL, &mDbTxn, 0);
   }
   
   ~Transaction()
   {
      abort();
   }

   void abort()
   {
      // don't throw exceptions since this is called in destructor
      if(mDbTxn !=0 )
      {
         try
         {
            mDbTxn->abort();
         }
         catch(DbException&) {}
         catch(std::exception&) {}
         catch(...) {}

         mDbTxn = 0;
      }
   }
   
   void commit()
   {
      // let caller catch exceptions
      if(mDbTxn !=0 )
      {
         mDbTxn->commit(0);
         mDbTxn = 0;
      }
   }

   DbTxn* mDbTxn;
};  

struct Cursor
{
   Cursor() : mDbc(0)
   { 
   }

   void init(Db* db, DbTxn* t)
   {
      // let caller catch exceptions
      db->cursor(t, &mDbc, 0);
   }

   ~Cursor()
   {
      close();
   }
   
   void close()
   { 
      // don't throw exceptions since this is called in destructor
      if(mDbc)
      {
         try
         {
            mDbc->close();
         }
         catch(DbException&) {}
         catch(std::exception&) {}
         catch(...) {}

         mDbc = 0;
      }
   }
   
   Dbc* mDbc;
};
#endif

PersistentMessageQueue::PersistentMessageQueue(const Data& baseDir) : 
#ifndef DISABLE_BERKELEYDB_USE
   DbEnv(0), 
   mDb(0), 
#endif
   mBaseDir(baseDir),
   mRecoveryNeeded(false)
{
}

PersistentMessageQueue::~PersistentMessageQueue()
{
#ifndef DISABLE_BERKELEYDB_USE
   if(mDb)
   {
      try
      {
         mDb->close(0);
      }
      catch(DbException&) {}
      catch(std::exception&) {}
      catch(...) {}
   }

   try
   {
      delete mDb;
      close(0);
   }
   catch(DbException&) {}
   catch(std::exception&) {}
   catch(...) {}
#endif
}

bool 
PersistentMessageQueue::init(bool sync, const resip::Data& queueName)
{
#ifndef DISABLE_BERKELEYDB_USE
   try
   {
      // For Berkeley DB Concurrent Data Store applications, perform locking on an environment-wide basis rather than per-database.
      set_flags(DB_CDB_ALLDB, 1);

      if(sync)
      {
         // Write changes to disk on transaction commit
         set_flags(DB_TXN_NOSYNC, 0);
      }
      else
      {
         set_flags(DB_TXN_NOSYNC, 1);
      }

      Data homeDir;
      if (mBaseDir.postfix("/") || 
          mBaseDir.postfix("\\") ||
          mBaseDir.empty())
      {
         homeDir = mBaseDir + queueName;
      }
      else
      {
         homeDir = mBaseDir + Data("/") + queueName;
      }

      // Create directory if it doesn't exist
      FileSystem::Directory dir(homeDir);
      dir.create();
      
      open(homeDir.c_str(),   // home directory 
           DB_INIT_LOCK        // Initialize Locking (needed for transactions)
           | DB_INIT_LOG       // Initialize Logging (needed for transactions)
           | DB_INIT_TXN       // Initialize transactions
           | DB_INIT_MPOOL     // Initialize the cache (needed for transactions)
           | DB_REGISTER       // Check to see if recovery needs to be performed before opening the database environment
           | DB_RECOVER        // Run normal recovery
           | DB_CREATE         // If the env does not exist, then create it
           | DB_THREAD,        // Free-thread the env handle
           0);         // mode

      mDb = new Db(this, 0);

      // cause the logical record numbers to be mutable, and change as records are added to and deleted from the database.
      mDb->set_flags(DB_RENUMBER);  

      mDb->open(0,          // txnid
                "msgqueue", // filename
                0,          // database name
                DB_RECNO,   // database type
                DB_AUTO_COMMIT  // enclose db->open within a transaction
                | DB_CREATE     // create the database if it doesn't already exist
                | DB_THREAD,    // allow free-threaded db access
                0);         // mode

      return true;
   }
   catch(DbException& e)
   {
      if(e.get_errno() == DB_RUNRECOVERY)
      {
         mRecoveryNeeded = true;
      }
      WarningLog( << "PersistentMessageQueue::init - DBException: " << e.what());
   } 
   catch(std::exception& e)
   {
      WarningLog( << "PersistentMessageQueue::init - std::exception: " << e.what());
   } 
   catch(...) 
   {
      WarningLog( << "PersistentMessageQueue::init - unknown exception");
   }
#endif
   return false;
}

bool 
PersistentMessageQueue::isRecoveryNeeded()
{
   return mRecoveryNeeded;
}

bool 
PersistentMessageEnqueue::push(const resip::Data& data)
{
#ifndef DISABLE_BERKELEYDB_USE
   int res;

   try 
   {
      Transaction transaction;
      transaction.init(this);

      db_recno_t recno; 
      recno = 0;
      Dbt val((void*)data.c_str(), data.size());
      Dbt key((void*)&recno, sizeof(recno));

      key.set_ulen(sizeof(recno));
      key.set_flags(DB_DBT_USERMEM);

      res = mDb->put(transaction.mDbTxn, &key, &val, DB_APPEND);
      if(res == 0)
      {
         transaction.commit();
         return true;
      } 
      else
      {
         WarningLog( << "PersistentMessageEnqueue::push - put failed: " << db_strerror(res));
         return false;
      }
   } 
   catch(DbException& e)
   {
      if(e.get_errno() == DB_RUNRECOVERY)
      {
         mRecoveryNeeded = true;
      }
      WarningLog( << "PersistentMessageEnqueue::push - DBException: " << e.what());
   } 
   catch(std::exception& e)
   {
      WarningLog( << "PersistentMessageEnqueue::push - std::exception: " << e.what());
   } 
   catch(...) 
   {
      WarningLog( << "PersistentMessageEnqueue::push - unknown exception");
   }
#endif
   return false;
}

// returns true for success, false for failure - can return true and 0 records if none available
// Note:  if autoCommit is used then it is safe to allow multiple consumers
bool 
PersistentMessageDequeue::pop(size_t numRecords, std::vector<resip::Data>& records, bool autoCommit)  
{
#ifndef DISABLE_BERKELEYDB_USE
   if(mNumRecords != 0) // TODO, warning previous pop wasn't committed
   {
      abort();
   }
   records.clear();

   // Get a cursor and Read
   try 
   {
      Transaction transaction;  // Only used with auto-commit
      Cursor cursor;
      if(autoCommit)
      {
         transaction.init(this);
      }
      cursor.init(mDb, transaction.mDbTxn);  // If autoCommit is false then mDbTxn is 0

      Dbt val;
      db_recno_t recno = 0;
      Dbt key((void*)&recno, sizeof(recno));

      for(size_t i = 0; i < numRecords; i ++ )
      {
         int get_res = cursor.mDbc->get(&key, &val, DB_NEXT | (autoCommit ?  DB_RMW : 0));

         if(get_res == 0)
         {
            records.push_back(Data((const char *)val.get_data(), val.get_size()));
            if(autoCommit)
            {
               cursor.mDbc->del(0);
            }
         }
         else
         {
            break;
         }
      }

      cursor.close();
      if(autoCommit)
      {
         transaction.commit();
      }
      else
      {
         mNumRecords = records.size();
      }
      return true;
   } 
   catch(DbException& e)
   {
      if(e.get_errno() == DB_RUNRECOVERY)
      {
         mRecoveryNeeded = true;
      }
      WarningLog( << "PersistentMessageDequeue::pop - DBException: " << e.what());
      abort();
   } 
   catch(std::exception& e)
   {
      WarningLog( << "PersistentMessageDequeue::pop - std::exception: " << e.what());
      abort();
   }
   catch(...) 
   {
      WarningLog( << "PersistentMessageDequeue::pop - unknown exception");
      return false;
   }
#endif
   return false;
}

bool 
PersistentMessageDequeue::commit()
{
#ifndef DISABLE_BERKELEYDB_USE
   if(mNumRecords == 0)
   {
      return true;
   }
      
   // Read and delete
   try
   {
      Transaction transaction;
      Cursor cursor;

      transaction.init(this);
      cursor.init(mDb, transaction.mDbTxn);

      Dbt val;
      db_recno_t recno = 0;
      Dbt key((void*)&recno, sizeof(recno));
         
      for(size_t i = 0; i < mNumRecords; i++)
      {
         int get_res = cursor.mDbc->get(&key, &val, DB_NEXT | DB_RMW);
         if(get_res == 0)
         {
            cursor.mDbc->del(0);
         }
         else
         {
            break; // this is bad!
         }
      }

      mNumRecords = 0;
      cursor.close();
      transaction.commit();

      return true;
   } 
   catch(DbException& e)
   {
      if(e.get_errno() == DB_RUNRECOVERY)
      {
         mRecoveryNeeded = true;
      }
      WarningLog( << "PersistentMessageDequeue::commit - DBException: " << e.what());
   } 
   catch(std::exception& e)
   {
      WarningLog( << "PersistentMessageDequeue::commit - std::exception: " << e.what());
   }
   catch(...) 
   {
      WarningLog( << "PersistentMessageDequeue::commit - unknown exception");
   }
#endif
   return false;
}

void 
PersistentMessageDequeue::abort()
{
   mNumRecords = 0;
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
