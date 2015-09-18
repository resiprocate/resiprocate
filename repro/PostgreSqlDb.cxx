#include <cassert>
#include <fcntl.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_POSTGRESQL

#include "rutil/ResipAssert.h"
#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"

#include "repro/AbstractDb.hxx"
#include "repro/PostgreSqlDb.hxx"


using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

extern "C"
{
   void postgresqlThreadEnd(void*)
   {
      //mysql_thread_end(); // FIXME - PostgreSQL equivalent?
   }
}

// This class helps ensure that each thread using the PostgreSQL API's 
// initialize by calling mysql_thread_init before calling any mySQL functions
class PostgreSQLInitializer
{
   public:
      PostgreSQLInitializer()
      {
         ThreadIf::tlsKeyCreate(mThreadStorage, postgresqlThreadEnd);
      }
      ~PostgreSQLInitializer()
      {
         ThreadIf::tlsKeyDelete(mThreadStorage);
      }
      void setInitialized()
      {
         ThreadIf::tlsSetValue(mThreadStorage, (void*) true);
      }
      bool isInitialized()
      {
         // Note:  if value is not set yet then 0 (false) is returned
         return ThreadIf::tlsGetValue(mThreadStorage) != 0; 
      }

   private:
      ThreadIf::TlsKey mThreadStorage;
};
static PostgreSQLInitializer g_PostgreSQLInitializer;

PostgreSqlDb::PostgreSqlDb(const Data& connInfo,
                 const Data& server,
                 const Data& user, 
                 const Data& password, 
                 const Data& databaseName, 
                 unsigned int port, 
                 const Data& customUserAuthQuery) :
   mDBConnInfo(connInfo),
   mDBServer(server),
   mDBUser(user),
   mDBPassword(password),
   mDBName(databaseName),
   mDBPort(port),
   mCustomUserAuthQuery(customUserAuthQuery),
   mConn(0)
{ 
   InfoLog( << "Using PostgreSQL DB with server=" << server << ", user=" << user << ", dbName=" << databaseName << ", port=" << port);

   for (int i=0;i<MaxTable;i++)
   {
      mResult[i]=0;
      mRow[i]=0;
   }

   if(!PQisthreadsafe())
   {
      ErrLog( << "Repro uses PostgreSQL from multiple threads - you MUST link with a thread safe version of the PostgreSQL client library (libpq)!");
   }
   else
   {
      connectToDatabase();
   }
}


PostgreSqlDb::~PostgreSqlDb()
{
   disconnectFromDatabase();
}

void
PostgreSqlDb::initialize() const
{
   if(!g_PostgreSQLInitializer.isInitialized())
   {
      g_PostgreSQLInitializer.setInitialized();
      //mysql_thread_init();   // FIXME - PostgreSQL equivalent?
   }
}

void
PostgreSqlDb::disconnectFromDatabase() const
{
   if(mConn)
   {
      for (int i=0;i<MaxTable;i++)
      {
         if (mResult[i])
         {  
            PQclear(mResult[i]); 
            mResult[i]=0;
            mRow[i]=0;
         }
      }
   
      PQfinish(mConn);
      mConn = 0;
      setConnected(false);
   }
}

int 
PostgreSqlDb::connectToDatabase() const
{
   // Disconnect from database first (if required)
   disconnectFromDatabase();

   // Now try to connect
   resip_assert(mConn == 0);
   resip_assert(isConnected() == false);

   Data connInfo(mDBConnInfo);
   if(!mDBServer.empty())
   {
      connInfo = connInfo + " host=" + mDBServer;
   }
   if(mDBPort > 0)
   {
      connInfo = connInfo + " port=" + Data((UInt32)mDBPort);
   }
   if(!mDBName.empty())
   {
      connInfo = connInfo + " dbname=" + mDBName;
   }
   if(!mDBUser.empty())
   {
      connInfo = connInfo + " user=" + mDBUser;
   }
   Data connInfoLogString = connInfo;
   if(!mDBPassword.empty())
   {
      connInfo = connInfo + " password=" + mDBPassword;
      connInfoLogString = connInfoLogString + " password=<hidden>";
   }

   DebugLog(<<"Trying to connect to PostgreSQL server with conninfo string: " << connInfoLogString);
   mConn = PQconnectdb(connInfo.c_str());

   int rc = PQstatus(mConn);
   if (rc != CONNECTION_OK)
   { 
      ErrLog( << "PostgreSQL connect failed: " << PQerrorMessage(mConn));
      mConn = 0;
      setConnected(false);
      return -1;
   }
   else
   {
      setConnected(true);
      return 0;
   }
}

