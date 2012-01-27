/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#ifndef SQLITE_KV_DB_HXX
#define SQLITE_KV_DB_HXX 1

#include "rutil/KeyValueDbIf.hxx"
#include <vector>
#include <pthread.h>
#include <map>
#include "rutil/Data.hxx"
#include "rutil/Lock.hxx"
#include "rutil/Lockable.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/RWMutex.hxx"

extern "C"
{
#include <sqlite3.h>
}

namespace resip
{
/**
   @brief Implementation of KeyValueDbIf that uses sqlite.

   @note This class is threadsafe for pthreads. (ie. anything that will
   return a unique value when pthread_self() is called)
   @ingroup database_related
   @internal
**/
class SqliteKVDb : public KeyValueDbIf
{
   public:
      SqliteKVDb(); 
      SqliteKVDb(const resip::Data& dbName); 
      virtual ~SqliteKVDb();
      
      virtual void getKeys(std::vector<resip::Data>& container);
      // Db manipulation routines
      virtual void dbWriteRecord( const resip::Data& key, 
                                  const resip::Data& data );
      virtual bool dbReadRecord( const resip::Data& key, 
                                 resip::Data& data ) ; // return false if not found
      virtual void dbEraseRecord( const resip::Data& key );

   private:
      
      class DbKit
      {
         public:
            DbKit() : db(0), getKeys(0), write(0), read(0), erase(0){}
            ~DbKit();
            sqlite3* db;
            sqlite3_stmt* getKeys;
            sqlite3_stmt* write;
            sqlite3_stmt* read;
            sqlite3_stmt* erase;
      };
      
      DbKit* getHandle();
      bool openDbConnection(DbKit* kit);
      bool configureDb(DbKit* kit);
      bool setupTable(DbKit* kit);
      void setupIndex(DbKit* kit);
      bool setupGetKeys(DbKit* kit);
      bool setupWrite(DbKit* kit);
      bool setupRead(DbKit* kit);
      bool setupErase(DbKit* kit);
      resip::Data mDbName;
      resip::RWMutex mMapMutex;
      resip::RWMutex mTeardownMutex;
      std::map<pthread_t,DbKit*> mDbKits;
      bool mInactive;


};

}

#endif

/* Copyright 2007 Estacado Systems */
