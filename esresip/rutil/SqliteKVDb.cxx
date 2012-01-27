/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#include "rutil/SqliteKVDb.hxx"

#include "rutil/EsLogger.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Lock.hxx"
#include "rutil/Lockable.hxx"
#include "rutil/Mutex.hxx"

#include <assert.h>

namespace resip
{

SqliteKVDb::SqliteKVDb():
   mDbName("repro_sqlite.db"),
   mInactive(false)
{

}

SqliteKVDb::SqliteKVDb(const resip::Data& dbName):
   mDbName(dbName),
   mInactive(false)
{

}

SqliteKVDb::~SqliteKVDb()
{
   resip::WriteLock h(mTeardownMutex);
   mInactive=true;
   resip::WriteLock g(mMapMutex);

   std::map<pthread_t,DbKit*>::iterator i;
   
   for(i=mDbKits.begin();i!=mDbKits.end();i++)
   {
      delete i->second;
   }
   
   mDbKits.clear();
}

SqliteKVDb::DbKit*
SqliteKVDb::getHandle()
{
   resip::ReadLock h(mTeardownMutex);
   
   if(mInactive) return 0;
   
   pthread_t self=pthread_self();
   
   //!bwc! Relatively small critical section, that multiple readers can share.
   // Better than serializing access to a single sqlite3*.
   {
      resip::ReadLock g(mMapMutex);
      std::map<pthread_t,DbKit*>::iterator i=mDbKits.find(self);
      if(i!=mDbKits.end())
      {
         return i->second;
      }
   }

   DbKit* mKit = new DbKit;
   
   // .bwc. If we bail, this will delete mKit (and finalize everything in it)
   std::auto_ptr<DbKit> cleanupIfFail(mKit);
   
   if(!(openDbConnection(mKit) && configureDb(mKit) && setupTable(mKit)))
   {
      return 0;
   }

   setupIndex(mKit);

   if(!(setupGetKeys(mKit) && setupWrite(mKit) && setupRead(mKit) && setupErase(mKit)))
   {
      return 0;
   }

   // .bwc. Everything worked, so we don't need to delete mKit.
   cleanupIfFail.release();
   
   //.bwc. Happens very rarely with long-lifetime threads.
   {
      resip::WriteLock g(mMapMutex);
      mDbKits[self]=mKit;
   }
   
   return mKit;
   
}

void
SqliteKVDb::getKeys(std::vector<resip::Data>& container)
{
   resip::ReadLock g(mTeardownMutex);
   DbKit* mKit=getHandle();
   if(!mKit)
   {
      return;
   }
   
   container.clear();
   int res=0;

   while((res=sqlite3_step(mKit->getKeys))==SQLITE_ROW)
   {
      int bytes = sqlite3_column_bytes(mKit->getKeys,0);
      const char* blob=(const char*)sqlite3_column_blob(mKit->getKeys,0);
      container.push_back(resip::Data(blob,bytes));
   }
   
   sqlite3_reset(mKit->getKeys);
 
   // TODO instrument with logging
   switch(res)
   {
      case SQLITE_BUSY:
         ES_ERROR(estacado::SBF_PERSISTENCE,"In getKeys, Db was busy.");
         break;
      case SQLITE_DONE:
         break;
      case SQLITE_ROW:
         break;
      case SQLITE_ERROR:
         ES_ERROR(estacado::SBF_PERSISTENCE,"In getKeys, Error: " << sqlite3_errmsg(mKit->db));
         break;
      case SQLITE_MISUSE:
         ES_ERROR(estacado::SBF_PERSISTENCE,"In getKeys, Misuse: " << sqlite3_errmsg(mKit->db));
         break;
      default:
         assert(0);
   }
   
}

// Db manipulation routines
void
SqliteKVDb::dbWriteRecord( const resip::Data& key, 
                      const resip::Data& data )
{
   resip::ReadLock g(mTeardownMutex);
   DbKit* mKit=getHandle();
   if(!mKit)
   {
      return;
   }

   int res=0;

   res=sqlite3_bind_blob(mKit->write,1,key.data(),key.size(),SQLITE_TRANSIENT);

   if(res!=SQLITE_OK)
   {
      ES_ERROR(estacado::SBF_PERSISTENCE,"Failed binding blob, err:" << sqlite3_errmsg(mKit->db));
      sqlite3_reset(mKit->write);
      assert(0);
      return;
   }

   res=sqlite3_bind_blob(mKit->write,2,data.data(),data.size(),SQLITE_TRANSIENT);

   if(res!=SQLITE_OK)
   {
      ES_ERROR(estacado::SBF_PERSISTENCE,"Failed binding blob, err:" << sqlite3_errmsg(mKit->db));
      sqlite3_reset(mKit->write);
      assert(0);
      return;
   }

   res=sqlite3_step(mKit->write);
   sqlite3_reset(mKit->write);

   switch(res)
   {
      case SQLITE_BUSY:
         ES_ERROR(estacado::SBF_PERSISTENCE,"In dbWriteRecord, Db was busy.");
         break;
      case SQLITE_DONE:
         break;
      case SQLITE_ROW:
         break;
      case SQLITE_ERROR:
         ES_ERROR(estacado::SBF_PERSISTENCE,"In dbWriteRecord, Error: " << sqlite3_errmsg(mKit->db));
         break;
      case SQLITE_MISUSE:
         ES_ERROR(estacado::SBF_PERSISTENCE,"In dbWriteRecord, Misuse: " << sqlite3_errmsg(mKit->db));
         break;
      default:
         assert(0);
   }

}

bool 
SqliteKVDb::dbReadRecord( const resip::Data& key, 
                     resip::Data& data )
{
   resip::ReadLock g(mTeardownMutex);
   DbKit* mKit=getHandle();
   if(!mKit)
   {
      return false;
   }

   data.clear();

   bool success=true;
   int res=0;

   res=sqlite3_bind_blob(mKit->read,1,key.data(),key.size(),SQLITE_TRANSIENT);

   if(res!=SQLITE_OK)
   {
      ES_ERROR(estacado::SBF_PERSISTENCE,"Failed binding blob, err:" << sqlite3_errmsg(mKit->db));
      sqlite3_reset(mKit->read);
      assert(0);
      return false;
   }


   res=sqlite3_step(mKit->read);

   switch(res)
   {
      case SQLITE_BUSY:
         ES_ERROR(estacado::SBF_PERSISTENCE,"In dbReadRecord, Db was busy.");
         success= false;//Maybe retry a couple of times?
         break;
      case SQLITE_DONE:
         ES_DEBUG(estacado::SBF_PERSISTENCE,"In dbReadRecord, No match found.");
         success= false; 
         break;
      case SQLITE_ROW:
         break;
      case SQLITE_ERROR:
         ES_ERROR(estacado::SBF_PERSISTENCE,"In dbReadRecord, Error: " << sqlite3_errmsg(mKit->db));
         success= false; 
         break;
      case SQLITE_MISUSE:
         ES_ERROR(estacado::SBF_PERSISTENCE,"In dbReadRecord, Misuse: " << sqlite3_errmsg(mKit->db));
         success= false; 
         break;
      default:
         assert(0);
         success= false; 
   }

   if(success)
   {
      ES_DEBUG(estacado::SBF_PERSISTENCE,"Found a result.");
      assert(sqlite3_column_count(mKit->read)==1);
      int bytes = sqlite3_column_bytes(mKit->read,0);
      const char* blob=(const char*)sqlite3_column_blob(mKit->read,0);
      if ( blob )
      {
          data.append(blob,bytes);
      }
   }

   sqlite3_reset(mKit->read);


   return success;
}

void
SqliteKVDb::dbEraseRecord( const resip::Data& key )
{
   resip::ReadLock g(mTeardownMutex);
   DbKit* mKit=getHandle();
   if(!mKit)
   {
      return;
   }

   int res=0;

   res=sqlite3_bind_blob(mKit->erase,1,key.data(),key.size(),SQLITE_TRANSIENT);

   if(res!=SQLITE_OK)
   {
      ES_ERROR(estacado::SBF_PERSISTENCE,"Failed binding blob, err:" << sqlite3_errmsg(mKit->db));
      sqlite3_reset(mKit->erase);
      assert(0);
      return;
   }

   res=sqlite3_step(mKit->erase);
   sqlite3_reset(mKit->erase);

   switch(res)
   {
      case SQLITE_BUSY:
         ES_ERROR(estacado::SBF_PERSISTENCE,"In dbEraseRecord, Db was busy.");         
         break;
      case SQLITE_DONE:
         ES_DEBUG(estacado::SBF_PERSISTENCE,"In dbEraseRecord, No match found.");
         break;
      case SQLITE_ROW:
         break;
      case SQLITE_ERROR:
         ES_ERROR(estacado::SBF_PERSISTENCE,"In dbEraseRecord, Error: " << sqlite3_errmsg(mKit->db));
         break;
      case SQLITE_MISUSE:
         ES_ERROR(estacado::SBF_PERSISTENCE,"In dbEraseRecord, Misuse: " << sqlite3_errmsg(mKit->db));
         break;
      default:
         assert(0);
   }


}

bool 
SqliteKVDb::openDbConnection(DbKit* mKit)
{
   int res=0;
   res = sqlite3_open(mDbName.c_str(),&mKit->db);
   if(res!=SQLITE_OK)
   {
      ES_ERROR(estacado::SBF_PERSISTENCE,"Could not open Sqlite db file "
                     << mDbName << " err: " <<sqlite3_errmsg(mKit->db));
      return false;
   }
   return true;
}

bool 
SqliteKVDb::configureDb(DbKit* mKit)
{
   int res=0;
   res= sqlite3_busy_timeout(mKit->db,10);   
   if(res!=SQLITE_OK)
   {
      ES_ERROR(estacado::SBF_PERSISTENCE,"Could not set busy timeout "
                     << mDbName << " err: " <<sqlite3_errmsg(mKit->db));
      return false;
   }

   resip::Data pragmaSyncOff("PRAGMA synchronous = OFF");
   resip::Data pragmaTempStoreMem("PRAGMA temp_store = MEMORY");
   char* err;
   sqlite3_exec(mKit->db,pragmaSyncOff.c_str(),NULL,NULL,&err);
   if(err!=NULL)
   {
      sqlite3_free(err);
      err=0;
   }
   
   sqlite3_exec(mKit->db,pragmaTempStoreMem.c_str(),NULL,NULL,&err);
   if(err!=NULL)
   {
      sqlite3_free(err);
      err=0;
   }
   return true;
}

bool
SqliteKVDb::setupTable(DbKit* mKit)
{
   char* err;
   int res=0;

   resip::Data checkTable("SELECT type FROM sqlite_master WHERE (name = \"version1\") AND (type=\"table\")");
   sqlite3_stmt* checkTableStmt;
   const char* excess;
   
   res = sqlite3_prepare(mKit->db,checkTable.c_str(),checkTable.size(),&checkTableStmt,&excess);
   
   if(res!=SQLITE_OK)
   {
      ES_ERROR(estacado::SBF_PERSISTENCE,"Could not compile statement for "
                     << "existence check for table version1. Bailing.");
      assert(0);
      return false;
   }
   
   res = sqlite3_step(checkTableStmt);
   
   if(res==SQLITE_DONE)
   {
      resip::Data makeTable("CREATE TABLE version1(Key BLOB NOT NULL PRIMARY KEY UNIQUE ON CONFLICT REPLACE, Value BLOB )");

      res = sqlite3_exec(mKit->db,makeTable.c_str(),NULL,NULL,&err);

      if(res!=SQLITE_OK)
      {
         ES_ERROR(estacado::SBF_PERSISTENCE,"Could not create table "
                     << mDbName << " err: " <<sqlite3_errmsg(mKit->db));
         if(err!=NULL)
         {
            sqlite3_free(err);
            err=0;
         }
         sqlite3_finalize(checkTableStmt);
         return false;
      }
   }
   else if(res!=SQLITE_ROW)
   {
      ES_ERROR(estacado::SBF_PERSISTENCE,"Error checking for table! err:"
                     << sqlite3_errmsg(mKit->db));
      sqlite3_finalize(checkTableStmt);
      return false;
   }
   
   sqlite3_finalize(checkTableStmt);
   return true;
}

void
SqliteKVDb::setupIndex(DbKit* mKit)
{
   char* err;
   int res=0;

   resip::Data indexTable("CREATE UNIQUE INDEX index_key ON version1(Key)");   
   res=sqlite3_exec(mKit->db,indexTable.c_str(),NULL,NULL,&err);
   
   if(res!=SQLITE_OK)
   {
      ES_ERROR(estacado::SBF_PERSISTENCE,"Could not create key index on table "
                  << mDbName << " err: " <<sqlite3_errmsg(mKit->db)
                  << ": This may be because is already exists.");   
      if(err!=NULL)
      {
         sqlite3_free(err);
         err=0;
      }
   }

}

bool 
SqliteKVDb::setupGetKeys(DbKit* mKit)
{
   const char* tail;
   int res=0;

   const static char* getKeysCommand="SELECT Key FROM version1 ";
   const static int getKeysCommandlen=strlen(getKeysCommand);
   res=sqlite3_prepare(mKit->db,getKeysCommand,getKeysCommandlen,&mKit->getKeys,&tail);

   if(res!=SQLITE_OK)
   {
      ES_ERROR(estacado::SBF_PERSISTENCE,"Failed compiling sql, err: " << sqlite3_errmsg(mKit->db) );
      return false;
   }
   return true;
}

bool 
SqliteKVDb::setupWrite(DbKit* mKit)
{
   const char* tail;
   int res=0;

   const static char* writeCommand="REPLACE INTO version1 (Key, Value) VALUES ( :key , :val )";
   const static int writeCommandlen=strlen(writeCommand);
   res=sqlite3_prepare(mKit->db,writeCommand,writeCommandlen,&mKit->write,&tail);

   if(res!=SQLITE_OK)
   {
      ES_ERROR(estacado::SBF_PERSISTENCE,"Failed compiling sql, err: " << sqlite3_errmsg(mKit->db) );
      return false;
   }
   return true;
}

bool 
SqliteKVDb::setupRead(DbKit* mKit)
{
   const char* tail;
   int res=0;

   const static char* readCommand="SELECT Value FROM version1 WHERE Key = :key ";
   const static int readCommandlen=strlen(readCommand);
   res=sqlite3_prepare(mKit->db,readCommand,readCommandlen,&mKit->read,&tail);

   if(res!=SQLITE_OK)
   {
      ES_ERROR(estacado::SBF_PERSISTENCE,"Failed compiling sql, err: " << sqlite3_errmsg(mKit->db) );
      return false;
   }
   return true;
}

bool 
SqliteKVDb::setupErase(DbKit* mKit)
{
   const char* tail;
   int res=0;

   const static char* eraseCommand="DELETE FROM version1 WHERE Key = :key ";
   const static int eraseCommandlen=strlen(eraseCommand);
   res=sqlite3_prepare(mKit->db,eraseCommand,eraseCommandlen,&mKit->erase,&tail);

   if(res!=SQLITE_OK)
   {
      ES_ERROR(estacado::SBF_PERSISTENCE,"Failed compiling sql, err: " << sqlite3_errmsg(mKit->db) );
      return false;
   }
   return true;
}


SqliteKVDb::DbKit::~DbKit()
{
   if(getKeys) sqlite3_finalize(getKeys);
   if(read) sqlite3_finalize(read);
   if(write) sqlite3_finalize(write);
   if(erase) sqlite3_finalize(erase);
   if(db) sqlite3_close(db);
}

}

/* Copyright 2007 Estacado Systems */
