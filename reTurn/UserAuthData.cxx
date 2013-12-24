#include "UserAuthData.hxx"

#include <rutil/ConfigParse.hxx>
#include <rutil/MD5Stream.hxx>

#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;
using namespace resip;

namespace reTurn {

UserAuthData::UserAuthData(const resip::Data& userName, const resip::Data& realm, const resip::Data& ha1) :
   mUserName(userName),
   mRealm(realm),
   mHa1(ha1)
{
}

UserAuthData::~UserAuthData()
{
}

UserAuthData
UserAuthData::createFromPassword(const resip::Data& userName, const resip::Data& realm, const resip::Data& password)
{
   MD5Stream r;
   r << userName << ":" << realm << ":" << password; 
   Data ha1(r.getBin());
   return UserAuthData(userName, realm, ha1);
}

UserAuthData
UserAuthData::createFromHex(const resip::Data& userName, const resip::Data& realm, const resip::Data& ha1Hex)
{
   if(ha1Hex.size() != 32)
   {
      throw ConfigParse::Exception("H(A1) password field must contain 32 character hex string", __FILE__, __LINE__);
   }
   return UserAuthData(userName, realm, ha1Hex.fromHex());
}

} // namespace


/* ====================================================================

 Copyright (c) 2012, Ready Technology (UK) Limited
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Ready Technology nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
