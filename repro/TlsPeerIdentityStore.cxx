

#include "rutil/ResipAssert.h"

#include "rutil/Data.hxx"
#include "rutil/MD5Stream.hxx"
#include "rutil/DataStream.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "resip/dum/UserAuthInfo.hxx"

#include "repro/TlsPeerIdentityStore.hxx"
#include "repro/AbstractDb.hxx"
#include "rutil/WinLeakCheck.hxx"


using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

const resip::Data TlsPeerIdentityStore::SEPARATOR(",");

TlsPeerIdentityStore::TlsPeerIdentityStore(AbstractDb& db ) : mDb(db)
{ 
}

TlsPeerIdentityStore::~TlsPeerIdentityStore()
{ 
}

AbstractDb::TlsPeerIdentityRecord
TlsPeerIdentityStore::getTlsPeerIdentityInfo( const Key& key ) const
{
   return mDb.getTlsPeerIdentity(key);
}

bool
TlsPeerIdentityStore::isAuthorized(  const std::set<resip::Data>& peerNames,
                             const std::set<resip::Data>& identities ) const
{
   return mDb.isAuthorized( peerNames, identities );
}

bool 
TlsPeerIdentityStore::addTlsPeerIdentity( const Data& peerName,
                    const Data& authorizedIdentity)
{
   AbstractDb::TlsPeerIdentityRecord rec;
   rec.peerName = peerName;
   rec.authorizedIdentity = authorizedIdentity;

   return mDb.addTlsPeerIdentity( buildKey(peerName, authorizedIdentity), rec);
}

void 
TlsPeerIdentityStore::eraseTlsPeerIdentity( const Key& key )
{ 
   mDb.eraseTlsPeerIdentity( key );
}

bool
TlsPeerIdentityStore::updateTlsPeerIdentity( const Key& originalKey, 
                       const resip::Data& peerName,
                       const resip::Data& authorizedIdentity)
{
   Key newkey = buildKey(peerName, authorizedIdentity);
   
   bool ret = addTlsPeerIdentity(peerName, authorizedIdentity);
   if ( newkey != originalKey )
   {
      eraseTlsPeerIdentity(originalKey);
   }
   return ret;
}

TlsPeerIdentityStore::Key
TlsPeerIdentityStore::getFirstKey()
{
   return mDb.firstTlsPeerIdentityKey();
}

TlsPeerIdentityStore::Key
TlsPeerIdentityStore::getNextKey()
{
   return mDb.nextTlsPeerIdentityKey();
}

TlsPeerIdentityStore::Key
TlsPeerIdentityStore::buildKey( const resip::Data& peerName, const resip::Data& authorizedIdentity)
{
   Data ret = peerName + Data(SEPARATOR) + authorizedIdentity;
   return ret;
}

void
TlsPeerIdentityStore::getTlsPeerIdentityFromKey(const Key& key, Data& peerName, Data& authorizedIdentity)
{
   ParseBuffer pb(key);
   const char* start = pb.position();
   pb.skipToOneOf(SEPARATOR);
   pb.data(peerName, start);
   const char* anchor = pb.skipChar();
   pb.skipToEnd();
   pb.data(authorizedIdentity, anchor);
}

/* ====================================================================
 *
 * Copyright 2014 Daniel Pocock http://danielpocock.com  All rights reserved.
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
 *
 *
 */