inline int pqOK(const PGresult *result)
{
   ExecStatusType t = PQresultStatus(result);
   return (t == PGRES_COMMAND_OK || t == PGRES_TUPLES_OK ? 0 : 1);
}

int
PostgreSqlDb::query(const Data& queryCommand, PGresult** result) const
{
   int rc = 0;
   PGresult *_result;

   initialize();

   DebugLog( << "PostgreSqlDb::query: executing query: " << queryCommand);

   Lock lock(mMutex);
   if(mConn == 0 || !isConnected())
   {
      rc = connectToDatabase();
   }
   if(rc == 0)
   {
      resip_assert(mConn!=0);
      resip_assert(isConnected());
      _result = PQexec(mConn, queryCommand.c_str());
      rc = pqOK(_result);
      if(rc != 0)
      {
         PQclear(_result);
         if(rc == PGRES_FATAL_ERROR)
         {
            // First failure may be a connection error - try to re-connect and then try again
            rc = connectToDatabase();
            if(rc == 0)
            {
               // OK - we reconnected - try query again
               _result = PQexec(mConn,queryCommand.c_str());
               rc = pqOK(_result);
               if( rc != 0)
               {
                  ErrLog( << "PostgreSQL query failed (twice): " << PQerrorMessage(mConn));
               }
            }
         }
         else
         {
            ErrLog( << "PostgreSQL query failed: " << PQerrorMessage(mConn));
         }
      }
   }

   // Now store result - if pointer to result pointer was supplied and no errors
   if(rc == 0 && result)
   {
      *result = _result;
   }

   if(rc != 0)
   {
      ErrLog( << " SQL Command was: " << queryCommand) ;
   }
   return rc;
}

int
PostgreSqlDb::query(const Data& queryCommand) const
{
   return query(queryCommand, 0);
}

int
PostgreSqlDb::singleResultQuery(const Data& queryCommand, std::vector<Data>& fields) const
{
   PGresult* result=0;
   int rc = query(queryCommand, &result);
      
   if(rc == 0)
   {
      if(result == 0)
      {
         return rc;
      }

      if(PQntuples(result) > 0)
      {
         for(int i = 0; i < PQnfields(result); i++)
         {
            fields.push_back(Data(PQgetvalue(result, 0, i)));
         }
      }
      else
      {
         ErrLog( << "PostgreSQL failed, no error");
      }
      PQclear(result);
   }
   return rc;
}

resip::Data& 
PostgreSqlDb::escapeString(const resip::Data& str, resip::Data& escapedStr) const
{
   int rc = 0;
   escapedStr.truncate2(PQescapeStringConn(mConn, (char*)escapedStr.getBuf(str.size()*2+1), str.c_str(), str.size(), &rc));
   if(rc != 0)
   {
      ErrLog(<< "PostgreSQL string escaping failed: " << PQerrorMessage(mConn));
      // FIXME - should probably throw here.  According to the docs, there is a value in
      // the output buffer even after failure so we'll try to use it and fail later.
   }
   return escapedStr;
}

bool 
PostgreSqlDb::addUser(const AbstractDb::Key& key, const AbstractDb::UserRecord& rec)
{ 
   Data command;
   {
      DataStream ds(command);
      // Use two queries together to simulate UPSERT
      // Real UPSERT is coming in PostgreSQL 9.5
      ds << "UPDATE users SET"
         << " realm='" << rec.realm
         << "', passwordHash='" << rec.passwordHash
         << "', passwordHashAlt='" << rec.passwordHashAlt
         << "', name='" << rec.name
         << "', email='" << rec.email
         << "', forwardAddress='" << rec.forwardAddress
         << "' WHERE username = '" << rec.user
         << "' AND domain='" << rec.domain
         << "'; "
         << "INSERT INTO users (username, domain, realm, passwordHash, passwordHashAlt, name, email, forwardAddress)"
         << " SELECT '" 
         << rec.user << "', '"
         << rec.domain << "', '"
         << rec.realm << "', '"
         << rec.passwordHash << "', '"
         << rec.passwordHashAlt << "', '"
         << rec.name << "', '"
         << rec.email << "', '"
         << rec.forwardAddress << "'"
         << " WHERE NOT EXISTS (SELECT 1 FROM users WHERE "
         << "username = '" << rec.user << "' AND domain = '" << rec.domain << "')";
   }
   return query(command, 0) == 0;
}


