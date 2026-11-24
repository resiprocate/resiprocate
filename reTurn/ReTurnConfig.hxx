#if !defined(RETURN_CONFIG_HXX)
#define RETURN_CONFIG_HXX 

#include <cstdint>
#include <map>
#include <string>
#include <vector>
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

/// IPv4 and IPv6 address ranges, for AllowedPeerAddresses and DeniedPeerAddresses
class PeerAddressList
{
public:
   /// Adds an address range in CIDR form ("10.0.0.0/8", "fc00::/7"), or a single address.
   /// Throws std::invalid_argument if entry is neither.
   void add(const resip::Data& entry);
   /// Whether address is in one of the ranges.  An IPv4 address is only in IPv4 ranges and an
   /// IPv6 address only in IPv6 ranges, so an IPv6 address carrying an IPv4 one (as ::ffff:a.b.c.d)
   /// should be given as that IPv4 address (see RequestHandler::toComparableAddress).
   bool contains(const asio::ip::address& address) const;
   bool empty() const { return mV4.empty() && mV6.empty(); }
   /// The ranges, comma separated
   std::string toString() const;

private:
   std::vector<asio::ip::network_v4> mV4;
   std::vector<asio::ip::network_v6> mV6;
};

class ReTurnConfig : public resip::ConfigParse
{
public:
   class Exception final : public resip::BaseException
   {
      public:
         Exception(const resip::Data& msg,
                   const resip::Data& file,
                   const int line)
            : resip::BaseException(msg, file, line) {}            
      protected:
         const char* name() const noexcept override { return "ReTurnConfig::Exception"; }
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
   unsigned long mMaxAllocationsPerUser;  // over all transports, 0 for no limit
   bool mAllowLocalPeers;                 // whether clients may relay to this host: loopback, unspecified and its own addresses
   PeerAddressList mAllowedPeerAddresses; // peers allowed even if local or denied
   PeerAddressList mDeniedPeerAddresses;  // peers refused

   resip::Data mTlsServerCertificateFilename;
   resip::Data mTlsServerPrivateKeyFilename;
   resip::Data mTlsTempDhFilename;
   resip::Data mTlsPrivateKeyPassword;
   unsigned long mTlsHandshakeTimeout;  // seconds, 0 for none
   unsigned long mTcpConnectionIdleTimeout;  // seconds a TCP/TLS connection without an allocation may receive nothing, 0 for none
   unsigned long mMaxTcpConnectionsPerIp;    // TCP and TLS connections from one client IP address, 0 for no limit
   uint64_t mOpenSSLCTXSetOptions;    // SSL_OP_* flags for SSL_CTX_set_options, as repro's settings of the same name
   uint64_t mOpenSSLCTXClearOptions;  // SSL_OP_* flags for SSL_CTX_clear_options

   resip::Data mUsersDatabaseFilename;
   bool mUserDatabaseHashedPasswords;

   bool mDaemonize;
   resip::Data mPidFile;
   resip::Data mRunAsUser;
   resip::Data mRunAsGroup;

   bool isUserNameValid(const resip::Data& username,  const resip::Data& realm) const;
   resip::Data getHa1ForUsername(const resip::Data& username, const resip::Data& realm) const;
   std::unique_ptr<UserAuthData> getUser(const resip::Data& userName, const resip::Data& realm) const;
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
      ReTurnUserFileScanner(asio::io_context& ioService, ReTurnConfig& reTurnConfig);
      void start();

   private:
      time_t mLoadedTime;
      ReTurnConfig& mReTurnConfig;
      static bool mHup;
      int mLoopInterval;
      time_t mNextFileCheck;
      asio::steady_timer mTimer;

      bool hasUserFileChanged();
      void timeout(const asio::error_code& e);

      static void onSignal(int signum);
};

} // namespace

#endif


/* ====================================================================

 Copyright (c) 2008-2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
