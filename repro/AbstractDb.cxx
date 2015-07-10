#include "rutil/ResipAssert.h"

#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/ParseBuffer.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/Logger.hxx"


#include "repro/UserStore.hxx"
#include "repro/AbstractDb.hxx"


using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

#define REPRO_DB_VERSION_USERS 3

static void 
encodeString(oDataStream& s, const Data& data)
{
   short len = (short)data.size();
   s.write( (char*)(&len) , sizeof( len ) );
   s.write( data.data(), len );
}


static void
decodeString(iDataStream& s, Data& data)
{
   data.clear();

   if(s.eof()) return;

   short len;
   s.read((char*)(&len), sizeof(len));
   if(s.eof()) return;

   // [TODO] This is probably OK for now, but we can do better than this.
   if (len > 8192)
   {
      ErrLog( << "Tried to decode a database record that was much larger (>8k) than expected.  Returning an empty Data instead." );
      return;
   }

   s.read(data.getBuf(len), len);
}


AbstractDb::AbstractDb()
{
}


AbstractDb::~AbstractDb()
{
}


Data 
AbstractDb::dbFirstKey(const AbstractDb::Table table)
{
   return dbNextKey(table,true/*first*/);
}


bool
AbstractDb::dbFirstRecord(const AbstractDb::Table table, 
                          const Data& key,
                          Data& data,
                          bool forUpdate)
{
   return dbNextRecord(table, key, data, forUpdate, true/*first*/);
}


// Callback used by BerkeleyDb for secondary table support.  Returns key to use in
// secondary database table
// secondaryKey should point to persistent data (ie. typically something from data itself)
int
AbstractDb::getSecondaryKey(const Table table, 
                            const Key& key, 
                            const Data& data, 
                            void** secondaryKey, 
                            unsigned int* secondaryKeyLen)
{
   if(table == SiloTable)
   {
      // Secondary Key for Silo table is DestUri
      Data nonConstData(Data::Share, data.data(), data.size());
      iDataStream s(nonConstData);

      short version;
      resip_assert(sizeof(version) == 2);
      s.read((char*)(&version), sizeof(version));
      resip_assert(version == 1);
      if (version == 1)
      {
         // DestUri is first element after version
         short len;
         s.read( (char*)(&len), sizeof(len));
         *secondaryKeyLen = (unsigned int)len;
         *secondaryKey = (void*)(nonConstData.data() + (sizeof(version)+sizeof(len)));
         return 0;
      }
   }
   return -1;
}


void 
AbstractDb::encodeUser(const UserRecord& rec, resip::Data& data)
{
   oDataStream s(data);
      
   short version = REPRO_DB_VERSION_USERS;
   resip_assert( sizeof( version) == 2 );
   s.write( (char*)(&version) , sizeof(version) );
      
   encodeString( s, rec.user );
   encodeString( s, rec.domain);
   encodeString( s, rec.realm);
   encodeString( s, rec.passwordHash);
   encodeString( s, rec.passwordHashAlt);
   encodeString( s, rec.name);
   encodeString( s, rec.email);
   encodeString( s, rec.forwardAddress);
   s.flush();
}

bool
AbstractDb::addUser( const AbstractDb::Key& key, const AbstractDb::UserRecord& rec )
{  
   resip_assert( !key.empty() );
   
   Data data;
   encodeUser(rec, data);
   
   return dbWriteRecord(UserTable,key,data);
}


