#include <resip/stack/Tuple.hxx>

#include "ReconSubsystem.hxx"
#include "UserAgentMasterProfile.hxx"

#include <utility>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

UserAgentMasterProfile::UserAgentMasterProfile()
: mStatisticsManagerEnabled(false),
  mDTMFDigitLoggingEnabled(true),
  mRTPPortRangeMin(16384),
  mRTPPortRangeMax(17385),
  mSubscriptionRetryInterval(60)
{
#ifdef WIN32
   mCertPath = "./certs";
#else
   const char* home_dir = getenv("HOME");
   if(home_dir)
   {
      mCertPath = home_dir;
   }
   mCertPath += "/.sipCerts/";
#endif
}

void
UserAgentMasterProfile::setTransportSipMessageLoggingHandler(std::shared_ptr<Transport::SipMessageLoggingHandler> handler) noexcept
{
   mTransportSipMessageLoggingHandler = handler;
}

std::shared_ptr<Transport::SipMessageLoggingHandler>
UserAgentMasterProfile::getTransportSipMessageLoggingHandler() const noexcept
{
   return mTransportSipMessageLoggingHandler;
}

void
UserAgentMasterProfile::setRTCPEventLoggingHandler(std::shared_ptr<flowmanager::RTCPEventLoggingHandler> handler) noexcept
{
   mRTCPEventLoggingHandler = handler;
}

std::shared_ptr<flowmanager::RTCPEventLoggingHandler>
UserAgentMasterProfile::getRTCPEventLoggingHandler() const noexcept
{
   return mRTCPEventLoggingHandler;
}

bool&
UserAgentMasterProfile::dtmfDigitLoggingEnabled()
{
   return mDTMFDigitLoggingEnabled;
}

const bool
UserAgentMasterProfile::dtmfDigitLoggingEnabled() const
{
   return mDTMFDigitLoggingEnabled;
}

void 
UserAgentMasterProfile::addTransport( TransportType protocol,
                                      int port, 
                                      IpVersion version,
                                      StunSetting stun,
                                      const Data& ipInterface, 
                                      const Data& sipDomainname,
                                      const Data& privateKeyPassPhrase,
                                      SecurityTypes::SSLType sslType,
                                      unsigned transportFlags,
                                      const Data& certificateFilename,
                                      const Data& privateKeyFilename,
                                      SecurityTypes::TlsClientVerificationMode cvm,
                                      bool useEmailAsSIP,
                                      unsigned int rcvBufLen)
{
   TransportInfo info;

   info.mProtocol = protocol;
   info.mPort = port;
   info.mActualPort = 0; // Only set after UserAgent is created and transports are added.  Useful if mPort is specified as ephemeral (0).
   info.mIPVersion = version;
   info.mIPInterface = ipInterface;
   info.mStunEnabled = stun;
   info.mSipDomainname = sipDomainname;
   info.mTlsPrivateKeyPassPhrase = privateKeyPassPhrase;
   info.mSslType = sslType;
   info.mTransportFlags = transportFlags;
   info.mTlsCertificate = certificateFilename;
   info.mTlsPrivateKey = privateKeyFilename;
   info.mCvm = cvm;
   info.mUseEmailAsSIP = useEmailAsSIP;
   info.mRcvBufLen = rcvBufLen;

   mTransports.push_back(info);
}

std::vector<UserAgentMasterProfile::TransportInfo>& 
UserAgentMasterProfile::getTransports()
{
   return mTransports;
}

void 
UserAgentMasterProfile::addEnumSuffix( const Data& enumSuffix)
{
   mEnumSuffixes.push_back(enumSuffix);
}

const std::vector<Data>& 
UserAgentMasterProfile::getEnumSuffixes() const
{
   return mEnumSuffixes;
}

void 
UserAgentMasterProfile::addAdditionalDnsServer( const Data& dnsServerIPAddress)
{
   mAdditionalDnsServers.push_back(Tuple(dnsServerIPAddress, 0, UNKNOWN_TRANSPORT).toGenericIPAddress());
}

const DnsStub::NameserverList& 
UserAgentMasterProfile::getAdditionalDnsServers() const
{
   return mAdditionalDnsServers;
}

Data& 
UserAgentMasterProfile::certPath()
{
   return mCertPath;
}

const Data 
UserAgentMasterProfile::certPath() const
{
   return mCertPath;
}

std::vector<Data>&
UserAgentMasterProfile::rootCertDirectories()
{
   return mRootCertDirectories;
}

const std::vector<Data>&
UserAgentMasterProfile::rootCertDirectories() const
{
   return mRootCertDirectories;
}

std::vector<Data>&
UserAgentMasterProfile::rootCertBundles()
{
   return mRootCertBundles;
}

const std::vector<Data>&
UserAgentMasterProfile::rootCertBundles() const
{
   return mRootCertBundles;
}

bool& 
UserAgentMasterProfile::statisticsManagerEnabled()
{
   return mStatisticsManagerEnabled;
}

const bool 
UserAgentMasterProfile::statisticsManagerEnabled() const
{
   return mStatisticsManagerEnabled;
}

unsigned short& 
UserAgentMasterProfile::rtpPortRangeMin()
{
   return mRTPPortRangeMin;
}

const unsigned short 
UserAgentMasterProfile::rtpPortRangeMin() const
{
   return mRTPPortRangeMin;
}

unsigned short& 
UserAgentMasterProfile::rtpPortRangeMax()
{
   return mRTPPortRangeMax;
}
 
const unsigned short 
UserAgentMasterProfile::rtpPortRangeMax() const
{
   return mRTPPortRangeMax;
}

int& 
UserAgentMasterProfile::subscriptionRetryInterval()
{
   return mSubscriptionRetryInterval;
}

const int 
UserAgentMasterProfile::subscriptionRetryInterval() const
{
   return mSubscriptionRetryInterval;
}


/* ====================================================================

 Copyright (c) 2021, SIP Spectrum, Inc. www.sipspectrum.com
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
