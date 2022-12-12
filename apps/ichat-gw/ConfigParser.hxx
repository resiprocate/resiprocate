#if !defined(ConfigParser_hxx)
#define ConfigParser_hxx

#include <list>
#include <resip/dum/MasterProfile.hxx>
#include <rutil/dns/DnsStub.hxx>
#include <rutil/Log.hxx>


namespace gateway
{

class ConfigParser 
{
public:
   ConfigParser(int argc, char** argv);
   virtual ~ConfigParser();
   
   resip::Data mAddress;
   resip::DnsStub::NameserverList mDnsServers;
   resip::NameAddr mGatewayIdentity;
   unsigned short mSipPort;
   unsigned short mTlsPort;
   resip::Data mTlsDomain;
   resip::NameAddr mOutboundProxy;
   unsigned int mRegistrationTime;
   unsigned int mRegistrationRetryTime;
   bool mKeepAlives;
   resip::Data mLogLevel;
   resip::Data mLogFilename;
   unsigned int mLogFileMaxLines;
   unsigned short mGatewayIPCPort;
   unsigned short mJabberConnectorIPCPort;
   unsigned short mHttpPort;
   bool mHttpAuth;
   resip::Data mHttpAuthPwd;
   unsigned int mIChatProceedingTimeout;
   bool mAlwaysRelayIChatMedia;
   bool mPreferIPv6;
   bool mSkipFirstIChatAddress; // (for testing): skipping the first IPV4 NAT mapped address (on systems with one interface) causes the local address to be used
   typedef std::set<unsigned short> CodecIdList;
   CodecIdList mCodecIdFilterList;
   unsigned short mMediaRelayPortRangeMin;
   unsigned short mMediaRelayPortRangeMax;
   typedef std::list<std::pair<resip::Data,resip::Data> > TranslationList;
   TranslationList mAddressTranslations;

   // Jabber specific settings
   resip::Data mJabberServer;
   resip::Data mJabberComponentName;
   resip::Data mJabberComponentPassword;
   unsigned short mJabberComponentPort;
   unsigned int mJabberServerPingDuration;
   resip::Data mJabberControlUsername;

   // TLS specific settings
   resip::Data mTLSDHParamsFilename;
   resip::Data mTLSCertificate;
   resip::Data mTLSPrivateKey;
   resip::Data mTLSPrivateKeyPassPhrase;

   // iChat Jabber Connector file path
   resip::Data mIchatJabberConnectorPath;
   
private:
   void parseCommandLine(int argc, char** argv);
   void parseConfigFile(const resip::Data& filename);
   bool processOption(const resip::Data& name, const resip::Data& value);
   bool assignNameAddr(const resip::Data& settingName, const resip::Data& settingValue, resip::NameAddr& nameAddr);
   bool assignOnOffSetting(const resip::Data& value, bool& setting);
};
 
}

#endif

/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
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

