
#include <fcntl.h>
#include <db4/db_185.h>
#include <cassert>

#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/MD5Stream.hxx"
#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/Symbols.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/TransactionUser.hxx"

#include "repro/UserDb.hxx"
#include "resiprocate/dum/UserAuthInfo.hxx"

using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

UserAbstractDb::UserAbstractDb( )
{ 
}

UserDb::UserDb( char* fileName )
{ 
   mDb = dbopen(fileName,O_CREAT|O_RDWR,0000600,DB_BTREE,0);
   if ( !mDb )
   {
      ErrLog( <<"Could not open user database at " << fileName );
   }
   assert(mDb);
}


UserAbstractDb::~UserAbstractDb()
{ 
}


UserDb::~UserDb()
{ 
   int ret = mDb->close(mDb);
   assert( ret == 0 );
}


void 
UserAbstractDb::requestUserAuthInfo( const resip::Data& user, 
                                     const resip::Data& realm,
                                     const resip::Data& transactionId,
                                     resip::TransactionUser& transactionUser ) const
{
   Data key = buildKey(user,realm);
   Data a1 = getUserAuthInfo(key);
    
   UserAuthInfo* msg = new UserAuthInfo(user,realm,transactionId, a1);
   transactionUser.post( msg );
}


Data 
UserAbstractDb::getUserAuthInfo( const Data& key ) const
{
   Data record;
   bool ok = dbReadRecord( key, record );
   if (!ok)
   {
      return  Data::Empty;
   }
   
   UserRecord rec = decodeUserRecord( record );

   return rec.passwordHash;
}


void 
UserAbstractDb::addUser( const Data& username,
                         const Data& realm,
                         const Data& password, 
                         const Data& fullName, 
                         const Data& emailAddress )
{
   Data key = buildKey( username, realm );
     
   MD5Stream a1;
   a1 << username
      << Symbols::COLON
      << realm
      << Symbols::COLON
      << password;

   UserAbstractDb::UserRecord rec;
   rec.version = 1;
   rec.passwordHash = a1.getHex();
   rec.name = fullName;
   rec.email = emailAddress;
   rec.forwardAddress = Data::Empty;

   Data data = encodeUserRecord( rec );;
   
   dbWriteRecord( key , data );  
}


void 
UserAbstractDb::removeUser( const Data& aor )
{ 
   Data key = aor;
   dbRemoveRecord(key);
}


Data 
UserAbstractDb::encodeUserRecord( const UserRecord& rec ) const
{
   Data data;
   oDataStream s(data);
   short len;
   assert( sizeof(len) == 2 );
   
   assert( rec.version == 1 );
   
   assert( sizeof( rec.version) == 2 );
   s.write( (char*)(&rec.version) , sizeof( rec.version ) );
   
   len = rec.passwordHash.size();
   s.write( (char*)(&len) , sizeof( len ) );
   s.write( rec.passwordHash.data(), len );
   
   len = rec.name.size();
   s.write( (char*)(&len) , sizeof( len ) );
   s.write( rec.name.data(), len );
   
   len = rec.email.size();
   s.write( (char*)(&len) , sizeof( len ) );
   s.write( rec.email.data(), len );
   
   len = rec.forwardAddress.size();
   s.write( (char*)(&len) , sizeof( len ) );
   s.write( rec.forwardAddress.data(), len );
      
   return data;
}


