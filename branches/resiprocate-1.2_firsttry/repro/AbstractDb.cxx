#include <cassert>

#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/Logger.hxx"


#include "repro/UserStore.hxx"
#include "repro/AbstractDb.hxx"


using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO


static void 
encodeString( oDataStream& s, const Data& data )
{
   short len = (short)data.size();
   s.write( (char*)(&len) , sizeof( len ) );
   s.write( data.data(), len );
}


static Data
decodeString( iDataStream& s)
{
	short len;
	s.read( (char*)(&len), sizeof(len) ); 

   // [TODO] This is probably OK for now, but we can do better than this.
   if (len > 8192)
   {
      ErrLog( << "Tried to decode a database record that was much larger (>8k) than expected.  Returning an empty Data instead." );
      return Data::Empty;
   }

	char buf[8192];
	s.read( buf, len );
       
	Data data( buf, len );
	return data;
}


Data 
AbstractDb::dbFirstKey(const AbstractDb::Table table)
{
   return dbNextKey(table,true/*first*/);
}


AbstractDb::AbstractDb()
{
}


AbstractDb::~AbstractDb()
{
}


void 
AbstractDb::addUser( const AbstractDb::Key& key, const AbstractDb::UserRecord& rec )
{  
   assert( !key.empty() );
   
   Data data;
   {
      oDataStream s(data);
      
      short version=2;
      assert( sizeof( version) == 2 );
      s.write( (char*)(&version) , sizeof(version) );
      
      encodeString( s, rec.user );
      encodeString( s, rec.domain);
      encodeString( s, rec.realm);
      encodeString( s, rec.passwordHash);
      encodeString( s, rec.name);
      encodeString( s, rec.email);
      encodeString( s, rec.forwardAddress);
      s.flush();
   }
   
   dbWriteRecord(UserTable,key,data);
}


void 
AbstractDb::eraseUser( const AbstractDb::Key& key )
{
   dbEraseRecord( UserTable, key);
}


void 
AbstractDb::writeUser( const AbstractDb::Key& oldkey, const AbstractDb::Key& newkey, const AbstractDb::UserRecord& rec )
{  
   assert( !oldkey.empty() );
   assert( !newkey.empty() );
   
   Data data;
   {
      oDataStream s(data);
      
      short version=2;
      assert( sizeof( version) == 2 );
      s.write( (char*)(&version) , sizeof(version) );
      
      encodeString( s, rec.user );
      encodeString( s, rec.domain);
      encodeString( s, rec.realm);
      encodeString( s, rec.passwordHash);
      encodeString( s, rec.name);
      encodeString( s, rec.email);
      encodeString( s, rec.forwardAddress);
      s.flush();
   }
   
   if (oldkey != newkey)   // the domain or user (or both) changed, so the key has changed
   {
      dbEraseRecord( UserTable, oldkey);
   }
   
   dbWriteRecord(UserTable,newkey,data);
}


AbstractDb::UserRecord 
AbstractDb::getUser( const AbstractDb::Key& key ) const
{
   AbstractDb::UserRecord rec;
   Data data;
   bool stat = dbReadRecord( UserTable, key, data );
   if ( !stat )
   {
      return rec;
   }
   if ( data.empty() )
   {
      return rec;
   }

   iDataStream s(data);

   short version;
   assert( sizeof(version) == 2 );
   s.read( (char*)(&version), sizeof(version) );
   
   if ( version == 2 )
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
      ErrLog( <<"Data in user database with unknown version " << version );
      ErrLog( <<"record size is " << data.size() );
   }
      
   return rec;
}


Data 
AbstractDb::getUserAuthInfo(  const AbstractDb::Key& key ) const
{ 
   AbstractDb::UserRecord rec = getUser(key);
   
   return rec.passwordHash;
}


AbstractDb::Key 
AbstractDb::firstUserKey()
{
   return dbFirstKey(UserTable);
}


AbstractDb::Key 
AbstractDb::nextUserKey()
{
   return dbNextKey(UserTable);
}
 

