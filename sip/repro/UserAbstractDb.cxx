
#ifdef WIN32
#include <db_cxx.h>
#else
#include <db4/db_cxx.h>
#endif

#include <cassert>

#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/MD5Stream.hxx"
#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/Symbols.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/TransactionUser.hxx"

#include "repro/UserAbstractDb.hxx"
#include "resiprocate/dum/UserAuthInfo.hxx"

using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

UserAbstractDb::UserAbstractDb( )
{ 
}

UserAbstractDb::~UserAbstractDb()
{ 
}


void 
UserAbstractDb::requestUserAuthInfo( const resip::Data& user, 
                                     const resip::Data& realm,
                                     const resip::Data& transactionToken,
                                     resip::TransactionUser& transactionUser ) const
{
   // TODO - this shoudl put a message on a local queue then a thread should
   // read that and then do the stuff in the rest of this fucntion
   
   Data key = buildKey(user,realm);
   Data a1 = getUserAuthInfo(key);
    
   UserAuthInfo* msg = new UserAuthInfo(user,realm,a1,transactionToken);
   transactionUser.post( msg );
}


Data 
UserAbstractDb::getUserAuthInfo(  const resip::Data& user, 
                                  const resip::Data& realm ) const
{
   return getUserAuthInfo( buildKey(user,realm) );
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

   assert( !record.empty() );
   UserRecord rec = decodeUserRecord( record );
   DebugLog (<< "getUserAuthInfo A1[" << key << "] = " << rec.passwordHash);
   
   return rec.passwordHash;
}


void 
UserAbstractDb::addUser( const Data& username,
                         const Data& domain,
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
   a1.flush();

   UserAbstractDb::UserRecord rec;
   rec.version = 2;
   rec.user = username;
   rec.domain = domain;
   rec.realm = realm;
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


static void 
encodeString( oDataStream& s, const Data& data )
{
   short len = data.size();
   s.write( (char*)(&len) , sizeof( len ) );
   s.write( data.data(), len );
}


Data 
UserAbstractDb::encodeUserRecord( const UserRecord& rec ) const
{
   Data data;
   oDataStream s(data);
   short len;
   assert( sizeof(len) == 2 );
   
   assert( rec.version == 2 );
   
   assert( sizeof( rec.version) == 2 );
   s.write( (char*)(&rec.version) , sizeof( rec.version ) );
   
   encodeString( s, rec.user );
   encodeString( s, rec.domain);
   encodeString( s, rec.realm);
   encodeString( s, rec.passwordHash);
   encodeString( s, rec.name);
   encodeString( s, rec.email);
   encodeString( s, rec.forwardAddress);

   return data;
}


static Data
decodeString( iDataStream& s)
{
	short len;
	s.read( (char*)(&len), sizeof(len) ); 

	char buf[2048];
	assert( len < 2048 ); // !cj! TODO fix 

	s.read( buf, len );
       
	Data data( buf, len );
	return data;
}


UserAbstractDb::UserRecord 
UserAbstractDb::decodeUserRecord( const Data& pData ) const 
{
   UserAbstractDb::UserRecord rec;

	assert( !pData.empty() );
   Data data = pData;
   
   iDataStream s(data);
   short len;
   assert( sizeof(len) == 2 );

   s.read( (char*)(&len), sizeof(len) );
   rec.version =  len;
   
   if ( rec.version == 2 )
   {
	   rec.user = decodeString( s );
	   rec.domain  = decodeString( s );
	   rec.realm = decodeString( s );
	   rec.passwordHash = decodeString( s );
	   rec.name = decodeString( s );
	   rec.email = decodeString( s );
	   rec.forwardAddress = decodeString( s );
   }
   else
   {
      // unkonwn version 
      ErrLog( <<"Data in user database with unknown version " << rec.version );
      ErrLog( <<"record size is " << pData.size() );
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
