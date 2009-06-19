#include "ReTurnConfig.hxx"

#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;
using namespace resip;

namespace reTurn {

ReTurnConfig::ReTurnConfig() :
   mTurnPort(3478),
   mTlsTurnPort(5349),
   mAltStunPort(0), // Note:  The default is to disable RFC3489 binding support
   mTurnAddress(asio::ip::address::from_string("0.0.0.0")),
   mAltStunAddress(asio::ip::address::from_string("0.0.0.0")),
   mAuthenticationMode(LongTermPassword),  // required for TURN
   mAuthenticationRealm("reTurn"),
   mNonceLifetime(3600),            // 1 hour - at least 1 hours is recommended by the draft
   mAllocationPortRangeMin(49152),  // must be even - This default range is the Dynamic and/or Private Port range - recommended by RFC
   mAllocationPortRangeMax(65535),  // must be odd
   mDefaultAllocationLifetime(600), // 10 minutes
   mMaxAllocationLifetime(3600),    // 1 hour
   mMaxAllocationsPerUser(0),       // 0 - no max
   mTlsServerCertificateFilename("server.pem"),
   mTlsTempDhFilename("dh512.pem"),
   mTlsPrivateKeyPassword("password"),
   mLoggingType(resip::Log::Cout),
   mLoggingLevel(resip::Log::Info),
   mLoggingFilename("reTurnServer.log"),
   mLoggingFileMaxLineCount(50000)  // 50000 about 5M size
{
   mAuthenticationCredentials["test"] = "1234";
}

bool 
ReTurnConfig::isUserNameValid(const resip::Data& username) const
{
   std::map<resip::Data,resip::Data>::const_iterator it = mAuthenticationCredentials.find(username);
   return it != mAuthenticationCredentials.end();
}

const Data& 
ReTurnConfig::getPasswordForUsername(const Data& username) const
{
   std::map<resip::Data,resip::Data>::const_iterator it = mAuthenticationCredentials.find(username);
   if(it != mAuthenticationCredentials.end())
   {
      return it->second;
   }
   else
   {
      return Data::Empty;
   }
}


} // namespace


/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
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
