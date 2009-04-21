#include <cassert>
#include <fcntl.h>

#ifdef USE_MYSQL

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



MySqlDb::MySqlDb( const Data& server )
{ 
   InfoLog( << "Using MySQL DB with server: " << server );

   mConn = mysql_init(NULL);
   assert(mConn);

   MYSQL* ret = mysql_real_connect(mConn,
                                   server.c_str(), // hostname
                                   "repro",//user
                                   NULL,//password
                                   "repro",//DB
                                   0,//port
                                   NULL,//opt
                                   0 /*flags*/ );
   if ( ret == NULL )
   {
      ErrLog( << "MySQL connect failed: " << mysql_error(mConn) );
      mysql_close(mConn);

      throw; /* !cj! TODO fix up */
   }
 
   assert( MaxTable <= 4 );
   for (int i=0;i<MaxTable;i++)
   {
      mResult[i]=NULL;
   }
}


MySqlDb::~MySqlDb()
{  
   for (int i=0;i<MaxTable;i++)
   {
      if ( mResult[i] )
      {  
         mysql_free_result( mResult[i] ); mResult[i]=0;
      }
   }
   
   mysql_close(mConn);
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
   
   int r;
   r = mysql_query(mConn,command.c_str());
   if (r!= 0)
   {
      ErrLog( << "MySQL write record failed: " << mysql_error(mConn)  ) ;
      ErrLog( << " SQL Command was: " << command  ) ;
   }
}


void 
MySqlDb::eraseUser( const AbstractDb::Key& key )
{ 
   Data command = Data("DELETE FROM users ") + sqlWhere(key);
   
   int r;
   r = mysql_query(mConn,command.c_str());
   if (r!= 0)
   {
      ErrLog( << "MySQL read failed: " << mysql_error(mConn) );
      ErrLog( << " SQL Command was: " << command  ) ;
      throw; /* !cj! TODO FIX */ 
   }
}


AbstractDb::UserRecord 
MySqlDb::getUser( const AbstractDb::Key& key ) const
{
   AbstractDb::UserRecord  ret;

   Data command = Data("SELECT "
                       "user, domain, realm, passwordHash, name "
                       "email, forwardAddress FROM users ") + sqlWhere(key);
   
   int r;
   r = mysql_query(mConn,command.c_str());
   if (r!= 0)
   {
      ErrLog( << "MySQL read failed: " << mysql_error(mConn) );
      ErrLog( << " SQL Command was: " << command  ) ;
      throw; /* !cj! TODO FIX */ 
   }
   
   MYSQL_RES* result = mysql_store_result(mConn);
   if (result==NULL)
   {
      ErrLog( << "MySQL store result failed: " << mysql_error(mConn) );
      throw; /* !cj! TODO FIX */ 
   }

   MYSQL_ROW row=mysql_fetch_row(result);
   if ( row )
   {
      ret.user = Data( row[0] );
      ret.domain = Data( row[1] );
      ret.realm = Data( row[2] );
      ret.passwordHash = Data( row[3] );
      ret.name = Data( row[4] );
      ret.email = Data( row[5] );
      ret.forwardAddress = Data( row[6] );
   }
   mysql_free_result( result );

   return ret;
}


resip::Data 
MySqlDb::getUserAuthInfo(  const AbstractDb::Key& key ) const
{ 
   Data ret;

   Data command = Data("SELECT passwordHash FROM users ") + sqlWhere(key);
   
   int r;
   r = mysql_query(mConn,command.c_str());
   if (r!= 0)
   {
      ErrLog( << "MySQL read failed: " << mysql_error(mConn) );
      ErrLog( << " SQL Command was: " << command  ) ;
      throw; /* !cj! TODO FIX */ 
   }
   
   MYSQL_RES* result = mysql_store_result(mConn);
   if (result==NULL)
   {
      ErrLog( << "MySQL store result failed: " << mysql_error(mConn) );
      throw; /* !cj! TODO FIX */ 
   }

   MYSQL_ROW row=mysql_fetch_row(result);
   if ( row )
   {
      ret = Data( row[0] );
   }
   mysql_free_result( result );

   DebugLog( << "Auth password is " << ret );
   
   return ret;
}


AbstractDb::Key 
MySqlDb::firstUserKey()
{  
   // free memory from previos search 
   if ( mResult[UserTable] )
   {
      mysql_free_result( mResult[UserTable] ); mResult[UserTable]=0;
   }
   
   Data command = Data("SELECT user, domain FROM users");
   
   int r;
   r = mysql_query(mConn,command.c_str());
   if (r!= 0)
   {
      ErrLog( << "MySQL read table failed: " << mysql_error(mConn) );
      ErrLog( << " SQL Command was: " << command  ) ;
      throw; /* !cj! TODO FIX */ 
   }
   else
   {
      mResult[UserTable] = mysql_store_result(mConn);
      if (mResult[UserTable]==NULL)
      {
         ErrLog( << "MySQL store result failed: " << mysql_error(mConn) );
         throw; /* !cj! TODO FIX */ 
      }
   }
   
   return nextUserKey();
}


