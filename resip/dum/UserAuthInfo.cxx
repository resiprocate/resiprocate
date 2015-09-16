
#include "rutil/ResipAssert.h"

#include "resip/dum/UserAuthInfo.hxx"
#include "rutil/Data.hxx"
#include "rutil/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

UserAuthInfo::UserAuthInfo(const Data& user,
                           const Data& realm,
                           InfoMode mode,
                           const Data& transactionId):
   DumFeatureMessage(transactionId),
   mMode(mode),
   mUser(user),
   mRealm(realm)
{
}

UserAuthInfo::UserAuthInfo(const Data& user, 
                           const Data& realm, 
                           const Data& a1, 
                           const Data& transactionId):
   DumFeatureMessage(transactionId),
   mMode(RetrievedA1),
   mUser(user),
   mRealm(realm),
   mA1(a1)
{
}

UserAuthInfo::UserAuthInfo( const resip::Data& user,
                            const resip::Data& realm,
                            const resip::Data& transactionId,
                            resip::TransactionUser* transactionUser):
   DumFeatureMessage(transactionId),
   mMode(RetrievedA1),
   mUser(user),
   mRealm(realm)
{
   mTu = transactionUser;
}

UserAuthInfo::~UserAuthInfo()
{
}

UserAuthInfo::InfoMode
UserAuthInfo::getMode() const
{
   return mMode;
}

const Data&
UserAuthInfo::getA1() const
{
   return mA1;
}

const Data& 
UserAuthInfo::getRealm() const
{
   return mRealm;
}

const Data&
UserAuthInfo::getUser() const
{
   return mUser;
}

void 
UserAuthInfo::setMode(InfoMode mode)
{
   mMode = mode;
}
      
void 
UserAuthInfo::setA1(const resip::Data& a1)
{
   mA1 = a1;
}

Data 
UserAuthInfo::brief() const
{  
   Data buffer;
   DataStream strm(buffer);
   strm << "UserAuthInfo " << mUser << " @ " << mRealm << " A1=" << mA1;
   strm.flush();
   return buffer;
}

resip::Message* 
UserAuthInfo::clone() const
{
   resip_assert(false); return NULL;
}

EncodeStream& 
UserAuthInfo::encode(EncodeStream& strm) const
{
   strm << brief();
   return strm;
}

EncodeStream&
UserAuthInfo::encodeBrief(EncodeStream& strm) const
{
   return encode(strm);
}

EncodeStream& 
resip::operator<<(EncodeStream& strm, const UserAuthInfo& msg)
{
   return msg.encode(strm);
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