AbstractDb::UserRecord 
PostgreSqlDb::getUser( const AbstractDb::Key& key ) const
{
   AbstractDb::UserRecord  ret;

   Data command;
   {
      DataStream ds(command);
      ds << "SELECT username, domain, realm, passwordHash, passwordHashAlt, name, email, forwardAddress FROM users ";
      userWhereClauseToDataStream(key, ds);
   }
   
   PGresult* result=0;
   if(query(command, &result) != 0)
   {
      return ret;
   }
   
   if (result==0)
   {
      ErrLog( << "PostgreSQL failed: " << PQerrorMessage(mConn));
      return ret;
   }

   if (PQntuples(result) > 0)
   {
      int col = 0;
      ret.user            = Data(PQgetvalue(result, 0, col++));
      ret.domain          = Data(PQgetvalue(result, 0, col++));
      ret.realm           = Data(PQgetvalue(result, 0, col++));
      ret.passwordHash    = Data(PQgetvalue(result, 0, col++));
      ret.passwordHashAlt = Data(PQgetvalue(result, 0, col++));
      ret.name            = Data(PQgetvalue(result, 0, col++));
      ret.email           = Data(PQgetvalue(result, 0, col++));
      ret.forwardAddress  = Data(PQgetvalue(result, 0, col++));
   }

   PQclear(result);

   return ret;
}


resip::Data 
PostgreSqlDb::getUserAuthInfo(  const AbstractDb::Key& key ) const
{ 
   std::vector<Data> ret;

   Data command;
   {
      DataStream ds(command);
      Data user;
      Data domain;
      getUserAndDomainFromKey(key, user, domain);
      ds << "SELECT passwordHash FROM users WHERE username = '" << user << "' AND domain = '" << domain << "' ";
   
      // Note: domain is empty when querying for HTTP admin user - for this special user, 
      // we will only check the repro db, by not adding the UNION statement below
      if(!mCustomUserAuthQuery.empty() && !domain.empty())  
      {
         ds << " UNION " << mCustomUserAuthQuery;
         ds.flush();
         command.replace("$user", user);
         command.replace("$domain", domain);
      }
   }

   if(singleResultQuery(command, ret) != 0 || ret.size() == 0)
   {
      return Data::Empty;
   }
   
   DebugLog( << "Auth password is " << ret.front());
   
   return ret.front();
}


AbstractDb::Key 
PostgreSqlDb::firstUserKey()
{  
   // free memory from previous search 
   if (mResult[UserTable])
   {
      PQclear(mResult[UserTable]); 
      mResult[UserTable] = 0;
      mRow[UserTable] = 0;
   }
   
   Data command("SELECT username, domain FROM users");

   if(query(command, &mResult[UserTable]) != 0)
   {
      return Data::Empty;
   }

   if(mResult[UserTable] == 0)
   {
      ErrLog( << "PostgreSQL failed: " << PQerrorMessage(mConn));
      return Data::Empty;
   }
   
   return nextUserKey();
}


AbstractDb::Key 
PostgreSqlDb::nextUserKey()
{ 
   if(mResult[UserTable] == 0)
   { 
      return Data::Empty;
   }
   
   PGresult *result = mResult[UserTable];
   if (mRow[UserTable] >= PQntuples(result))
   {
      PQclear(result);
      mResult[UserTable] = 0;
      mRow[UserTable] = 0;
      return Data::Empty;
   }
   Data user(PQgetvalue(result, mRow[UserTable], 0));
   Data domain(PQgetvalue(result, mRow[UserTable]++, 1));
   
   return user+"@"+domain;
}


bool 
PostgreSqlDb::dbWriteRecord(const Table table, 
                       const resip::Data& pKey, 
                       const resip::Data& pData)
{
   Data command;

   // Check if there is a secondary key or not and get it's value
   char* secondaryKey;
   unsigned int secondaryKeyLen;
   Data escapedKey;
   if(AbstractDb::getSecondaryKey(table, pKey, pData, (void**)&secondaryKey, &secondaryKeyLen) == 0)
   {
      Data escapedSKey;
      Data sKey(Data::Share, secondaryKey, secondaryKeyLen);
      DataStream ds(command);
      ds << "DELETE FROM " << tableName(table)
         << " WHERE attr='" << escapeString(pKey, escapedKey)
         << "' AND attr2='" << escapeString(sKey, escapedSKey)
         << "';"
         << " INSERT INTO " << tableName(table)
         << " (attr, attr2, value) VALUES ("
         << "'" << escapeString(pKey, escapedKey)
         << "', '" << escapeString(sKey, escapedSKey)
         << "', '"  << pData.base64encode()
         << "')";
   }
   else
   {
      DataStream ds(command);
      ds << "DELETE FROM " << tableName(table)
         << " WHERE attr='" << escapeString(pKey, escapedKey)
         << "';"
         << " INSERT INTO " << tableName(table)
         << " (attr, value) VALUES ("
         << "'" << escapeString(pKey, escapedKey)
         << "', '"  << pData.base64encode()
         << "')";
   }

   return query(command, 0) == 0;
}

