

#include <cassert>

#include "rutil/Data.hxx"
#include "rutil/MD5Stream.hxx"
#include "rutil/DataStream.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "resip/dum/UserAuthInfo.hxx"

#include "repro/UserStore.hxx"
#include "repro/AbstractDb.hxx"
#include "rutil/WinLeakCheck.hxx"


using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

UserStore::UserStore(AbstractDb& db ):
   mDb(db)
{ 
}

UserStore::~UserStore()
{ 
}


void 
UserStore::requestUserAuthInfo( const resip::Data& user, 
                                const resip::Data& realm,
                                const resip::Data& transactionToken,
                                resip::TransactionUser& transactionUser ) const
{
   // TODO - this should put a message on a local queue then a thread should
   // read that and then do the stuff in the rest of this fucntion
   
   resip::Data a1 = getUserAuthInfo(user, realm);

   UserAuthInfo* msg=0;

   if(a1.empty())
   {
      msg = new UserAuthInfo(user,realm,UserAuthInfo::UserUnknown,transactionToken);
   }
   else
   {
      msg = new UserAuthInfo(user,realm,a1,transactionToken);
   }

   transactionUser.post( msg );
}


AbstractDb::UserRecord
UserStore::getUserInfo( const Key& key ) const
{
   return mDb.getUser(key);
}


Data 
UserStore::getUserAuthInfo(  const resip::Data& user, 
                             const resip::Data& realm ) const
{
   Key key =  buildKey(user, realm);
   return mDb.getUserAuthInfo( key );
}


void 
UserStore::addUser( const Data& username,
                    const Data& domain,
                    const Data& realm,
                    const Data& password, 
                    bool  applyA1HashToPassword,
                    const Data& fullName, 
                    const Data& emailAddress )
{
   AbstractDb::UserRecord rec;
   rec.user = username;
   rec.domain = domain;
   rec.realm = realm;
   if(applyA1HashToPassword)
   {
      MD5Stream a1;
      a1 << username
         << Symbols::COLON
         << realm
         << Symbols::COLON
         << password;
      a1.flush();
      rec.passwordHash = a1.getHex();
   }
   else
   {
      rec.passwordHash = password;
   }
   rec.name = fullName;
   rec.email = emailAddress;
   rec.forwardAddress = Data::Empty;

   mDb.addUser( buildKey(username,domain), rec);
}


void 
UserStore::eraseUser( const Key& key )
{ 
   mDb.eraseUser( key );
}

void
UserStore::updateUser( const Key& originalKey, 
                       const resip::Data& user, 
                       const resip::Data& domain, 
                       const resip::Data& realm, 
                       const resip::Data& password, 
                       bool  applyA1HashToPassword,
                       const resip::Data& fullName,
                       const resip::Data& emailAddress )
{
   Key newkey = buildKey(user, domain);
   
   addUser( user,domain,realm,password,applyA1HashToPassword,fullName,emailAddress);
   if ( newkey != originalKey )
   {
      eraseUser(originalKey);
   }
}


UserStore::Key
UserStore::getFirstKey()
{
   return mDb.firstUserKey();
}


UserStore::Key
UserStore::getNextKey()
{
   return mDb.nextUserKey();
}


UserStore::Key
UserStore::buildKey( const resip::Data& user, 
                     const resip::Data& realm) const
{
   Data ret = user + Data("@") + realm;
   return ret;
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