void 
AbstractDb::addRoute( const AbstractDb::Key& key, 
                 const AbstractDb::RouteRecord& rec )
{ 
   assert( !key.empty() );
   
   Data data;
   {
      oDataStream s(data);
      
      short version=1;
      assert( sizeof( version) == 2 );
      s.write( (char*)(&version) , sizeof(version) );
      
      encodeString( s, rec.mMethod );
      encodeString( s, rec.mEvent );
      encodeString( s, rec.mMatchingPattern );
      encodeString( s, rec.mRewriteExpression );
      s.write( (char*)(&rec.mOrder) , sizeof( rec.mOrder ) );
      assert( sizeof( rec.mOrder) == 2 );
  
      //!cj! TODO - add the extra local use only flag 

      s.flush();
   }
   
   dbWriteRecord( RouteTable, key, data );
}


void 
AbstractDb::eraseRoute(  const AbstractDb::Key& key )
{  
   dbEraseRecord (RouteTable, key);
}


void
AbstractDb::writeRoute( const Key& oldkey, const Key& newkey, const RouteRecord& rec )
{
   assert( !oldkey.empty() );
   assert( !newkey.empty() );
   
   Data data;
   {
      oDataStream s(data);
      
      short version=1;
      assert( sizeof( version) == 2 );
      s.write( (char*)(&version) , sizeof(version) );
      
      encodeString( s, rec.mMethod );
      encodeString( s, rec.mEvent );
      encodeString( s, rec.mMatchingPattern );
      encodeString( s, rec.mRewriteExpression );
      s.write( (char*)(&rec.mOrder) , sizeof( rec.mOrder ) );
      assert( sizeof( rec.mOrder) == 2 );
      s.flush();
   }
   
   if (oldkey != newkey)   // the domain or user (or both) changed, so the key has changed
   {
      dbEraseRecord( RouteTable, oldkey);
   }
   
   dbWriteRecord(RouteTable,newkey,data);
}

AbstractDb::RouteRecord 
AbstractDb::getRoute( const AbstractDb::Key& key) const
{ 
   AbstractDb::RouteRecord rec;
   Data data;
   bool stat = dbReadRecord( RouteTable, key, data );
   if ( !stat )
   {
      return rec;
   }
   if ( data.empty() )
   {
      return rec;
   }

   iDataStream s(data);

   short version;
   assert( sizeof(version) == 2 );
   s.read( (char*)(&version), sizeof(version) );
   
   if ( version == 1 )
   {
      rec.mMethod = decodeString( s );
      rec.mEvent  = decodeString( s );
      rec.mMatchingPattern = decodeString( s );
      rec.mRewriteExpression  = decodeString( s );
      s.read( (char*)(&rec.mOrder), sizeof(rec.mOrder) ); 
      assert( sizeof( rec.mOrder) == 2 );
   }
   else
   {
      // unkonwn version 
      ErrLog( <<"Data in route database with unknown version " << version );
      ErrLog( <<"record size is " << data.size() );
   }
      
   return rec;
}


AbstractDb::RouteRecordList 
AbstractDb::getAllRoutes()
{
   AbstractDb::RouteRecordList ret;
   
   AbstractDb::Key key = firstRouteKey();
   while ( !key.empty() )
   {
      AbstractDb::RouteRecord rec = getRoute(key);
      
      ret.push_back(rec);
            
      key = nextRouteKey();
   }
   
   return ret;
}


AbstractDb::Key 
AbstractDb::firstRouteKey()
{ 
   return dbFirstKey(RouteTable);
}


AbstractDb::Key 
AbstractDb::nextRouteKey()
{ 
   return dbNextKey(RouteTable);
}


void 
AbstractDb::addAcl( const AbstractDb::Key& key, 
                    const AbstractDb::AclRecord& rec )
{ 
   assert( !key.empty() );
   
   Data data;
   {
      oDataStream s(data);
      
      short version=1;
      assert( sizeof( version) == 2 );
      s.write( (char*)(&version) , sizeof(version) );

      encodeString( s, rec.mTlsPeerName );
      encodeString( s, rec.mAddress );
      s.write( (char*)(&rec.mMask) , sizeof( rec.mMask ) );
      s.write( (char*)(&rec.mPort) , sizeof( rec.mPort ) );
      s.write( (char*)(&rec.mFamily) , sizeof( rec.mFamily ) );
      s.write( (char*)(&rec.mTransport) , sizeof( rec.mTransport ) );

      s.flush();
   }
   
   dbWriteRecord( AclTable, key, data );
}