AbstractDb::Key 
MySqlDb::nextUserKey()
{ 
   if ( mResult[UserTable] == NULL )
   { 
      return Data::Empty;
   }
   
   MYSQL_ROW row=mysql_fetch_row( mResult[UserTable] );
   if ( !row )
   {
      mysql_free_result( mResult[UserTable]  ); mResult[UserTable]=0;
      return Data::Empty;
   }
   Data user( row[0] );
   Data domain( row[1] );
   
   return user+"@"+domain;
}


void 
MySqlDb::dbWriteRecord( const Table table, 
                          const resip::Data& pKey, 
                          const resip::Data& pData )
{
   Data command = Data("REPLACE INTO ")+tableName(table)
      +" SET attr='" + pKey + "', value='" +pData.base64encode()+ "'";
   
   int r;
   r = mysql_query(mConn,command.c_str());
   if (r!= 0)
   {
      ErrLog( << "MySQL write record failed: " << mysql_error(mConn)  ) ;
      ErrLog( << " SQL Command was: " << command  ) ;
   }
}


bool 
MySqlDb::dbReadRecord( const Table table, 
                         const resip::Data& pKey, 
                         resip::Data& pData ) const
{ 
   Data command = Data("SELECT value FROM ")+tableName(table)
      +" WHERE attr='" + pKey + "'";
   
   int r;
   r = mysql_query(mConn,command.c_str());
   if (r!= 0)
   {
      ErrLog( << "MySQL read failed: " << mysql_error(mConn) );
      ErrLog( << " SQL Command was: " << command  ) ;
      throw; /* !cj! TODO FIX */ 
   }
   else
   {
      MYSQL_RES* result = mysql_store_result(mConn);
      if (result==NULL)
      {
         ErrLog( << "MySQL store result failed: " << mysql_error(mConn) );
         throw; /* !cj! TODO FIX */ 
      }
      else
      {
         MYSQL_ROW row=mysql_fetch_row(result);
         if ( !row )
         {
            pData = Data::Empty;
         }
         else
         {
            Data enc( row[0] );
            
            pData = enc.base64decode();
            
            return true;
         }
         mysql_free_result( result );
      }
   } 
   return false;
}


void 
MySqlDb::dbEraseRecord( const Table table, 
                          const resip::Data& pKey )
{ 
  Data command = Data("DELETE FROM ")+tableName(table)
      +" WHERE attr='" + pKey + "'";
   
   int r;
   r = mysql_query(mConn,command.c_str());
   if (r!= 0)
   {
      ErrLog( << "MySQL read failed: " << mysql_error(mConn) );
      ErrLog( << " SQL Command was: " << command  ) ;
      throw; /* !cj! TODO FIX */ 
   }
}


resip::Data 
MySqlDb::dbNextKey( const Table table, 
                      bool first)
{ 
   if (first)
   {
      // free memory from previos search 
      if ( mResult[table] )
      {
         mysql_free_result( mResult[table] ); mResult[table]=0;
      }
      
      Data command = Data("SELECT attr FROM ")+tableName(table);
      
      int r;
      r = mysql_query(mConn,command.c_str());
      if (r!= 0)
      {
         ErrLog( << "MySQL read table failed: " << mysql_error(mConn) );
         ErrLog( << " SQL Command was: " << command  ) ;
        throw; /* !cj! TODO FIX */ 
      }
      else
      {
         mResult[table] = mysql_store_result(mConn);
         if (mResult[table]==NULL)
         {
            ErrLog( << "MySQL store result failed: " << mysql_error(mConn) );
            throw; /* !cj! TODO FIX */ 
         }
      }
   }
   
   if ( mResult[table] == NULL )
   { 
      return Data::Empty;
   }
   
   MYSQL_ROW row=mysql_fetch_row( mResult[table] );
   if ( !row )
   {
      mysql_free_result( mResult[table]  ); mResult[table]=0;
      return Data::Empty;
   }

   return Data( row[0] );
}


char*
MySqlDb::tableName( Table table ) const
{
   switch (table)
   {
      case UserTable:
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
MySqlDb::sqlWhere( const AbstractDb::Key& key) const
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
   
   DebugLog( << "sqlWhere returing: << "<< ret );
   
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
