#include <resip/stack/Tuple.hxx>

#include "UserAgentSubsystem.hxx"
#include "UserAgentMasterProfile.hxx"

using namespace useragent;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM UserAgentSubsystem::USERAGENT

UserAgentMasterProfile::UserAgentMasterProfile()
: mStatisticsManagerEnabled(false),
  mRTPPortRangeMin(16384),
  mRTPPortRangeMax(17385),
  mSubscriptionRetryInterval(60)
{
#ifdef WIN32
   mCertPath = ".";
#else
   mCertPath = getenv("HOME");
   mCertPath += "/.sipCerts/";
#endif
}

void 
UserAgentMasterProfile::addTransport( TransportType protocol,
                                      int port, 
                                      IpVersion version,
                                      const Data& ipInterface, 
                                      const Data& sipDomainname,
                                      SecurityTypes::SSLType sslType)
{
   TransportInfo info;

   info.mProtocol = protocol;
   info.mPort = port;
   info.mIPVersion = version;
   info.mIPInterface = ipInterface;
   info.mSipDomainname = sipDomainname;
   info.mSslType = sslType;

   mTransports.push_back(info);
}

const std::vector<UserAgentMasterProfile::TransportInfo>& 
UserAgentMasterProfile::getTransports() const
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

 Original contribution Copyright (C) 2008 Plantronics, Inc.
 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */
