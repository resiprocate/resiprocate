
#include "rutil/ResipAssert.h"

#include "apps/reConServer/CredentialInfo.hxx"
#include "apps/reConServer/B2BCallManager.hxx"
#include "rutil/Data.hxx"
#include "rutil/Logger.hxx"

using namespace reconserver;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

CredentialInfo::CredentialInfo(resip::SharedPtr<B2BCall> call, const resip::Data& user, const resip::Data& realm, const Data& transactionId, resip::TransactionUser* transactionUser, B2BCallManager* b) :
   mCall(call),
   mUser(user),
   mRealm(realm),
   mMode(Error)
{
   mTu = transactionUser;
   mB2BCallManager = b;
}

Data
CredentialInfo::brief() const
{
   Data buffer;
   DataStream strm(buffer);
   strm << "CredentialInfo " << mUser << " @ " << mRealm << " mSecret=" << mSecret;
   strm.flush();
   return buffer;
}

void
CredentialInfo::executeCommand()
{
   DebugLog(<<"invoked on DUM thread");
   mB2BCallManager->makeBLeg(call(), this);
}

resip::Message*
CredentialInfo::clone() const
{
   resip_assert(false); return NULL;
}

EncodeStream&
CredentialInfo::encode(EncodeStream& strm) const
{
   strm << brief();
   return strm;
}

EncodeStream&
CredentialInfo::encodeBrief(EncodeStream& strm) const
{
   return encode(strm);
}

EncodeStream&
reconserver::operator<<(EncodeStream& strm, const CredentialInfo& msg)
{
   return msg.encode(strm);
}


/* ====================================================================
 *
 * Copyright 2017 Daniel Pocock http://danielpocock.com  All rights reserved.
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
 *
 */

