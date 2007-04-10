
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/UserProfile.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Inserter.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/os/MD5Stream.hxx"

using namespace resip;
#define RESIPROCATE_SUBSYSTEM Subsystem::DUM


UserProfile::UserProfile(Profile *baseProfile) : Profile(baseProfile)
{
}

void
UserProfile::setDefaultFrom(const NameAddr& from)
{
   mDefaultFrom = from;
}

const NameAddr& 
UserProfile::getDefaultFrom() const
{
   return mDefaultFrom;
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
   assert(0);
   static NameAddr gruu;
   return gruu;
}

NameAddr&
UserProfile:: getGruu(const Data& aor, const NameAddr& contact)
{
   assert(0);
   static NameAddr gruu;
   return gruu;
}

void 
UserProfile::disableGruu()
{
   assert(0);
}

void 
UserProfile::clearDigestCredentials()
{
   mDigestCredentials.clear();
}

void 
UserProfile::setDigestCredential( const Data& realm, const Data& user, const Data& password)
{
   DigestCredential cred( realm, user, password );

   DebugLog (<< "Adding credential: " << cred);
   mDigestCredentials.erase(cred);
   mDigestCredentials.insert(cred);
}
     
const UserProfile::DigestCredential&
UserProfile::getDigestCredential( const Data& realm  )
{
   if(mDigestCredentials.empty())
   {
      // !jf! why not just throw here? 
      static const DigestCredential empty;
      return empty;
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

UserProfile::DigestCredential::DigestCredential(const Data& r, const Data& u, const Data& pwd) :
   realm(r),
   user(u),
   password(pwd)
{  
//    MD5Stream a1;
//    a1 << user
//       << Symbols::COLON
//       << realm
//       << Symbols::COLON
//       << password;
//    passwordHashA1 = a1.getHex();
}

UserProfile::DigestCredential::DigestCredential() : 
   realm(Data::Empty),
   user(Data::Empty),
   password(Data::Empty)
{
}  

UserProfile::DigestCredential::DigestCredential(const Data& pRealm) : 
   realm(pRealm),
   user(Data::Empty),
   password(Data::Empty) 
{
}  

bool
UserProfile::DigestCredential::operator<(const DigestCredential& rhs) const
{
   return realm < rhs.realm;
}

std::ostream&
resip::operator<<(std::ostream& strm, const UserProfile& profile)
{
   strm << "UserProfile: " << profile.mDefaultFrom << Inserter(profile.mDigestCredentials);
   return strm;
}

std::ostream&
resip::operator<<(std::ostream& strm, const UserProfile::DigestCredential& cred)
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
