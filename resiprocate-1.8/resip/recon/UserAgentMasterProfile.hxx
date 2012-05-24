#if !defined(UserAgentMasterProfile_hxx)
#define UserAgentMasterProfile_hxx

#include <rutil/TransportType.hxx>
#include <rutil/dns/DnsStub.hxx>
#include <resip/stack/SecurityTypes.hxx> 
#include <resip/dum/MasterProfile.hxx>
#include <vector>

namespace recon
{

/**
  This class extends the resip MasterProfile to include UserAgent 
  specific settings.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class UserAgentMasterProfile : public resip::MasterProfile
{
public:  
   UserAgentMasterProfile();  

   class TransportInfo
   {
   public:
      resip::TransportType mProtocol;
      int mPort;
      resip::IpVersion mIPVersion;
      resip::Data mIPInterface;
      resip::Data mSipDomainname;
      resip::SecurityTypes::SSLType mSslType;
   };

   /**
     Adds a network transport to use for send/receiving SIP messages.

     @note This MUST be called before the UserAgent is created

     @param protocol Transport type: UDP,TCP or TLS
     @param port     UDP or TCP port to listen on
     @param version  IP Version: V4 or V6
     @param ipInterface IP Interface to bind to - empty string
                        binds to all interfaces
     @param sipDomainname TLS Domain name - only used if protocol 
                      is TLS
     @param sslType  SSLv23 vs TLSv1 - only use if protocol is TLS
   */
   void addTransport( resip::TransportType protocol,
                      int port, 
                      resip::IpVersion version=resip::V4,
                      const resip::Data& ipInterface = resip::Data::Empty, 
                      const resip::Data& sipDomainname = resip::Data::Empty, // TLS only
                      resip::SecurityTypes::SSLType sslType = resip::SecurityTypes::TLSv1 );

   /**
     Gets a vector of the transports previously added.

     @return Reference to a vector of TransportInfo's
   */
   const std::vector<TransportInfo>& getTransports() const;

   /**
     Adds a domain suffix used in ENUM DNS queries.  
     For example: e164.arpa

     @note This MUST be called before the UserAgent is created

     @param enumSuffix ENUM Domain suffix to add
   */
   void addEnumSuffix(const resip::Data& enumSuffix);

   /**
     Gets a vector of the ENUM domain suffixes previously added.

     @return Reference to a vector of ENUM domain suffixes
   */
   const std::vector<resip::Data>& getEnumSuffixes() const;

   /**
     Adds an additional DNS server to ones that are attempted
     to be discovered by the SIP stack.

     @note This MUST be called before the UserAgent is created

     @param dnsServerIPAddress IP Address of DNS server to add
   */
   void addAdditionalDnsServer(const resip::Data& dnsServerIPAddress);

   /**
     Gets a vector of the DNS Servers previously added.

     @note This does not include DNS servers detected by the stack

     @return Reference to a vector of DNS server IP Addresses
   */
   const resip::DnsStub::NameserverList& getAdditionalDnsServers() const;

   /**
     Get/Set the location where the SIP stack will look for X509 
     certificates

     @note This MUST be called before the UserAgent is created

     @return Data cert path location
   */
   virtual resip::Data& certPath();
   virtual const resip::Data certPath() const; 

   /**
     Get/Set wether SIP message statistics are send to logging subsystem

     @return bool Set to true to enable statistics
   */
   virtual bool& statisticsManagerEnabled();
   virtual const bool statisticsManagerEnabled() const; 

   /**
     Get/Set the starting port number in the range of valid port
     numbers to be used for RTP traffic.

     @note This MUST be called before the UserAgent is created

     @return unsigned short range min
   */
   virtual unsigned short& rtpPortRangeMin();
   virtual const unsigned short rtpPortRangeMin() const;

   /**
     Get/Set the ending port number in the range of valid port
     numbers to be used for RTP traffic.

     @note This MUST be called before the UserAgent is created

     @return unsigned short range max
   */
   virtual unsigned short& rtpPortRangeMax();
   virtual const unsigned short rtpPortRangeMax() const;

   /**
     Get/Set the interval at which subscriptions are retried if
     they fail (if one is not suggested in the failure).

     @note This MUST be called before the UserAgent is created

     @return int subscription retry interval in seconds
   */
   virtual int& subscriptionRetryInterval();
   virtual const int subscriptionRetryInterval() const;

private:            
   resip::Data mCertPath;
   bool mStatisticsManagerEnabled;
   std::vector<TransportInfo> mTransports;
   std::vector<resip::Data> mEnumSuffixes;
   resip::DnsStub::NameserverList mAdditionalDnsServers;
   unsigned short mRTPPortRangeMin;
   unsigned short mRTPPortRangeMax;
   int mSubscriptionRetryInterval;
};

}

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
