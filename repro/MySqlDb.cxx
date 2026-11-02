#include "rutil/ResipAssert.h"
#include <fcntl.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_MYSQL

#ifdef WIN32
#include <errmsg.h>
#else 
#include <mysql/errmsg.h>
#endif

#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"

#include "repro/AbstractDb.hxx"
#include "repro/MySqlDb.hxx"
#include "repro/UserStore.hxx"

#include "repro/TlsPeerIdentityStore.hxx"


using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

extern "C"
{
   void mysqlThreadEnd(void*)
   {
      mysql_thread_end();
   }
}

// This class helps ensure that each thread using the MySQL API's 
// initialize by calling mysql_thread_init before calling any mySQL functions
class MySQLInitializer
{
   public:
      MySQLInitializer()
      {
         ThreadIf::tlsKeyCreate(mThreadStorage, mysqlThreadEnd);
      }
      ~MySQLInitializer()
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
static MySQLInitializer g_MySQLInitializer;

MySqlDb::MySqlDb(const resip::ConfigParse& config,
                 const Data& server,
                 const Data& user, 
                 const Data& password, 
                 const Data& databaseName, 
                 unsigned int port, 
                 const Data& customUserAuthQuery) :
   SqlDb(config),
   mDBServer(server),
   mDBUser(user),
   mDBPassword(password),
   mDBName(databaseName),
   mDBPort(port),
   mCustomUserAuthQuery(customUserAuthQuery),
   mConn(0)
{ 
   InfoLog( << "Using MySQL DB with server=" << server << ", user=" << user << ", dbName=" << databaseName << ", port=" << port);

   for (int i=0;i<MaxTable;i++)
   {
      mResult[i]=0;
   }

   mysql_library_init(0, 0, 0);
   if(!mysql_thread_safe())
   {
      ErrLog( << "Repro uses MySQL from multiple threads - you MUST link with a thread safe version of the mySQL client library!");
   }
   else
   {
      connectToDatabase();
   }
}


MySqlDb::~MySqlDb()
{
   disconnectFromDatabase();
}

void
MySqlDb::initialize() const
{
   if(!g_MySQLInitializer.isInitialized())
   {
      g_MySQLInitializer.setInitialized();
      mysql_thread_init();
   }
}

void
MySqlDb::disconnectFromDatabase() const
{
   if(mConn)
   {
      for (int i=0;i<MaxTable;i++)
      {
         if (mResult[i])
         {  
            mysql_free_result(mResult[i]); 
            mResult[i]=0;
         }
      }
   
      mysql_close(mConn);
      mConn = 0;
      setConnected(false);
   }
}

int 
MySqlDb::connectToDatabase() const
{
   // Disconnect from database first (if required)
   disconnectFromDatabase();

   // Now try to connect
   resip_assert(mConn == 0);
   resip_assert(isConnected() == false);

   mConn = mysql_init(0);
   if(mConn == 0)
   {
      ErrLog( << "MySQL init failed: insufficient memory.");
      return CR_OUT_OF_MEMORY;
   }

   MYSQL* ret = mysql_real_connect(mConn,
                                   mDBServer.c_str(),   // hostname
                                   mDBUser.c_str(),     // user
                                   mDBPassword.c_str(), // password
                                   mDBName.c_str(),     // DB
                                   mDBPort,             // port
                                   0,                   // unix socket file
                                   CLIENT_MULTI_RESULTS); // client flags (enable multiple results, since some custom stored procedures might require this)

   if (ret == 0)
   { 
      int rc = mysql_errno(mConn);
      ErrLog( << "MySQL connect failed: error=" << rc << ": " << mysql_error(mConn));
      mysql_close(mConn); 
      mConn = 0;
      setConnected(false);
      return rc;
   }
   else
   {
      setConnected(true);
      return 0;
   }
}

int
MySqlDb::query(const Data& queryCommand, MYSQL_RES** result) const
{
   return runQuery(queryCommand, 0, result);
}

int
MySqlDb::query(const Data& queryTemplate, const std::vector<Data>& values, MYSQL_RES** result) const
{
   return runQuery(queryTemplate, &values, result);
}

int
MySqlDb::runQuery(const Data& queryText, const std::vector<Data>* values, MYSQL_RES** result) const
{
   int rc = 0;

   initialize();

   Lock lock(mMutex);
   if(mConn == 0 || !isConnected())
   {
      rc = connectToDatabase();
   }

   // Escape the values here, under the lock and once there is a connection
   Data filled;
   if(rc == 0 && values && !fillPlaceholders(queryText, *values, filled))
   {
      rc = CR_UNKNOWN_ERROR;
   }
   const Data& queryCommand = filled.empty() ? queryText : filled;

   DebugLog( << "MySqlDb::query: executing query: " << queryCommand);

   if(rc == 0)
   {
      resip_assert(mConn!=0);
      resip_assert(isConnected());
      rc = mysql_query(mConn,queryCommand.c_str());
      if(rc != 0)
      {
         rc = mysql_errno(mConn);
         if(rc == CR_SERVER_GONE_ERROR ||
            rc == CR_SERVER_LOST)
         {
            // First failure is a connection error - try to re-connect and then try again
            rc = connectToDatabase();
            if(rc == 0)
            {
               // OK - we reconnected - try query again
               rc = mysql_query(mConn,queryCommand.c_str());
               if( rc != 0)
               {
                  ErrLog( << "MySQL query failed: error=" << mysql_errno(mConn) << ": " << mysql_error(mConn));
               }
            }
         }
         else
         {
            ErrLog( << "MySQL query failed: error=" << mysql_errno(mConn) << ": " << mysql_error(mConn));
         }
      }
   }

   // Now store result - if pointer to result pointer was supplied and no errors
   if(rc == 0 && result)
   {
      *result = mysql_store_result(mConn);
      if(*result == 0)
      {
         rc = mysql_errno(mConn);
         if(rc != 0)
         {
            ErrLog( << "MySQL store result failed: error=" << rc << ": " << mysql_error(mConn));
         }
      }
   }

   if(rc != 0)
   {
      ErrLog( << " SQL Command was: " << queryCommand) ;
   }
   return rc;
}

int
MySqlDb::query(const Data& queryCommand) const
{
   return query(queryCommand, 0);
}

int
MySqlDb::query(const Data& queryTemplate, const std::vector<Data>& values) const
{
   return query(queryTemplate, values, 0);
}

int
MySqlDb::singleResultQuery(const Data& queryCommand, std::vector<Data>& fields) const
{
   return runSingleResultQuery(queryCommand, 0, fields);
}

int
MySqlDb::singleResultQuery(const Data& queryTemplate, const std::vector<Data>& values, std::vector<Data>& fields) const
{
   return runSingleResultQuery(queryTemplate, &values, fields);
}

int
MySqlDb::runSingleResultQuery(const Data& queryText, const std::vector<Data>* values, std::vector<Data>& fields) const
{
   StackLog(<<"executing query: " << queryText);
   MYSQL_RES* result=0;
   int rc = runQuery(queryText, values, &result);
      
   if(rc == 0)
   {
      if(result == 0)
      {
         return rc;
      }

      MYSQL_ROW row = mysql_fetch_row(result);
      if(row)
      {
         for(unsigned int i = 0; i < result->field_count; i++)
         {
            fields.push_back(Data(row[i]));
         }
      }
      else
      {
         rc = mysql_errno(mConn);
         if(rc != 0)
         {
            ErrLog( << "MySQL fetch row failed: error=" << rc << ": " << mysql_error(mConn));
         }
         else
         {
            DebugLog(<<"singleResultQuery: no rows returned by query");
         }
      }
      mysql_free_result(result);
   }
   return rc;
}

bool
MySqlDb::escapeValue(const resip::Data& str, resip::Data& escapedStr) const
{
   if(mConn == 0)
   {
      ErrLog( << "MySQL string escaping failed: not connected");
      escapedStr.clear();
      return false;
   }
   unsigned long len = mysql_real_escape_string(mConn, (char*)escapedStr.getBuf(str.size()*2+1), str.c_str(), str.size());
   if(len == (unsigned long)-1)
   {
      ErrLog( << "MySQL string escaping failed: error=" << mysql_errno(mConn) << ": " << mysql_error(mConn));
      escapedStr.clear();
      return false;
   }
   escapedStr.truncate2(len);
   return true;
}

bool 
MySqlDb::addUser(const AbstractDb::Key& key, const AbstractDb::UserRecord& rec)
{ 
   Data command;
   {
      DataStream ds(command);
      ds << "INSERT INTO " << tableName(UserTable) << " (user, domain, realm, passwordHash, passwordHashAlt, name, email, forwardAddress)"
         << " VALUES('?', '?', '?', '?', '?', '?', '?', '?')"
         << " ON DUPLICATE KEY UPDATE"
         << " user='?', domain='?', realm='?', passwordHash='?',"
         << " passwordHashAlt='?', name='?', email='?', forwardAddress='?'";
   }
   const std::vector<Data> fields = { rec.user, rec.domain, rec.realm, rec.passwordHash,
                                      rec.passwordHashAlt, rec.name, rec.email, rec.forwardAddress };
   // once for VALUES and again for the UPDATE
   std::vector<Data> values(fields);
   values.insert(values.end(), fields.begin(), fields.end());
   return query(command, values, 0) == 0;
}


AbstractDb::UserRecord 
MySqlDb::getUser( const AbstractDb::Key& key ) const
{
   AbstractDb::UserRecord  ret;

   Data command;
   std::vector<Data> values;
   {
      DataStream ds(command);
      ds << "SELECT user, domain, realm, passwordHash, passwordHashAlt, name, email, forwardAddress FROM " << tableName(UserTable) << " ";
      userWhereClauseToDataStream(key, ds, values);
   }
   
   MYSQL_RES* result=0;
   if(query(command, values, &result) != 0)
   {
      return ret;
   }
   
   if (result==0)
   {
      ErrLog( << "MySQL store result failed: error=" << mysql_errno(mConn) << ": " << mysql_error(mConn));
      return ret;
   }

   MYSQL_ROW row = mysql_fetch_row(result);
   if (row)
   {
      int col = 0;
      ret.user            = Data(row[col++]);
      ret.domain          = Data(row[col++]);
      ret.realm           = Data(row[col++]);
      ret.passwordHash    = Data(row[col++]);
      ret.passwordHashAlt = Data(row[col++]);
      ret.name            = Data(row[col++]);
      ret.email           = Data(row[col++]);
      ret.forwardAddress  = Data(row[col++]);
   }

   mysql_free_result(result);

   return ret;
}


resip::Data 
MySqlDb::getUserAuthInfo(  const AbstractDb::Key& key ) const
{ 
   std::vector<Data> ret;

   Data command;
   std::vector<Data> values;
   {
      DataStream ds(command);
      Data user;
      Data domain;
      UserStore::getUserAndDomainFromKey(key, user, domain);
      // user is the digest username of a request that isn't authenticated yet,
      // so it only goes in as a value for query() to escape
      ds << "SELECT passwordHash FROM " << tableName(UserTable) << " WHERE user = '?' AND domain = '?' ";
      values.push_back(user);
      values.push_back(domain);

      // Note: domain is empty when querying for HTTP admin user - for this special user, 
      // we will only check the repro db, by not adding the UNION statement below
      if(!mCustomUserAuthQuery.empty() && !domain.empty())  
      {
         ds << " UNION ";
         appendCustomUserAuthQuery(mCustomUserAuthQuery, user, domain, ds, values);
      }
   }

   if(singleResultQuery(command, values, ret) != 0 || ret.size() == 0)
   {
      return Data::Empty;
   }
   
   DebugLog( << "Auth password is " << ret.front());
   
   return ret.front();
}


AbstractDb::Key 
MySqlDb::firstUserKey()
{  
   // free memory from previous search 
   if (mResult[UserTable])
   {
      mysql_free_result(mResult[UserTable]); 
      mResult[UserTable] = 0;
   }
   
   Data command;
   {
      DataStream ds(command);
      ds << "SELECT user, domain FROM " << tableName(UserTable);
   }

   if(query(command, &mResult[UserTable]) != 0)
   {
      return Data::Empty;
   }

   if(mResult[UserTable] == 0)
   {
      ErrLog( << "MySQL store result failed: error=" << mysql_errno(mConn) << ": " << mysql_error(mConn));
      return Data::Empty;
   }
   
   return nextUserKey();
}


AbstractDb::Key 
MySqlDb::nextUserKey()
{ 
   if(mResult[UserTable] == 0)
   { 
      return Data::Empty;
   }
   
   MYSQL_ROW row = mysql_fetch_row(mResult[UserTable]);
   if (!row)
   {
      mysql_free_result(mResult[UserTable]); 
      mResult[UserTable] = 0;
      return Data::Empty;
   }
   Data user(row[0]);
   Data domain(row[1]);
   
   return UserStore::buildKey(user, domain);
}


bool 
MySqlDb::addTlsPeerIdentity(const AbstractDb::Key& key, const AbstractDb::TlsPeerIdentityRecord& rec)
{
   Data command;
   {
      DataStream ds(command);
      ds << "INSERT INTO " << tableName(TlsPeerIdentityTable) << " (peerName, authorizedIdentity)"
         << " VALUES('?', '?')"
         << " ON DUPLICATE KEY UPDATE"
         << " peerName='?', authorizedIdentity ='?'";
   }
   const std::vector<Data> values = { rec.peerName, rec.authorizedIdentity,
                                      rec.peerName, rec.authorizedIdentity };
   return query(command, values, 0) == 0;
}


AbstractDb::TlsPeerIdentityRecord
MySqlDb::getTlsPeerIdentity( const AbstractDb::Key& key ) const
{
   AbstractDb::TlsPeerIdentityRecord  ret;

   Data command;
   std::vector<Data> values;
   {
      DataStream ds(command);
      ds << "SELECT peerName, authorizedIdentity FROM " << tableName(TlsPeerIdentityTable) << " ";
      tlsPeerIdentityWhereClauseToDataStream(key, ds, values);
   }

   MYSQL_RES* result=0;
   if(query(command, values, &result) != 0)
   {
      return ret;
   }

   if (result==0)
   {
      ErrLog( << "MySQL store result failed: error=" << mysql_errno(mConn) << ": " << mysql_error(mConn));
      return ret;
   }

   MYSQL_ROW row = mysql_fetch_row(result);
   if (row)
   {
      int col = 0;
      ret.peerName        = Data(row[col++]);
      ret.authorizedIdentity = Data(row[col++]);
   }

   mysql_free_result(result);

   return ret;
}


AbstractDb::Key
MySqlDb::firstTlsPeerIdentityKey()
{
   // free memory from previous search
   if (mResult[TlsPeerIdentityTable])
   {
      mysql_free_result(mResult[TlsPeerIdentityTable]);
      mResult[TlsPeerIdentityTable] = 0;
   }
 
   Data command;
   {
      DataStream ds(command);
      ds << "SELECT peerName, authorizedIdentity FROM " << tableName(TlsPeerIdentityTable);
   }

   if(query(command, &mResult[TlsPeerIdentityTable]) != 0)
   {
      return Data::Empty;
   }

   if(mResult[TlsPeerIdentityTable] == 0)
   {
      ErrLog( << "MySQL store result failed: error=" << mysql_errno(mConn) << ": " << mysql_error(mConn));
      return Data::Empty;
   }

   return nextTlsPeerIdentityKey();
}


AbstractDb::Key
MySqlDb::nextTlsPeerIdentityKey()
{
   if(mResult[TlsPeerIdentityTable] == 0)
   {
      return Data::Empty;
   }
 
   MYSQL_ROW row = mysql_fetch_row(mResult[TlsPeerIdentityTable]);
   if (!row)
   {
      mysql_free_result(mResult[TlsPeerIdentityTable]);
      mResult[TlsPeerIdentityTable] = 0;
      return Data::Empty;
   }
   Data peerName(row[0]);
   Data authorizedIdentity(row[1]);
 
   return TlsPeerIdentityStore::buildKey(peerName, authorizedIdentity);
}


bool
MySqlDb::dbWriteRecord(const Table table, 
                       const resip::Data& pKey, 
                       const resip::Data& pData)
{
   Data command;

   // Check if there is a secondary key or not and get it's value
   char* secondaryKey;
   unsigned int secondaryKeyLen;
   std::vector<Data> values;
   if(AbstractDb::getSecondaryKey(table, pKey, pData, (void**)&secondaryKey, &secondaryKeyLen) == 0)
   {
      DataStream ds(command);
      ds << "REPLACE INTO " << tableName(table)
         << " SET attr='?', attr2='?', value='?'";
      values.push_back(pKey);
      values.push_back(Data(secondaryKey, secondaryKeyLen));
   }
   else
   {
      DataStream ds(command);
      ds << "REPLACE INTO " << tableName(table)
         << " SET attr='?', value='?'";
      values.push_back(pKey);
   }
   values.push_back(pData.base64encode());

   return query(command, values, 0) == 0;
}

bool 
MySqlDb::dbReadRecord(const Table table, 
                      const resip::Data& pKey, 
                      resip::Data& pData) const
{ 
   Data command;
   {
      DataStream ds(command);
      ds << "SELECT value FROM " << tableName(table)
         << " WHERE attr='?'";
   }

   MYSQL_RES* result = 0;
   if(query(command, std::vector<Data>(1, pKey), &result) != 0)
   {
      return false;
   }

   if (result == 0)
   {
      ErrLog( << "MySQL store result failed: error=" << mysql_errno(mConn) << ": " << mysql_error(mConn));
      return false;
   }
   else
   {
      bool success = false;
      MYSQL_ROW row=mysql_fetch_row(result);
      if(row)
      {
         pData = Data(Data::Share, row[0], (Data::size_type)strlen(row[0])).base64decode();
         success = true;
      }
      mysql_free_result(result);
      return success;
   }
}


resip::Data 
MySqlDb::dbNextKey(const Table table, bool first)
{ 
   if(first)
   {
      // free memory from previous search 
      if (mResult[table])
      {
         mysql_free_result(mResult[table]); 
         mResult[table] = 0;
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
         ErrLog( << "MySQL store result failed: error=" << mysql_errno(mConn) << ": " << mysql_error(mConn));
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
   
   MYSQL_ROW row = mysql_fetch_row(mResult[table]);
   if (!row)
   {
      mysql_free_result(mResult[table]); 
      mResult[table] = 0;
      return Data::Empty;
   }

   return Data(row[0]);
}


bool 
MySqlDb::dbNextRecord(const Table table,
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
         mysql_free_result(mResult[table]); 
         mResult[table] = 0;
      }
      
      Data command;
      std::vector<Data> values;
      {
         DataStream ds(command);
         ds << "SELECT value FROM " << tableName(table);
         if(!key.empty())
         {
            // dbNextRecord is used to iterator through database tables that support duplication records
            // it is only appropriate for MySQL tables that contain the attr2 non-unique index (secondary key)
            ds << " WHERE attr2='?'";
            values.push_back(key);
         }
         if(forUpdate)
         {
            ds << " FOR UPDATE";
         }
      }

      if(query(command, values, &mResult[table]) != 0)
      {
         return false;
      }

      if (mResult[table] == 0)
      {
         ErrLog( << "MySQL store result failed: error=" << mysql_errno(mConn) << ": " << mysql_error(mConn));
         return false;
      }
   }
   
   if (mResult[table] == 0)
   { 
      return false;
   }
   
   MYSQL_ROW row = mysql_fetch_row(mResult[table]);
   if (!row)
   {
      mysql_free_result(mResult[table]); 
      mResult[table] = 0;
      return false;
   }

   data = Data(Data::Share, row[0], (Data::size_type)strlen(row[0])).base64decode();

   return true;
}

bool 
MySqlDb::dbBeginTransaction(const Table table)
{
   Data command("SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ");
   if(query(command, 0) == 0)
   {
      command = "START TRANSACTION";
      return query(command, 0) == 0;
   }
   return false;
}

#endif // USE_MYSQL

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2026, SIP Spectrum, Inc. http://sipspectrum.com
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
