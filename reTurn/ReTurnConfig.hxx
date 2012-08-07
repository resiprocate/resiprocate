#if !defined(RETURN_CONFIG_HXX)
#define RETURN_CONFIG_HXX 

#include <map>
#include <asio.hpp>
#include <rutil/ConfigParse.hxx>
#include <rutil/Data.hxx>
#include <rutil/Log.hxx>

#include <reTurn/UserAuthData.hxx>

namespace reTurn {

typedef std::map<resip::Data,reTurn::UserAuthData> RealmUsers;

class ReTurnConfig : public resip::ConfigParse
{
public:
   ReTurnConfig();
   virtual ~ReTurnConfig();

   virtual void parseConfig(int argc, char** argv, const resip::Data& defaultConfigFilename);

   void printHelpText(int argc, char **argv);
   using resip::ConfigParse::getConfigValue;

   typedef enum
   {
      NoAuthentication = 0,
      ShortTermPassword = 1,
      LongTermPassword = 2
   } AuthenticationMode;

   unsigned short mTurnPort;
   unsigned short mTlsTurnPort;
   unsigned short mAltStunPort;  
   asio::ip::address mTurnAddress;
   asio::ip::address mAltStunAddress;

   AuthenticationMode mAuthenticationMode;
   resip::Data mAuthenticationRealm;
   std::map<resip::Data,resip::Data> mAuthenticationCredentials;
   std::map<resip::Data,RealmUsers> mUsers;
   unsigned long mNonceLifetime;

   unsigned short mAllocationPortRangeMin;
   unsigned short mAllocationPortRangeMax;
   unsigned long mDefaultAllocationLifetime;
   unsigned long mMaxAllocationLifetime;
   unsigned long mMaxAllocationsPerUser;  // TODO - enforcement needs to be implemented

   resip::Data mTlsServerCertificateFilename;
   resip::Data mTlsTempDhFilename;
   resip::Data mTlsPrivateKeyPassword;

   resip::Data mLoggingType;
   resip::Data mLoggingLevel;
   resip::Data mLoggingFilename;
   unsigned int mLoggingFileMaxLineCount;
   bool mDaemonize;
   resip::Data mPidFile;

   bool isUserNameValid(const resip::Data& username) const;
   const resip::Data& getPasswordForUsername(const resip::Data& username) const;
   const UserAuthData* getUser(const resip::Data& userName, const resip::Data& realm) const;

protected:
   void calcUserAuthData();
};

} // namespace

#endif


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