UserAbstractDb::UserRecord 
UserAbstractDb::decodeUserRecord( const Data& pData ) const 
{
   UserAbstractDb::UserRecord rec;

   Data data = pData;
   
   iDataStream s(data);
   short len;
   assert( sizeof(len) == 2 );

   s.read( (char*)(&len), sizeof(len) );
   rec.version =  len;
   
   if (  rec.version == 1 )
   {
      {
         s.read( (char*)(&len), sizeof(len) ); 
         char buf[len+1];
         s.read( buf, len );
         Data data( buf, len );
         rec.passwordHash = data;
      }
      
      {
         s.read( (char*)(&len), sizeof(len) ); 
         char buf[len+1];
         s.read( buf, len );
         Data data( buf, len );
         rec.name = data;
      }
      
      {
         s.read( (char*)(&len), sizeof(len) ); 
         char buf[len+1];
         s.read( buf, len );
         Data data( buf, len );
         rec.email = data;
      }
      
      {
         s.read( (char*)(&len), sizeof(len) ); 
         char buf[len+1];
         s.read( buf, len );
         Data data( buf, len );
         rec.forwardAddress = data;
      }
   }
   else
   {
      // unkonwn version 
      ErrLog( <<"Data in user database with unknown version " << rec.version );
      assert(0);
   }
      
   return rec;
}


resip::Data 
UserAbstractDb::buildKey( const resip::Data& user, 
                          const resip::Data& realm) const
{
   Data ret = user + Data("@") + realm;
   return ret;
}


void 
UserDb::dbWriteRecord( const Data& pKey, const Data& pData )
{ 
   DBT key,data;
   int ret;

   key.data = const_cast<char*>( pKey.data() );
   key.size = pKey.size();
   data.data = const_cast<char*>( pData.data() );
   data.size = pData.size();
   
   assert( mDb );
   ret = mDb->put(mDb,&key,&data,0);
   assert( ret == 0 );

   // TODO - not sure if next sync is useful 
   ret = mDb->sync(mDb,0);
   assert( ret == 0 );
}


bool 
UserDb::dbReadRecord( const Data& pKey, Data& pData ) const
{ 
   DBT key,data;
   int ret;

   key.data = const_cast<char*>( pKey.data() );
   key.size = pKey.size();
   data.data = 0;
   data.size = 0;
   
   assert( mDb );
   ret = mDb->get(mDb,&key,&data, 0);
   if ( ret ==  -1 )
   {
      assert(0);
      // TODO 
   }
   if ( ret == 1 )
   {
      // key not found 
      return false;
   }
   
   assert( ret == 0 );
   // key was found 
   
   //Data result( Data::Take, reinterpret_cast<const char*>(data.data), data.size );
   Data result( reinterpret_cast<const char*>(data.data), data.size );

   pData = result;
   
   return true;
}


void 
UserDb::dbRemoveRecord( const Data& pKey )
{ 
   DBT key;

   key.data = const_cast<char*>( pKey.data() );
   key.size = pKey.size();

   assert( mDb );
   mDb->del(mDb,&key, 0);
}


resip::Data 
UserAbstractDb::getFirstKey()
{
   return dbFirstKey();
}


resip::Data 
UserAbstractDb::getNextKey()
{
   return dbNextKey();
}


resip::Data 
UserDb::dbFirstKey()
{ 
   DBT key,data;
   int ret;
   
   assert( mDb );
   ret = mDb->seq(mDb,&key,&data, R_FIRST);
   assert( ret != -1 );
   assert( ret != 2 );
   if ( ret == 1 )
   {
      // key not found 
      return Data::Empty;
   }
   assert ( ret == 0 );
   
   //  Data d(Data::Take, reinterpret_cast<const char*>(data.data), data.size );
   Data d( reinterpret_cast<const char*>(key.data), key.size );

   //clog << "got first key of " << d << endl;
   
   return d;
}


resip::Data 
UserDb::dbNextKey()
{ 
   DBT key,data;
   int ret;
   
   assert( mDb );
   ret = mDb->seq(mDb,&key,&data, R_NEXT);
   assert( ret != -1 );
   assert( ret != 2 );
   if ( ret == 1 )
   {
      // key not found 
      return Data::Empty;
   }
   assert ( ret == 0 );
         // key found 
   
   //Data d(Data::Take, reinterpret_cast<const char*>(key.data), key.size );
   Data d(reinterpret_cast<const char*>(key.data), key.size );

   //clog << "Got key of "<< d << endl;
   
   return d;
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
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