void 
AbstractDb::eraseUser( const AbstractDb::Key& key )
{
   dbEraseRecord( UserTable, key);
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
   resip_assert( sizeof(version) == 2 );
   s.read( (char*)(&version), sizeof(version) );
   
   if ( version == REPRO_DB_VERSION_USERS )
   {
      decodeString(s, rec.user);
      decodeString(s, rec.domain);
      decodeString(s, rec.realm);
      decodeString(s, rec.passwordHash);
      decodeString(s, rec.passwordHashAlt);
      decodeString(s, rec.name);
      decodeString(s, rec.email);
      decodeString(s, rec.forwardAddress);
   }
   else if ( version == 2 )
   {
      // We can read from the older version DB, but the entry
      // will be written back in the new format with
      // passwordHashAlt blank
      decodeString(s, rec.user);
      decodeString(s, rec.domain);
      decodeString(s, rec.realm);
      decodeString(s, rec.passwordHash);
      decodeString(s, rec.name);
      decodeString(s, rec.email);
      decodeString(s, rec.forwardAddress);
      rec.passwordHashAlt = Data::Empty;
   }
   else
   {
      // unknown version 
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
AbstractDb::encodeRoute(const RouteRecord& rec, resip::Data& data)
{
   oDataStream s(data);
      
   short version=1;
   resip_assert( sizeof( version) == 2 );
   s.write( (char*)(&version) , sizeof(version) );
      
   encodeString( s, rec.mMethod );
   encodeString( s, rec.mEvent );
   encodeString( s, rec.mMatchingPattern );
   encodeString( s, rec.mRewriteExpression );
   s.write( (char*)(&rec.mOrder) , sizeof( rec.mOrder ) );
   resip_assert( sizeof( rec.mOrder) == 2 );
  
   //!cj! TODO - add the extra local use only flag 

   s.flush();
}


bool
AbstractDb::addRoute( const AbstractDb::Key& key, 
                 const AbstractDb::RouteRecord& rec )
{ 
   resip_assert( !key.empty() );
   
   Data data;
   encodeRoute(rec, data);
   
   return dbWriteRecord( RouteTable, key, data );
}


void 
AbstractDb::eraseRoute(  const AbstractDb::Key& key )
{  
   dbEraseRecord (RouteTable, key);
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
   resip_assert( sizeof(version) == 2 );
   s.read( (char*)(&version), sizeof(version) );
   
   if ( version == 1 )
   {
      decodeString(s, rec.mMethod);
      decodeString(s, rec.mEvent);
      decodeString(s, rec.mMatchingPattern);
      decodeString(s, rec.mRewriteExpression);
      s.read( (char*)(&rec.mOrder), sizeof(rec.mOrder) ); 
      resip_assert( sizeof( rec.mOrder) == 2 );
   }
   else
   {
      // unknown version 
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


bool
AbstractDb::addAcl( const AbstractDb::Key& key, 
                    const AbstractDb::AclRecord& rec )
{ 
   resip_assert( !key.empty() );
   
   Data data;
   {
      oDataStream s(data);
      
      short version=1;
      resip_assert( sizeof( version) == 2 );
      s.write( (char*)(&version) , sizeof(version) );

      encodeString( s, rec.mTlsPeerName );
      encodeString( s, rec.mAddress );
      s.write( (char*)(&rec.mMask) , sizeof( rec.mMask ) );
      s.write( (char*)(&rec.mPort) , sizeof( rec.mPort ) );
      s.write( (char*)(&rec.mFamily) , sizeof( rec.mFamily ) );
      s.write( (char*)(&rec.mTransport) , sizeof( rec.mTransport ) );

      s.flush();
   }
   
   return dbWriteRecord( AclTable, key, data );
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
   resip_assert( sizeof(version) == 2 );
   s.read( (char*)(&version), sizeof(version) );
   
   if ( version == 1 )
   {
      decodeString(s, rec.mTlsPeerName);
      decodeString(s, rec.mAddress);
      s.read( (char*)(&rec.mMask), sizeof(rec.mMask) ); 
      s.read( (char*)(&rec.mPort), sizeof(rec.mPort) ); 
      s.read( (char*)(&rec.mFamily), sizeof(rec.mFamily) ); 
      s.read( (char*)(&rec.mTransport), sizeof(rec.mTransport) ); 
   }
   else
   {
      // unknown version 
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


bool
AbstractDb::addConfig( const AbstractDb::Key& key, 
                 const AbstractDb::ConfigRecord& rec )
{ 
   resip_assert( !key.empty() );
   
   Data data;
   {
      oDataStream s(data);
      
      short version=1;
      resip_assert( sizeof( version) == 2 );
      s.write( (char*)(&version) , sizeof(version) );
      
      encodeString( s, rec.mDomain );
      s.write( (char*)(&rec.mTlsPort) , sizeof( rec.mTlsPort ) );
      resip_assert( sizeof(rec.mTlsPort) == 2 );

      s.flush();
   }
   
   return dbWriteRecord( ConfigTable, key, data );
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
   resip_assert( sizeof(version) == 2 );
   s.read( (char*)(&version), sizeof(version) );
   
   if ( version == 1 )
   {
      decodeString(s, rec.mDomain);

      s.read( (char*)(&rec.mTlsPort), sizeof(rec.mTlsPort) ); 
      resip_assert( sizeof( rec.mTlsPort) == 2 );
   }
   else
   {
      // unknown version 
      ErrLog( <<"Data in Config database with unknown version " << version );
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


bool
AbstractDb::addStaticReg( const AbstractDb::Key& key, 
                          const AbstractDb::StaticRegRecord& rec )
{ 
   resip_assert( !key.empty() );
   
   Data data;
   {
      oDataStream s(data);
      
      short version=1;
      resip_assert( sizeof( version) == 2 );
      s.write( (char*)(&version) , sizeof(version) );
      
      encodeString( s, rec.mAor );
      encodeString( s, rec.mContact );
      encodeString( s, rec.mPath );

      s.flush();
   }
   
   return dbWriteRecord( StaticRegTable, key, data );
}


void 
AbstractDb::eraseStaticReg(  const AbstractDb::Key& key )
{  
   dbEraseRecord (StaticRegTable, key);
}


AbstractDb::StaticRegRecord 
AbstractDb::getStaticReg( const AbstractDb::Key& key) const
{ 
   AbstractDb::StaticRegRecord rec;
   Data data;
   bool stat = dbReadRecord( StaticRegTable, key, data );
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
   resip_assert( sizeof(version) == 2 );
   s.read( (char*)(&version), sizeof(version) );
   
   if ( version == 1 )
   {
      decodeString(s, rec.mAor);
      decodeString(s, rec.mContact);
      decodeString(s, rec.mPath);
   }
   else
   {
      // unknown version 
      ErrLog( <<"Data in StaticReg database with unknown version " << version );
      ErrLog( <<"record size is " << data.size() );
   }
      
   return rec;
}


AbstractDb::StaticRegRecordList 
AbstractDb::getAllStaticRegs()
{
   AbstractDb::StaticRegRecordList ret;
   
   AbstractDb::Key key = firstStaticRegKey();
   while ( !key.empty() )
   {
      AbstractDb::StaticRegRecord rec = getStaticReg(key);
      
      ret.push_back(rec);
            
      key = nextStaticRegKey();
   }
   
   return ret;
}


AbstractDb::Key 
AbstractDb::firstStaticRegKey()
{ 
   return dbFirstKey(StaticRegTable);
}


AbstractDb::Key 
AbstractDb::nextStaticRegKey()
{ 
   return dbNextKey(StaticRegTable);
}


void 
AbstractDb::encodeFilter(const FilterRecord& rec, resip::Data& data)
{
   oDataStream s(data);

   short version=1;
   resip_assert(sizeof( version) == 2);
   s.write((char*)(&version) , sizeof(version));
      
   encodeString(s, rec.mCondition1Header);
   encodeString(s, rec.mCondition1Regex);
   encodeString(s, rec.mCondition2Header);
   encodeString(s, rec.mCondition2Regex);
   encodeString(s, rec.mMethod);
   encodeString(s, rec.mEvent);
   s.write((char*)(&rec.mAction), sizeof (rec.mAction));
   resip_assert(sizeof(rec.mAction) == 2);
   encodeString(s, rec.mActionData);
   s.write((char*)(&rec.mOrder), sizeof(rec.mOrder));
   resip_assert(sizeof(rec.mOrder) == 2);

   s.flush();
}


bool
AbstractDb::addFilter(const AbstractDb::Key& key, 
                      const AbstractDb::FilterRecord& rec)
{ 
   resip_assert( !key.empty() );
   
   Data data;
   encodeFilter(rec, data);
   return dbWriteRecord(FilterTable, key, data);
}


void 
AbstractDb::eraseFilter(const AbstractDb::Key& key)
{  
   dbEraseRecord(FilterTable, key);
}


AbstractDb::FilterRecord 
AbstractDb::getFilter( const AbstractDb::Key& key) const
{ 
   AbstractDb::FilterRecord rec;
   Data data;
   bool stat = dbReadRecord(FilterTable, key, data);
   if (!stat)
   {
      return rec;
   }
   if (data.empty())
   {
      return rec;
   }

   iDataStream s(data);

   short version;
   resip_assert(sizeof(version) == 2);
   s.read((char*)(&version), sizeof(version));
   
   if (version == 1)
   {
      decodeString(s, rec.mCondition1Header);
      decodeString(s, rec.mCondition1Regex);
      decodeString(s, rec.mCondition2Header);
      decodeString(s, rec.mCondition2Regex);
      decodeString(s, rec.mMethod);
      decodeString(s, rec.mEvent);
      s.read((char*)(&rec.mAction), sizeof(rec.mAction)); 
      resip_assert(sizeof(rec.mAction) == 2);
      decodeString(s, rec.mActionData);
      s.read((char*)(&rec.mOrder), sizeof(rec.mOrder)); 
      resip_assert(sizeof(rec.mOrder) == 2);
   }
   else
   {
      // unknown version 
      ErrLog( <<"Data in filter database with unknown version " << version );
      ErrLog( <<"record size is " << data.size() );
   }
      
   return rec;
}


AbstractDb::FilterRecordList 
AbstractDb::getAllFilters()
{
   AbstractDb::FilterRecordList ret;
   
   AbstractDb::Key key = firstFilterKey();
   while ( !key.empty() )
   {
      AbstractDb::FilterRecord rec = getFilter(key);
      
      ret.push_back(rec);
            
      key = nextFilterKey();
   }
   
   return ret;
}


AbstractDb::Key 
AbstractDb::firstFilterKey()
{ 
   return dbFirstKey(FilterTable);
}


AbstractDb::Key 
AbstractDb::nextFilterKey()
{ 
   return dbNextKey(FilterTable);
}

bool
AbstractDb::addToSilo(const Key& key, const SiloRecord& rec)
{
   resip_assert( !key.empty() );
   
   Data data;
   {
      oDataStream s(data);

      short version=1;
      resip_assert(sizeof( version) == 2);
      s.write((char*)(&version) , sizeof(version));

      encodeString(s, rec.mDestUri);
      encodeString(s, rec.mSourceUri);
      s.write((char*)(&rec.mOriginalSentTime), sizeof (rec.mOriginalSentTime));
      resip_assert(sizeof(rec.mOriginalSentTime) == 8);
      encodeString(s, rec.mTid);
      encodeString(s, rec.mMimeType);
      encodeString(s, rec.mMessageBody);

      s.flush();
   }
   return dbWriteRecord(SiloTable, key, data);
}

void
AbstractDb::decodeSiloRecord(Data& data, SiloRecord& rec)
{
   iDataStream s(data);

   short version;
   resip_assert(sizeof(version) == 2);
   s.read((char*)(&version), sizeof(version));
   
   if (version == 1)
   {
      decodeString(s, rec.mDestUri);
      decodeString(s, rec.mSourceUri);
      s.read((char*)(&rec.mOriginalSentTime), sizeof(rec.mOriginalSentTime)); 
      resip_assert(sizeof(rec.mOriginalSentTime) == 8);
      decodeString(s, rec.mTid);
      decodeString(s, rec.mMimeType);
      decodeString(s, rec.mMessageBody);
   }
   else
   {
      // unknown version 
      ErrLog( <<"Data in silo database with unknown version " << version );
      ErrLog( <<"record size is " << data.size() );
   }
}

bool
AbstractDb::getSiloRecords(const Key& skey, AbstractDb::SiloRecordList& recordList)
{
   AbstractDb::SiloRecord rec;

   Data data;
   bool moreRecords = dbFirstRecord(SiloTable, skey, data, false /* forUpdate? */);
   if(moreRecords)
   {
      // Decode and store data
      decodeSiloRecord(data,rec);
      recordList.push_back(rec);
      while((moreRecords = dbNextRecord(SiloTable, skey, data, false /* forUpdate? */)))
      {
         // Decode and store data
         decodeSiloRecord(data,rec);
         recordList.push_back(rec);
      }
   }

   return true;
}

void 
AbstractDb::eraseSiloRecord(const Key& key)
{
   dbEraseRecord(SiloTable, key);
}

void 
AbstractDb::cleanupExpiredSiloRecords(UInt64 now, unsigned long expirationTime)
{
   AbstractDb::Key key = dbFirstKey(SiloTable);  // Iterate on primary key
   // Iterate through all silo records - retrieve Original send time embedded into the 
   // primary key and see if the record has expired.
   Data originalSendTimeData;
   UInt64 originalSendTime;
   while(!key.empty())
   {
      ParseBuffer pb(key);
      const char* anchor = pb.position();
      pb.skipToChar(':');
      pb.data(originalSendTimeData, anchor);
      originalSendTime = originalSendTimeData.convertUInt64();
      if((unsigned long)(now - originalSendTime) > expirationTime)
      {
         eraseSiloRecord(key);
      }
      key = dbNextKey(SiloTable);
   }
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
