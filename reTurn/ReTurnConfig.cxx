#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

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
   mAltStunAddress(asio::ip::address::from_string("0.0.0.0")),
   mAuthenticationMode(LongTermPassword),  // required for TURN
   mAuthenticationRealm("reTurn"),
   mNonceLifetime(3600),            // 1 hour - at least 1 hours is recommended by the RFC
   mAllocationPortRangeMin(49152),  // must be even - This default range is the Dynamic and/or Private Port range - recommended by RFC
   mAllocationPortRangeMax(65535),  // must be odd
   mDefaultAllocationLifetime(600), // 10 minutes
   mMaxAllocationLifetime(3600),    // 1 hour
   mMaxAllocationsPerUser(0),       // 0 - no max
   mTlsServerCertificateFilename("server.pem"),
   mTlsTempDhFilename("dh512.pem"),
   mTlsPrivateKeyPassword("password"),
   mLoggingType("cout"),
   mLoggingLevel("INFO"),
   mLoggingFilename("reTurnServer.log"),
   mLoggingFileMaxLineCount(50000),  // 50000 about 5M size
   mDaemonize(false),
   mPidFile(""),
   mRunAsUser(""),
   mRunAsGroup("")
{
   mAuthenticationCredentials["test"] = "1234";
   calcUserAuthData();
}

void ReTurnConfig::parseConfig(int argc, char** argv, const resip::Data& defaultConfigFilename)
{
   resip::ConfigParse::parseConfig(argc, argv, defaultConfigFilename);

   mTurnPort = getConfigUnsignedShort("TurnPort", 3478);
   mTlsTurnPort = getConfigUnsignedShort("TlsTurnPort", 5349);
   mAltStunPort = getConfigUnsignedShort("AltStunPort", 0);
   mTurnAddress = asio::ip::address::from_string(getConfigData("TurnAddress", "0.0.0.0").c_str());
   mAltStunAddress = asio::ip::address::from_string(getConfigData("AltStunAddress", "0.0.0.0").c_str());
   int authMode = getConfigUnsignedShort("AuthenticationMode", 2);
   switch(authMode)
   {
   case 0: mAuthenticationMode = NoAuthentication; break;
   case 1: mAuthenticationMode = ShortTermPassword; break;
   case 2: mAuthenticationMode = LongTermPassword; break;
   default: 
      throw std::runtime_error("Unsupported AuthenticationMode value in config");
   }
   mAuthenticationRealm = getConfigData("AuthenticationRealm", "reTurn");
   mNonceLifetime = getConfigUnsignedLong("NonceLifetime", 3600);
   mAllocationPortRangeMin = getConfigUnsignedShort("AllocationPortRangeMin", 49152);
   mAllocationPortRangeMax = getConfigUnsignedShort("AllocationPortRangeMax", 65535);
   mDefaultAllocationLifetime = getConfigUnsignedLong("DefaultAllocationLifetime", 600);
   mMaxAllocationLifetime = getConfigUnsignedLong("MaxAllocationLifetime", 3600);
   mMaxAllocationsPerUser = getConfigUnsignedLong("MaxAllocationsPerUser", 0);
   mTlsServerCertificateFilename = getConfigData("TlsServerCertificateFilename", "server.pem");
   mTlsTempDhFilename = getConfigData("TlsTempDhFilename", "dh512.pem");
   mTlsPrivateKeyPassword = getConfigData("TlsPrivateKeyPassword", "");
   mLoggingType = getConfigData("LoggingType", "cout");
   mLoggingLevel = getConfigData("LoggingLevel", "INFO");
   mLoggingFilename = getConfigData("LogFilename", "reTurnServer.log");
   mLoggingFileMaxLineCount = getConfigUnsignedLong("LogFileMaxLines", 50000);
   mDaemonize = getConfigBool("Daemonize", false);
   mPidFile = getConfigData("PidFile", "");
   mRunAsUser = getConfigData("RunAsUser", "");
   mRunAsGroup = getConfigData("RunAsGroup", "");

   // fork is not possible on Windows
#ifdef WIN32
   if(mDaemonize)
   {
      throw ConfigParse::Exception("Unable to fork/daemonize on Windows, please check the config", __FILE__, __LINE__);
   }
#endif

	// TODO: For ShortTermCredentials use mAuthenticationCredentials[username] = password;


	// LongTermCredentials

   Data usersDatabase(getConfigData("UserDatabaseFile", ""));

   if(usersDatabase.size() == 0)
   {
      throw ConfigParse::Exception("Missing user database option! Expected \"UserDatabaseFile = file location\".", __FILE__, __LINE__);
   }

   authParse(usersDatabase);
   calcUserAuthData();
}

ReTurnConfig::~ReTurnConfig()
{
}

void
ReTurnConfig::addUser(const resip::Data& username, const resip::Data& password, const resip::Data& realm)
{
   mRealmUsersAuthenticaionCredentials[std::make_pair(username, realm)] = password;
   RealmUsers& realmUsers(mUsers[realm]);

   UserAuthData newUser(UserAuthData::createFromPassword(username, realm, password));
   realmUsers.insert(pair<resip::Data,UserAuthData>(username, newUser));
}

void
ReTurnConfig::authParse(const resip::Data& accountDatabaseFilename)
{
   std::ifstream accountDatabaseFile(accountDatabaseFilename.c_str());
   std::string sline;
   int lineNbr = 0;
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

      if(accountState != REFUSED) {
         addUser(username, password, realm);
      }
   }

   accountDatabaseFile.close();
}


void
ReTurnConfig::calcUserAuthData()
{
   RealmUsers& realmUsers(mUsers[mAuthenticationRealm]);
   std::map<resip::Data,resip::Data>::const_iterator it = mAuthenticationCredentials.begin();
   while(it != mAuthenticationCredentials.end())
   {
      UserAuthData newUser(UserAuthData::createFromPassword(
            it->first,
            mAuthenticationRealm,
            it->second));
      realmUsers.insert(pair<resip::Data,UserAuthData>(it->first,  newUser));
      it++;
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

// ShortTermAuthentication
bool
ReTurnConfig::isUserNameValid(const resip::Data& username) const
{
   std::map<resip::Data,resip::Data>::const_iterator it = mAuthenticationCredentials.find(username);
   return it != mAuthenticationCredentials.end();
}

// LongTermAuthentication
bool
ReTurnConfig::isUserNameValid(const resip::Data& username, const resip::Data& realm) const
{
   return getUser(username, realm) != NULL;
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

const Data&
ReTurnConfig::getPasswordForUsername(const Data& username, const resip::Data& realm) const
{
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

const UserAuthData*
ReTurnConfig::getUser(const resip::Data& userName, const resip::Data& realm) const
{
   std::map<resip::Data,RealmUsers>::const_iterator it = mUsers.find(realm);
   if(it == mUsers.end())
      return NULL;

   RealmUsers realmUsers = it->second;
   RealmUsers::const_iterator it2 = realmUsers.find(userName);
   if(it2 == realmUsers.end())
      return NULL;

   return &(it2->second);
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
