#include <cassert>
#include <fcntl.h>

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

MySqlDb::MySqlDb(const Data& server, const Data& user, const Data& password, const Data& databaseName, unsigned int port) :
   mDBServer(server),
   mDBUser(user),
   mDBPassword(password),
   mDBName(databaseName),
   mDBPort(port),
   mConn(0),
   mConnected(false)
{ 
   InfoLog( << "Using MySQL DB with server=" << server << ", user=" << user << ", dbName=" << databaseName << ", port=" << port);

   assert( MaxTable <= 4 );
   for (int i=0;i<MaxTable;i++)
   {
      mResult[i]=0;
   }

   connectToDatabase();
}


MySqlDb::~MySqlDb()
{
   disconnectFromDatabase();
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
      mConnected = false;
   }
}

int 
MySqlDb::connectToDatabase() const
{
   // Disconnect from database first (if required)
   disconnectFromDatabase();

   // Now try to connect
   assert(mConn == 0);
   assert(mConnected == false);

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
                                   0);                  // client flags
   if (ret == 0)
   { 
      int rc = mysql_errno(mConn);
      ErrLog( << "MySQL connect failed: error=" << rc << ": " << mysql_error(mConn));
      mysql_close(mConn); 
      mConn = 0;
      mConnected = false;
      return rc;
   }
   else
   {
      mConnected = true;
      return 0;
   }
}

int  
MySqlDb::query(const Data& queryCommand) const
{
   int rc = 0;
   if(mConn == 0 || !mConnected)
   {
      rc = connectToDatabase();
   }
   if(rc == 0)
   {
      assert(mConn!=0);
      assert(mConnected);
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

   if(rc != 0)
   {
      ErrLog( << " SQL Command was: " << queryCommand) ;
   }
   return rc;
}

void 
MySqlDb::addUser( const AbstractDb::Key& key, const AbstractDb::UserRecord& rec )
{ 
   Data command = Data("REPLACE INTO users SET ")
      +"user='" + rec.user + "', "
      +"domain='" + rec.domain + "', "
      +"realm='" + rec.realm + "', "
      +"passwordHash='" + rec.passwordHash + "', "
      +"name='" + rec.name + "', "
      +"email='" + rec.email + "', "
      +"forwardAddress='" + rec.forwardAddress + "'";
   query(command);
}


void 
MySqlDb::eraseUser( const AbstractDb::Key& key )
{ 
   Data command = Data("DELETE FROM users ") + userDomainWhere(key);   
   query(command);
}


AbstractDb::UserRecord 
MySqlDb::getUser( const AbstractDb::Key& key ) const
{
   AbstractDb::UserRecord  ret;

   Data command = Data("SELECT "
                       "user, domain, realm, passwordHash, name "
                       "email, forwardAddress FROM users ") + userDomainWhere(key);
   
   if(query(command) != 0)
   {
      return ret;
   }
   
   MYSQL_RES* result = mysql_store_result(mConn);
   if (result==0)
   {
      ErrLog( << "MySQL store result failed: error=" << mysql_errno(mConn) << ": " << mysql_error(mConn));
      return ret;
   }

   MYSQL_ROW row = mysql_fetch_row(result);
   if (row)
   {
      ret.user           = Data(row[0]);
      ret.domain         = Data(row[1]);
      ret.realm          = Data(row[2]);
      ret.passwordHash   = Data(row[3]);
      ret.name           = Data(row[4]);
      ret.email          = Data(row[5]);
      ret.forwardAddress = Data(row[6]);
   }

   mysql_free_result(result);

   return ret;
}


resip::Data 
MySqlDb::getUserAuthInfo(  const AbstractDb::Key& key ) const
{ 
   Data ret;

   Data command = Data("SELECT passwordHash FROM users ") + userDomainWhere(key);
   
   if(query(command) != 0)
   {
      return Data::Empty;
   }
   
   MYSQL_RES* result = mysql_store_result(mConn);
   if(result == 0)
   {
      ErrLog( << "MySQL store result failed: error=" << mysql_errno(mConn) << ": " << mysql_error(mConn));
      return ret;
   }

   MYSQL_ROW row = mysql_fetch_row(result);
   if(row)
   {
      ret = Data(row[0]);
   }
   mysql_free_result(result);

   DebugLog( << "Auth password is " << ret);
   
   return ret;
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
   
   Data command = Data("SELECT user, domain FROM users");

   if(query(command) != 0)
   {
      return Data::Empty;
   }

   mResult[UserTable] = mysql_store_result(mConn);
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


void 
MySqlDb::dbWriteRecord( const Table table, 
                          const resip::Data& pKey, 
                          const resip::Data& pData )
{
   Data command = Data("REPLACE INTO ")+tableName(table)
      +" SET attr='" + pKey + "', value='" +pData.base64encode()+ "'";

   query(command);
}


bool 
MySqlDb::dbReadRecord(const Table table, 
                      const resip::Data& pKey, 
                      resip::Data& pData) const
{ 
   Data command = Data("SELECT value FROM ")+tableName(table)
      +" WHERE attr='" + pKey + "'";

   if(query(command) != 0)
   {
      return false;
   }

   MYSQL_RES* result = mysql_store_result(mConn);
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
         Data enc(row[0]);          
         pData = enc.base64decode();
         success = true;
      }
      mysql_free_result(result);
      return success;
   }
}


void 
MySqlDb::dbEraseRecord( const Table table, 
                        const resip::Data& pKey )
{ 
   Data command = Data("DELETE FROM ") + tableName(table) +
                       " WHERE attr='" + pKey + "'";
   
   query(command);
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
      
      Data command = Data("SELECT attr FROM ")+tableName(table);
      
      if(query(command) != 0)
      {
         return Data::Empty;
      }

      mResult[table] = mysql_store_result(mConn);
      if (mResult[table] == 0)
      {
         ErrLog( << "MySQL store result failed: error=" << mysql_errno(mConn) << ": " << mysql_error(mConn));
         return Data::Empty;
      }
   }
   
   if (mResult[table] == 0)
   { 
      return Data::Empty;
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


char*
MySqlDb::tableName(Table table) const
{
   switch (table)
   {
      case UserTable:
         assert(false);  // usersavp is not used!
         return "usersavp";
      case RouteTable:
         return "routesavp";
      case AclTable:
         return "aclsavp"; 
      case ConfigTable:
         return "configsavp";
      default:
         assert(0);
   }
   return 0;
}

resip::Data 
MySqlDb::userDomainWhere( const AbstractDb::Key& key) const
{ 
   Data user;
   Data domain;
   
   ParseBuffer pb(key);
   const char* start = pb.position();
   pb.skipToOneOf("@");
   pb.data(user, start);
   const char* anchor = pb.skipChar();
   pb.skipToEnd();
   pb.data(domain, anchor);
   Data ret = "WHERE ";
   ret += "user='" + user + "' ";
   ret += " AND domain='" + domain + "' ";
   
   DebugLog( << "userDomainWhere returing: << "<< ret );
   
   return ret;
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
