#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include <sys/stat.h>

#include <boost/bind.hpp>

#include "ReTurnConfig.hxx"

#include "ReTurnSubsystem.hxx"
#include "rutil/ParseBuffer.hxx"
#include <rutil/Logger.hxx>

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;
using namespace resip;

namespace reTurn {

ReTurnConfig::ReTurnConfig() :
   mTurnPort(3478),
   mTlsTurnPort(5349),
   mAltStunPort(0), // Note:  The default is to disable RFC3489 binding support
   mTurnAddress(asio::ip::address::from_string("0.0.0.0")),
   mTurnV6Address(asio::ip::address::from_string("::0")),
   mAltStunAddress(asio::ip::address::from_string("0.0.0.0")),
   mAuthenticationRealm("reTurn"),
   mUserDatabaseCheckInterval(60),
   mNonceLifetime(3600),            // 1 hour - at least 1 hours is recommended by the RFC
   mAllocationPortRangeMin(49152),  // must be even - This default range is the Dynamic and/or Private Port range - recommended by RFC
   mAllocationPortRangeMax(65535),  // must be odd
   mDefaultAllocationLifetime(600), // 10 minutes
   mMaxAllocationLifetime(3600),    // 1 hour
   mMaxAllocationsPerUser(0),       // 0 - no max
   mTlsServerCertificateFilename("server.pem"),
   mTlsServerPrivateKeyFilename(""),
   mTlsTempDhFilename("dh512.pem"),
   mTlsPrivateKeyPassword(""),
   mUsersDatabaseFilename(""),
   mUserDatabaseHashedPasswords(false),
   mRunWithoutValidUsers(false),
   mLoggingType("cout"),
   mLoggingLevel("INFO"),
   mLoggingFilename("reTurnServer.log"),
   mLoggingFileMaxLineCount(50000),  // 50000 about 5M size
   mDaemonize(false),
   mPidFile(""),
   mRunAsUser(""),
   mRunAsGroup("")
{
}

void ReTurnConfig::parseConfig(int argc, char** argv, const resip::Data& defaultConfigFilename)
{
   resip::ConfigParse::parseConfig(argc, argv, defaultConfigFilename);

   mTurnPort = getConfigUnsignedShort("TurnPort", mTurnPort);
   mTlsTurnPort = getConfigUnsignedShort("TlsTurnPort", mTlsTurnPort);
   mAltStunPort = getConfigUnsignedShort("AltStunPort", mAltStunPort);
   mTurnAddress = asio::ip::address::from_string(getConfigData("TurnAddress", "0.0.0.0").c_str());
   mTurnV6Address = asio::ip::address::from_string(getConfigData("TurnV6Address", "::0").c_str());
   mAltStunAddress = asio::ip::address::from_string(getConfigData("AltStunAddress", "0.0.0.0").c_str());
   mAuthenticationRealm = getConfigData("AuthenticationRealm", mAuthenticationRealm);
   mUserDatabaseCheckInterval = getConfigUnsignedShort("UserDatabaseCheckInterval", 60);
   mNonceLifetime = getConfigUnsignedLong("NonceLifetime", mNonceLifetime);
   mAllocationPortRangeMin = getConfigUnsignedShort("AllocationPortRangeMin", mAllocationPortRangeMin);
   mAllocationPortRangeMax = getConfigUnsignedShort("AllocationPortRangeMax", mAllocationPortRangeMax);
   mDefaultAllocationLifetime = getConfigUnsignedLong("DefaultAllocationLifetime", mDefaultAllocationLifetime);
   mMaxAllocationLifetime = getConfigUnsignedLong("MaxAllocationLifetime", mMaxAllocationLifetime);
   mMaxAllocationsPerUser = getConfigUnsignedLong("MaxAllocationsPerUser", mMaxAllocationsPerUser);
   mTlsServerCertificateFilename = getConfigData("TlsServerCertificateFilename", mTlsServerCertificateFilename);
   mTlsServerPrivateKeyFilename = getConfigData("TlsServerPrivateKeyFilename", mTlsServerPrivateKeyFilename);
   mTlsTempDhFilename = getConfigData("TlsTempDhFilename", mTlsTempDhFilename);
   mTlsPrivateKeyPassword = getConfigData("TlsPrivateKeyPassword", mTlsPrivateKeyPassword);
   mUserDatabaseHashedPasswords = getConfigBool("UserDatabaseHashedPasswords", mUserDatabaseHashedPasswords);
   mRunWithoutValidUsers = getConfigBool("RunWithoutValidUsers", mRunWithoutValidUsers);
   mLoggingType = getConfigData("LoggingType", mLoggingType);
   mLoggingLevel = getConfigData("LoggingLevel", mLoggingLevel);
   mLoggingFilename = getConfigData("LogFilename", mLoggingFilename);
   mLoggingFileMaxLineCount = getConfigUnsignedLong("LogFileMaxLines", mLoggingFileMaxLineCount);
   mDaemonize = getConfigBool("Daemonize", mDaemonize);
   mPidFile = getConfigData("PidFile", mPidFile);
   mRunAsUser = getConfigData("RunAsUser", mRunAsUser);
   mRunAsGroup = getConfigData("RunAsGroup", mRunAsGroup);

   // fork is not possible on Windows
#ifdef WIN32
   if(mDaemonize)
   {
      throw ConfigParse::Exception("Unable to fork/daemonize on Windows, please check the config", __FILE__, __LINE__);
   }
#endif

   // TODO: For ShortTermCredentials use mAuthenticationCredentials[username] = password;


   // LongTermCredentials
   mUsersDatabaseFilename = getConfigData("UserDatabaseFile", "");
   if(mUsersDatabaseFilename.size() == 0)
   {
      throw ConfigParse::Exception("Missing user database option! Expected \"UserDatabaseFile = file location\".", __FILE__, __LINE__);
   }

   AddBasePathIfRequired(mLoggingFilename);
   AddBasePathIfRequired(mTlsServerCertificateFilename);
   AddBasePathIfRequired(mTlsServerPrivateKeyFilename);
   AddBasePathIfRequired(mTlsTempDhFilename);
   AddBasePathIfRequired(mUsersDatabaseFilename);
   
   authParse(mUsersDatabaseFilename);
}

ReTurnConfig::~ReTurnConfig()
{
}

void
ReTurnConfig::addUser(const resip::Data& username, const resip::Data& password, const resip::Data& realm)
{
   UserAuthData newUser(
      mUserDatabaseHashedPasswords ?
         UserAuthData::createFromHex(username, realm, password)
       : UserAuthData::createFromPassword(username, realm, password)
      );
   mRealmUsersAuthenticaionCredentials[std::make_pair(username, realm)] = newUser.getHa1();
   RealmUsers& realmUsers(mUsers[realm]);
   realmUsers.insert(pair<resip::Data,UserAuthData>(username, newUser));
}

void
ReTurnConfig::authParse(const resip::Data& accountDatabaseFilename)
{
   std::ifstream accountDatabaseFile(accountDatabaseFilename.c_str());
   std::string sline;
   int lineNbr = 0, userCount = 0;
   if(!accountDatabaseFile)
   {
      throw ReTurnConfig::Exception("Error opening/reading user database file!", __FILE__, __LINE__);
   }

   while(std::getline(accountDatabaseFile, sline))
   {
      AccountState accountState;
      Data username;
      Data password;
      Data realm;
      Data state;
      Data line(sline);
      ParseBuffer pb(line);

      lineNbr++;

      // Jump over empty lines.
      if(line.size() == 0)
      {
          continue;
      }

      pb.skipWhitespace();
      if(!pb.eof() && *pb.position() == '#')
      {
         // Line is commented out, skip it
         continue;
      }

      const char * anchor = pb.position();

      pb.skipToOneOf(" :");

      if (pb.eof())
      {
         ErrLog(<< "Missing or invalid credentials at line " << lineNbr);
         continue;
      }

      pb.data(username, anchor);

      pb.skipToChar(':');
      if (!pb.eof())
      {
         pb.skipChar(':');
         pb.skipWhitespace();
      }

      anchor = pb.position();
      pb.skipToOneOf(" :");

      if (pb.eof())
      {
         ErrLog(<< "Missing or invalid credentials at line " << lineNbr);
         continue;
      }

      pb.data(password, anchor);

      pb.skipToChar(':');
      if (!pb.eof())
      {
         pb.skipChar(':');
         pb.skipWhitespace();
      }

      anchor = pb.position();
      pb.skipToOneOf(" :");

      if (pb.eof())
      {
         ErrLog(<< "Missing or invalid credentials at line " << lineNbr);
         continue;
      }

      pb.data(realm, anchor);

      pb.skipToChar(':');
      if (!pb.eof())
      {
         pb.skipChar(':');
         pb.skipWhitespace();
      }

      anchor = pb.position();
      pb.skipToOneOf(" \t\n");

      pb.data(state, anchor);
      state.lowercase();

      if (state.size() != 0)
      {
         if(state == "authorized")
         {
            accountState = AUTHORIZED;
         }
         else if(state == "restricted")
         {
            accountState = RESTRICTED;
         }
         else if(state == "refused")
         {
            accountState = REFUSED;
         }
         else
         {
            ErrLog(<< "Invalid state value at line " << lineNbr << ", state= " << state);
            continue;
         }
      }
      else
      {
         ErrLog(<< "Missing state value at line " << lineNbr);
         continue;
      }

      if(accountState != REFUSED) 
      {
         addUser(username, password, realm);
      }
      userCount++;
   }

   InfoLog(<< "Processed " << userCount << " user(s) from " << lineNbr << " line(s) in " << accountDatabaseFilename);
   accountDatabaseFile.close();

   if(mUsers.find(mAuthenticationRealm) == mUsers.end())
   {
      if(userCount > 0)
      {
         WarningLog(<<"AuthenticationRealm = " << mAuthenticationRealm << " but no users defined for this realm in " << accountDatabaseFilename);
      }
      else
      {
         WarningLog(<<"No valid users found");
      }
      if(!mRunWithoutValidUsers)
      {
         ErrLog(<<"No valid users found, please check AuthenticationRealm matches the realm in " << accountDatabaseFilename << " or set RunWithoutValidUsers if you only need to support STUN clients");
         throw ConfigParse::Exception("No valid users found, please fix or set RunWithoutValidUsers if you only need to support STUN clients", __FILE__, __LINE__);
      }
   }
}

void
ReTurnConfig::printHelpText(int argc, char **argv)
{
   std::cerr << "Command line format is:" << std::endl;
   std::cerr << "  " << removePath(argv[0]) << " [<ConfigFilename>] [--<ConfigValueName>=<ConfigValue>] [--<ConfigValueName>=<ConfigValue>] ..." << std::endl;
   std::cerr << "Sample Command lines:" << std::endl;
   std::cerr << "  " << removePath(argv[0]) << " reTurnServer.config --LogLevel=INFO" << std::endl;
}

// LongTermAuthentication
bool
ReTurnConfig::isUserNameValid(const resip::Data& username, const resip::Data& realm) const
{
   return getUser(username, realm).get() != NULL;
}

Data
ReTurnConfig::getHa1ForUsername(const Data& username, const resip::Data& realm) const
{
   ReadLock lock(mUserDataMutex);
   std::map<RealmUserPair, resip::Data>::const_iterator it = mRealmUsersAuthenticaionCredentials.find(std::make_pair(username, realm));
   if(it != mRealmUsersAuthenticaionCredentials.end())
   {
      return it->second;
   }
   else
   {
      return Data::Empty;
   }
}

std::auto_ptr<UserAuthData>
ReTurnConfig::getUser(const resip::Data& userName, const resip::Data& realm) const
{
   ReadLock lock(mUserDataMutex);
   std::auto_ptr<UserAuthData> ret(0);
   std::map<resip::Data,RealmUsers>::const_iterator it = mUsers.find(realm);
   if(it == mUsers.end())
      return ret;

   RealmUsers realmUsers = it->second;
   RealmUsers::const_iterator it2 = realmUsers.find(userName);
   if(it2 == realmUsers.end())
      return ret;

   return std::auto_ptr<UserAuthData>(new UserAuthData(it2->second));
}

ReTurnUserFileScanner::ReTurnUserFileScanner(asio::io_service& ioService, ReTurnConfig& reTurnConfig)
 : mLoadedTime(time(0)),
   mTimer(ioService, boost::posix_time::seconds(reTurnConfig.mUserDatabaseCheckInterval)),
   mReTurnConfig(reTurnConfig)
{
}

void
ReTurnUserFileScanner::start()
{
   if(mReTurnConfig.mUserDatabaseCheckInterval > 0)
   {
      mTimer.expires_from_now(boost::posix_time::seconds(mReTurnConfig.mUserDatabaseCheckInterval));
      mTimer.async_wait(boost::bind(&ReTurnUserFileScanner::timeout, this, asio::placeholders::error));
   }
   else
   {
      InfoLog(<<"UserDatabaseCheckInterval = 0, not checking for updates to user database " << mReTurnConfig.mUsersDatabaseFilename);
   }
}

void
ReTurnUserFileScanner::timeout(const asio::error_code& e)
{
   StackLog(<<"checking user database freshness");

   time_t latestFileTS = 0;
#ifdef WIN32
   StackLog(<<"not yet implemented on Windows");
#else
   struct stat user_stat;
   int ret = stat(mReTurnConfig.mUsersDatabaseFilename.c_str(), &user_stat);
   if(ret == 0)
   {
      latestFileTS = user_stat.st_mtime;
   }
   else
   {
      ErrLog(<<"Call to stat failed, not checking " << mReTurnConfig.mUsersDatabaseFilename << " freshness, errno = " << errno);
   }
#endif

   if(latestFileTS > mLoadedTime)
   {
      InfoLog(<<"change in user database detected, reloading...");
      WriteLock lock(mReTurnConfig.mUserDataMutex);
      mReTurnConfig.mUsers.clear();
      mReTurnConfig.mRealmUsersAuthenticaionCredentials.clear();
      mReTurnConfig.authParse(mReTurnConfig.mUsersDatabaseFilename);
      InfoLog(<<"user database reload completed");
      mLoadedTime = time(0);
   }
   // set the timer again:
   start();
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
