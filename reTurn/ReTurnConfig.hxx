#if !defined(RETURN_CONFIG_HXX)
#define RETURN_CONFIG_HXX 

#include <map>
#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif
#include <rutil/ConfigParse.hxx>
#include <rutil/Data.hxx>
#include <rutil/Log.hxx>
#include <rutil/BaseException.hxx>
#include <rutil/RWMutex.hxx>
#include <rutil/Lock.hxx>

#include <reTurn/UserAuthData.hxx>

namespace reTurn {

typedef std::map<resip::Data,reTurn::UserAuthData> RealmUsers;
typedef std::pair<resip::Data, resip::Data> RealmUserPair;

class ReTurnConfig : public resip::ConfigParse
{
public:
   class Exception : public resip::BaseException
   {
      public:
         Exception(const resip::Data& msg,
                   const resip::Data& file,
                   const int line)
            : resip::BaseException(msg, file, line) {}            
      protected:
         virtual const char* name() const { return "ReTurnConfig::Exception"; }
   };
   
   ReTurnConfig();
   virtual ~ReTurnConfig();

   virtual void parseConfig(int argc, char** argv, const resip::Data& defaultConfigFilename);

   void printHelpText(int argc, char **argv);
   using resip::ConfigParse::getConfigValue;
   
   typedef enum
   {
      AUTHORIZED,
      RESTRICTED,
      REFUSED 
   } AccountState;

   resip::Data mSoftwareName;
   bool mPadSoftwareName;

   unsigned short mTurnPort;
   unsigned short mTlsTurnPort;
   unsigned short mAltStunPort;  
   asio::ip::address mTurnAddress;
   asio::ip::address mTurnV6Address;
   asio::ip::address mAltStunAddress;

   resip::Data mAuthenticationRealm;
   int mUserDatabaseCheckInterval;
   mutable resip::RWMutex mUserDataMutex;
   unsigned long mNonceLifetime;

   unsigned short mAllocationPortRangeMin;
   unsigned short mAllocationPortRangeMax;
   unsigned long mDefaultAllocationLifetime;
   unsigned long mMaxAllocationLifetime;
   unsigned long mMaxAllocationsPerUser;  // TODO - enforcement needs to be implemented

   resip::Data mTlsServerCertificateFilename;
   resip::Data mTlsServerPrivateKeyFilename;
   resip::Data mTlsTempDhFilename;
   resip::Data mTlsPrivateKeyPassword;

   resip::Data mUsersDatabaseFilename;
   bool mUserDatabaseHashedPasswords;

   resip::Data mLoggingType;
   resip::Data mSyslogFacility;
   resip::Data mLoggingLevel;
   resip::Data mLoggingFilename;
   unsigned int mLoggingFileMaxLineCount;
   bool mDaemonize;
   resip::Data mPidFile;
   resip::Data mRunAsUser;
   resip::Data mRunAsGroup;

   bool isUserNameValid(const resip::Data& username,  const resip::Data& realm) const;
   resip::Data getHa1ForUsername(const resip::Data& username, const resip::Data& realm) const;
   std::auto_ptr<UserAuthData> getUser(const resip::Data& userName, const resip::Data& realm) const;
   void addUser(const resip::Data& username, const resip::Data& password, const resip::Data& realm);
   void authParse(const resip::Data& accountDatabaseFilename);

private:
   std::map<resip::Data,RealmUsers> mUsers;
   std::map<RealmUserPair, resip::Data> mRealmUsersAuthenticaionCredentials;

   friend class ReTurnUserFileScanner;
};

class ReTurnUserFileScanner
{
   public:
      ReTurnUserFileScanner(asio::io_service& ioService, ReTurnConfig& reTurnConfig);
      void start();

   private:
      time_t mLoadedTime;
      ReTurnConfig& mReTurnConfig;
      static bool mHup;
      int mLoopInterval;
      time_t mNextFileCheck;
      asio::deadline_timer mTimer;

      bool hasUserFileChanged();
      void timeout(const asio::error_code& e);

      static void onSignal(int signum);
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
