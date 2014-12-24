
#include "resip/dum/Profile.hxx"
#include "resip/dum/UserProfile.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/MD5Stream.hxx"

using namespace resip;
#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

const resip::NameAddr UserProfile::mAnonymous("\"Anonymous\" <sip:anonymous@anonymous.invalid>", true /* preCacheAor */);

UserProfile::UserProfile() : Profile(), 
   mGruuEnabled(false),
   mRegId(0),
   mClientOutboundEnabled(false),
   mDigestCacheUseLimit(0)
{
    //InfoLog (<< "************ UserProfile created (no base)!: " << *this);
}

UserProfile::UserProfile(SharedPtr<Profile> baseProfile) : Profile(baseProfile), 
   mGruuEnabled(false),
   mRegId(0),
   mClientOutboundEnabled(false),
   mDigestCacheUseLimit(0)
{
    //InfoLog (<< "************ UserProfile created (with base)!: " << *this);
}

UserProfile::~UserProfile()
{
    //InfoLog (<< "************ UserProfile destroyed!: " << *this);
}

SharedPtr<UserProfile> 
UserProfile::getAnonymousUserProfile() const
{
   SharedPtr<UserProfile> anon(this->clone());
   anon->setDefaultFrom(mAnonymous);
   return anon;
}

UserProfile* 
UserProfile::clone() const
{
   return new UserProfile(*this);
}

bool
UserProfile::isAnonymous() const
{
   return (mDefaultFrom.uri().getAor() == mAnonymous.uri().getAor());
}

void
UserProfile::setDefaultFrom(const NameAddr& from)
{
   mDefaultFrom = from;
}

NameAddr& 
UserProfile::getDefaultFrom()
{
   return mDefaultFrom;
}

void
UserProfile::setServiceRoute(const NameAddrs& sRoute)
{
   mServiceRoute = sRoute;
}

NameAddrs& 
UserProfile::getServiceRoute()
{
   return mServiceRoute;
}

bool
UserProfile::hasInstanceId()
{
   return !mInstanceId.empty();
}

void
UserProfile::setInstanceId(const Data& id)
{
   mInstanceId = id;
}

const Data&
UserProfile::getInstanceId() const
{
   return mInstanceId;
}

void 
UserProfile::addGruu(const Data& aor, const NameAddr& contact)
{
}

bool 
UserProfile::hasGruu(const Data& aor) const
{
   return false;
}

bool 
UserProfile::hasGruu(const Data& aor, const Data& instance) const
{
   return false;
}

NameAddr&
UserProfile:: getGruu(const Data& aor)
{
   resip_assert(0);
   static NameAddr gruu;
   return gruu;
}

NameAddr&
UserProfile:: getGruu(const Data& aor, const NameAddr& contact)
{
   resip_assert(0);
   static NameAddr gruu;
   return gruu;
}

void 
UserProfile::clearDigestCredentials()
{
   mDigestCredentials.clear();
}

void 
UserProfile::setDigestCredential( const Data& realm, const Data& user, const Data& password, bool isPasswordA1Hash)
{
   DigestCredential cred(realm, user, password, isPasswordA1Hash);

   DebugLog (<< "Adding credential: " << cred);
   mDigestCredentials.erase(cred);
   mDigestCredentials.insert(cred);
}
     
static const UserProfile::DigestCredential emptyDigestCredential;
const UserProfile::DigestCredential&
UserProfile::getDigestCredential( const Data& realm  )
{
   if(mDigestCredentials.empty())
   {
      // !jf! why not just throw here? 
      return emptyDigestCredential;
   }

   DigestCredentials::const_iterator it = mDigestCredentials.find(DigestCredential(realm));
   if (it == mDigestCredentials.end())
   {
      DebugLog(<< "Didn't find credential for realm: " << realm << " " << *mDigestCredentials.begin());
      return *mDigestCredentials.begin();
   }
   else      
   {
      DebugLog(<< "Found credential for realm: " << *it << realm);      
      return *it;
   }
}

UserProfile::DigestCredential::DigestCredential(const Data& r, const Data& u, const Data& pwd, bool pwdA1Hash) :
   realm(r),
   user(u),
   password(pwd),
   isPasswordA1Hash(pwdA1Hash)
{  
}

UserProfile::DigestCredential::DigestCredential() : 
   realm(Data::Empty),
   user(Data::Empty),
   password(Data::Empty),
   isPasswordA1Hash(false)
{
}  

UserProfile::DigestCredential::DigestCredential(const Data& pRealm) : 
   realm(pRealm),
   user(Data::Empty),
   password(Data::Empty),
   isPasswordA1Hash(false)
{
}  

bool
UserProfile::DigestCredential::operator<(const DigestCredential& rhs) const
{
   return realm < rhs.realm;
}

EncodeStream&
resip::operator<<(EncodeStream& strm, const UserProfile& profile)
{
   strm << "UserProfile: " << profile.mDefaultFrom << Inserter(profile.mDigestCredentials);
   return strm;
}

EncodeStream&
resip::operator<<(EncodeStream& strm, const UserProfile::DigestCredential& cred)
{
   strm << "realm=" << cred.realm << " user=" << cred.user ;
   return strm;
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