void 
AbstractDb::eraseAcl(  const AbstractDb::Key& key )
{  
   dbEraseRecord (AclTable, key);
}


AbstractDb::AclRecord 
AbstractDb::getAcl( const AbstractDb::Key& key) const
{ 
   AbstractDb::AclRecord rec;
   Data data;
   bool stat = dbReadRecord( AclTable, key, data );
   if ( !stat )
   {
      return rec;
   }
   if ( data.empty() )
   {
      return rec;
   }

   iDataStream s(data);

   short version;
   assert( sizeof(version) == 2 );
   s.read( (char*)(&version), sizeof(version) );
   
   if ( version == 1 )
   {
      rec.mTlsPeerName = decodeString( s );
      rec.mAddress = decodeString( s );
      s.read( (char*)(&rec.mMask), sizeof(rec.mMask) ); 
      s.read( (char*)(&rec.mPort), sizeof(rec.mPort) ); 
      s.read( (char*)(&rec.mFamily), sizeof(rec.mFamily) ); 
      s.read( (char*)(&rec.mTransport), sizeof(rec.mTransport) ); 
   }
   else
   {
      // unkonwn version 
      ErrLog( <<"Data in ACL database with unknown version " << version );
      ErrLog( <<"record size is " << data.size() );
   }
      
   return rec;
}


AbstractDb::AclRecordList 
AbstractDb::getAllAcls()
{
   AbstractDb::AclRecordList ret;
   
   AbstractDb::Key key = firstAclKey();
   while ( !key.empty() )
   {
      AbstractDb::AclRecord rec = getAcl(key);
      
      ret.push_back(rec);
            
      key = nextAclKey();
   }
   
   return ret;
}


AbstractDb::Key 
AbstractDb::firstAclKey()
{ 
   return dbFirstKey(AclTable);
}


AbstractDb::Key 
AbstractDb::nextAclKey()
{ 
   return dbNextKey(AclTable);
}




void 
AbstractDb::addConfig( const AbstractDb::Key& key, 
                 const AbstractDb::ConfigRecord& rec )
{ 
   assert( !key.empty() );
   
   Data data;
   {
      oDataStream s(data);
      
      short version=1;
      assert( sizeof( version) == 2 );
      s.write( (char*)(&version) , sizeof(version) );
      
      encodeString( s, rec.mDomain );
      s.write( (char*)(&rec.mTlsPort) , sizeof( rec.mTlsPort ) );
      assert( sizeof(rec.mTlsPort) == 2 );

      s.flush();
   }
   
   dbWriteRecord( ConfigTable, key, data );
}


void 
AbstractDb::eraseConfig(  const AbstractDb::Key& key )
{  
   dbEraseRecord (ConfigTable, key);
}


AbstractDb::ConfigRecord 
AbstractDb::getConfig( const AbstractDb::Key& key) const
{ 
   AbstractDb::ConfigRecord rec;
   Data data;
   bool stat = dbReadRecord( ConfigTable, key, data );
   if ( !stat )
   {
      return rec;
   }
   if ( data.empty() )
   {
      return rec;
   }

   iDataStream s(data);

   short version;
   assert( sizeof(version) == 2 );
   s.read( (char*)(&version), sizeof(version) );
   
   if ( version == 1 )
   {
      rec.mDomain = decodeString( s );   

      s.read( (char*)(&rec.mTlsPort), sizeof(rec.mTlsPort) ); 
      assert( sizeof( rec.mTlsPort) == 2 );
   }
   else
   {
      // unkonwn version 
      ErrLog( <<"Data in ACL database with unknown version " << version );
      ErrLog( <<"record size is " << data.size() );
   }
      
   return rec;
}


AbstractDb::ConfigRecordList 
AbstractDb::getAllConfigs()
{
   AbstractDb::ConfigRecordList ret;
   
   AbstractDb::Key key = firstConfigKey();
   while ( !key.empty() )
   {
      AbstractDb::ConfigRecord rec = getConfig(key);
      
      ret.push_back(rec);
            
      key = nextConfigKey();
   }
   
   return ret;
}


AbstractDb::Key 
AbstractDb::firstConfigKey()
{ 
   return dbFirstKey(ConfigTable);
}


AbstractDb::Key 
AbstractDb::nextConfigKey()
{ 
   return dbNextKey(ConfigTable);
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
