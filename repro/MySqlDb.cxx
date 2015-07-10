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

MySqlDb::MySqlDb(const Data& server, 
                 const Data& user, 
                 const Data& password, 
                 const Data& databaseName, 
                 unsigned int port, 
                 const Data& customUserAuthQuery) :
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
   int rc = 0;

   initialize();

   DebugLog( << "MySqlDb::query: executing query: " << queryCommand);

   Lock lock(mMutex);
   if(mConn == 0 || !isConnected())
   {
      rc = connectToDatabase();
   }
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
MySqlDb::singleResultQuery(const Data& queryCommand, std::vector<Data>& fields) const
{
   MYSQL_RES* result=0;
   int rc = query(queryCommand, &result);
      
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
      }
      mysql_free_result(result);
   }
   return rc;
}

resip::Data& 
MySqlDb::escapeString(const resip::Data& str, resip::Data& escapedStr) const
{
   escapedStr.truncate2(mysql_real_escape_string(mConn, (char*)escapedStr.getBuf(str.size()*2+1), str.c_str(), str.size()));
   return escapedStr;
}

bool 
MySqlDb::addUser(const AbstractDb::Key& key, const AbstractDb::UserRecord& rec)
{ 
   Data command;
   {
      DataStream ds(command);
      ds << "INSERT INTO users (user, domain, realm, passwordHash, passwordHashAlt, name, email, forwardAddress)"
         << " VALUES('" 
         << rec.user << "', '"
         << rec.domain << "', '"
         << rec.realm << "', '"
         << rec.passwordHash << "', '"
         << rec.passwordHashAlt << "', '"
         << rec.name << "', '"
         << rec.email << "', '"
         << rec.forwardAddress << "')"
         << " ON DUPLICATE KEY UPDATE"
         << " user='" << rec.user 
         << "', domain='" << rec.domain
         << "', realm='" << rec.realm
         << "', passwordHash='" << rec.passwordHash
         << "', passwordHashAlt='" << rec.passwordHashAlt
         << "', name='" << rec.name
         << "', email='" << rec.email
         << "', forwardAddress='" << rec.forwardAddress
         << "'";
   }
   return query(command, 0) == 0;
}


AbstractDb::UserRecord 
MySqlDb::getUser( const AbstractDb::Key& key ) const
{
   AbstractDb::UserRecord  ret;

   Data command;
   {
      DataStream ds(command);
      ds << "SELECT user, domain, realm, passwordHash, passwordHashAlt, name, email, forwardAddress FROM users ";
      userWhereClauseToDataStream(key, ds);
   }
   
   MYSQL_RES* result=0;
   if(query(command, &result) != 0)
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
   {
      DataStream ds(command);
      Data user;
      Data domain;
      getUserAndDomainFromKey(key, user, domain);
      ds << "SELECT passwordHash FROM users WHERE user = '" << user << "' AND domain = '" << domain << "' ";
   
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
MySqlDb::firstUserKey()
{  
   // free memory from previous search 
   if (mResult[UserTable])
   {
      mysql_free_result(mResult[UserTable]); 
      mResult[UserTable] = 0;
   }
   
   Data command("SELECT user, domain FROM users");

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
   
   return user+"@"+domain;
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
   Data escapedKey;
   if(AbstractDb::getSecondaryKey(table, pKey, pData, (void**)&secondaryKey, &secondaryKeyLen) == 0)
   {
      Data escapedSKey;
      Data sKey(Data::Share, secondaryKey, secondaryKeyLen);
      DataStream ds(command);
      ds << "REPLACE INTO " << tableName(table)
         << " SET attr='" << escapeString(pKey, escapedKey)
         << "', attr2='" << escapeString(sKey, escapedSKey)
         << "', value='"  << pData.base64encode()
         << "'";
   }
   else
   {
      DataStream ds(command);
      ds << "REPLACE INTO " << tableName(table) 
         << " SET attr='" << escapeString(pKey, escapedKey)
         << "', value='"  << pData.base64encode()
         << "'";
   }

   return query(command, 0) == 0;
}

bool 
MySqlDb::dbReadRecord(const Table table, 
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

   MYSQL_RES* result = 0;
   if(query(command, &result) != 0)
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
      {
         DataStream ds(command);
         ds << "SELECT value FROM " << tableName(table);
         if(!key.empty())
         {
            Data escapedKey;
            // dbNextRecord is used to iterator through database tables that support duplication records
            // it is only appropriate for MySQL tables that contain the attr2 non-unique index (secondary key)
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

void 
MySqlDb::userWhereClauseToDataStream(const Key& key, DataStream& ds) const
{
   Data user;
   Data domain;
   getUserAndDomainFromKey(key, user, domain);
   ds << " WHERE user='" << user
      << "' AND domain='" << domain
      << "'";      
}
   
#endif // USE_MYSQL

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