bool 
PostgreSqlDb::dbReadRecord(const Table table, 
                      const resip::Data& pKey, 
                      resip::Data& pData) const
{ 
   Data command;
   Data escapedKey;
   {
      DataStream ds(command);
      ds << "SELECT value FROM " << tableName(table) 
         << " WHERE attr='" << escapeString(pKey, escapedKey)
         << "'";
   }

   PGresult* result = 0;
   if(query(command, &result) != 0)
   {
      return false;
   }

   if (result == 0)
   {
      ErrLog( << "PostgreSQL result failed: " << PQerrorMessage(mConn));
      return false;
   }
   else
   {
      bool success = false;
      if(PQntuples(result) > 0)
      {
         char *value = PQgetvalue(result, 0, 0);
         pData = Data(Data::Share, value, (Data::size_type)strlen(value)).base64decode();
         success = true;
      }
      PQclear(result);
      StackLog(<<"query result: " << success);
      return success;
   }
}


resip::Data 
PostgreSqlDb::dbNextKey(const Table table, bool first)
{ 
   if(first)
   {
      // free memory from previous search 
      if (mResult[table])
      {
         PQclear(mResult[table]); 
         mResult[table] = 0;
         mRow[table] = 0;
      }
      
      Data command;
      {
         DataStream ds(command);
         ds << "SELECT attr FROM " << tableName(table);
      }
      
      if(query(command, &mResult[table]) != 0)
      {
         return Data::Empty;
      }

      if (mResult[table] == 0)
      {
         ErrLog( << "PostgreSQL failed: " << PQerrorMessage(mConn));
         return Data::Empty;
      }
   }
   else
   {
      if (mResult[table] == 0)
      { 
         return Data::Empty;
      }
   }
   
   PGresult *result = mResult[table];
   if (mRow[table] >= PQntuples(result))
   {
      PQclear(result);
      mResult[table] = 0;
      return Data::Empty;
   }

   return Data(PQgetvalue(result, mRow[table]++, 0));
}


bool 
PostgreSqlDb::dbNextRecord(const Table table,
                      const resip::Data& key,
                      resip::Data& data,
                      bool forUpdate,  // specifying to add SELECT ... FOR UPDATE so the rows are locked
                      bool first)  // return false if no more
{
   if(first)
   {
      // free memory from previous search 
      if (mResult[table])
      {
         PQclear(mResult[table]); 
         mResult[table] = 0;
         mRow[table] = 0;
      }
      
      Data command;
      {
         DataStream ds(command);
         ds << "SELECT value FROM " << tableName(table);
         if(!key.empty())
         {
            Data escapedKey;
            // dbNextRecord is used to iterator through database tables that support duplication records
            // it is only appropriate for PostgreSQL tables that contain the attr2 non-unique index (secondary key)
            ds << " WHERE attr2='" << escapeString(key, escapedKey) << "'";
         }
         if(forUpdate)
         {
            ds << " FOR UPDATE";
         }
      }

      if(query(command, &mResult[table]) != 0)
      {
         return false;
      }

      if (mResult[table] == 0)
      {
         ErrLog( << "PostgreSQL failed: " << PQerrorMessage(mConn));
         return false;
      }
   }
   
   if (mResult[table] == 0)
   { 
      return false;
   }
   
   PGresult *result = mResult[table];
   if (mRow[table] >= PQntuples(result))
   {
      PQclear(result);
      mResult[table] = 0;
      return false;
   }

   char *s = PQgetvalue(result, mRow[table]++, 0);
   data = Data(Data::Share, s, (Data::size_type)strlen(s)).base64decode();

   return true;
}

bool 
PostgreSqlDb::dbBeginTransaction(const Table table)
{
   Data command("SET SESSION CHARACTERISTICS AS TRANSACTION ISOLATION LEVEL REPEATABLE READ");
   if(query(command, 0))
   {
      command = "BEGIN";
      return query(command, 0);
   }
   return false;
}

void 
PostgreSqlDb::userWhereClauseToDataStream(const Key& key, DataStream& ds) const
{
   Data user;
   Data domain;
   getUserAndDomainFromKey(key, user, domain);
   ds << " WHERE username='" << user
      << "' AND domain='" << domain
      << "'";      
}
   
#endif // USE_POSTGRESQL

/* ====================================================================
 * 
 * Copyright (c) 2015 Daniel Pocock.  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * ====================================================================
 */
