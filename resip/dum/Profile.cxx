
#include "resip/dum/Profile.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/HeaderTypes.hxx"

#include <utility>

using namespace resip;
#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

Profile::Profile() :
   mHasOutboundDecorator(false)
{
	Profile::reset();  // set defaults
}

Profile::Profile(std::shared_ptr<Profile> baseProfile) :
   mHasOutboundDecorator(false),
   mBaseProfile(std::move(baseProfile))
{
   resip_assert(baseProfile.get());

   Profile::reset();  // default all settings to fallthrough to mBaseProfile
}

void
Profile::reset()
{
   unsetDefaultRegistrationTime();  
   unsetDefaultMaxRegistrationTime();   
   unsetDefaultRegistrationRetryTime();   
   unsetDefaultSubscriptionTime();   
   unsetDefaultPublicationTime();  
   unsetDefaultStaleCallTime();  
   unsetDefaultStaleReInviteTime();  
   unsetDefaultSessionTime(); 
   unsetDefaultSessionTimerMode();   
   unset1xxRetransmissionTime();   
   unset1xxRelResubmitTime();
   unsetOverrideHostAndPort(); 
   unsetAdvertisedCapabilities();
   unsetOutboundProxy(); 
   unsetForceOutboundProxyOnAllRequestsEnabled();
   unsetExpressOutboundAsRouteSetEnabled();
   unsetRportEnabled(); 
   unsetUserAgent(); 
   unsetProxyRequires(); 
   unsetKeepAliveTimeForDatagram();
   unsetKeepAliveTimeForStream();
   unsetFixedTransportPort(); 
   unsetFixedTransportInterface(); 
   unsetRinstanceEnabled();
   unsetOutboundDecorator();
   unsetMethodsParamEnabled();
   unsetUserAgentCapabilities();
   unsetExtraHeadersInReferNotifySipFragEnabled();
}

void
Profile::setDefaultRegistrationTime(UInt32 secs) noexcept
{
   mDefaultRegistrationExpires = secs;
   mHasDefaultRegistrationExpires = true;
}

UInt32 
Profile::getDefaultRegistrationTime() const noexcept
{
   // Fall through setting (if required)
   if (!mHasDefaultRegistrationExpires && mBaseProfile)
   {
       return mBaseProfile->getDefaultRegistrationTime();
   }
   return mDefaultRegistrationExpires;
}

void
Profile::unsetDefaultRegistrationTime() noexcept
{
   if (mBaseProfile) 
   {
      mHasDefaultRegistrationExpires = false;
   }
   else // No Base profile - so return to default setting
   {
      mHasDefaultRegistrationExpires = true;
      mDefaultRegistrationExpires = 3600; // 1 hour
   }
}

void
Profile::setDefaultMaxRegistrationTime(UInt32 secs) noexcept
{
   mDefaultMaxRegistrationExpires = secs;
   mHasDefaultMaxRegistrationExpires = true;
}

UInt32 
Profile::getDefaultMaxRegistrationTime() const noexcept
{
   // Fall through setting (if required)
   if (!mHasDefaultMaxRegistrationExpires && mBaseProfile)
   {
       return mBaseProfile->getDefaultMaxRegistrationTime();
   }
   return mDefaultMaxRegistrationExpires;
}

void
Profile::unsetDefaultMaxRegistrationTime() noexcept
{
   if (mBaseProfile) 
   {
      mHasDefaultMaxRegistrationExpires = false;
   }
   else // No Base profile - so return to default setting
   {
      mHasDefaultMaxRegistrationExpires = true;
      mDefaultMaxRegistrationExpires = 0; // No restriction
   }
}

void
Profile::setDefaultRegistrationRetryTime(int secs) noexcept
{
   mDefaultRegistrationRetryInterval = secs;
   mHasDefaultRegistrationRetryInterval = true;
}

int 
Profile::getDefaultRegistrationRetryTime() const noexcept
{
   // Fall through setting (if required)
   if (!mHasDefaultRegistrationRetryInterval && mBaseProfile)
   {
       return mBaseProfile->getDefaultRegistrationRetryTime();
   }
   return mDefaultRegistrationRetryInterval;
}

void
Profile::unsetDefaultRegistrationRetryTime() noexcept
{
   if (mBaseProfile) 
   {
      mHasDefaultRegistrationRetryInterval = false;
   }
   else // No Base profile - so return to default setting
   {
      mHasDefaultRegistrationRetryInterval = true;
      mDefaultRegistrationRetryInterval = 0; // Retries disabled
   }
}

void
Profile::setDefaultSubscriptionTime(UInt32 secs) noexcept
{
   mDefaultSubscriptionExpires = secs;
   mHasDefaultSubscriptionExpires = true;
}

UInt32 
Profile::getDefaultSubscriptionTime() const noexcept
{
   // Fall through setting (if required)
   if (!mHasDefaultSubscriptionExpires && mBaseProfile)
   {
       return mBaseProfile->getDefaultSubscriptionTime();
   }
   return mDefaultSubscriptionExpires;
}

void
Profile::unsetDefaultSubscriptionTime() noexcept
{
   if (mBaseProfile) 
   {
      mHasDefaultSubscriptionExpires = false;
   }
   else // No Base profile - so return to default setting
   {
      mHasDefaultSubscriptionExpires = true;
      mDefaultSubscriptionExpires = 3600; // 1 hour
   }
}

void
Profile::setDefaultPublicationTime(UInt32 secs) noexcept
{
   mDefaultPublicationExpires = secs;
   mHasDefaultPublicationExpires = true;
}

UInt32 
Profile::getDefaultPublicationTime() const noexcept
{
   // Fall through setting (if required)
   if (!mHasDefaultPublicationExpires && mBaseProfile)
   {
       return mBaseProfile->getDefaultPublicationTime();
   }
   return mDefaultPublicationExpires;
}

void
Profile::unsetDefaultPublicationTime() noexcept
{
   if (mBaseProfile) 
   {
      mHasDefaultPublicationExpires = false;
   }
   else // No Base profile - so return to default setting
   {
      mHasDefaultPublicationExpires = true;
      mDefaultPublicationExpires = 3600;  // 1 hour
   }
}

void
Profile::setDefaultStaleCallTime(int secs) noexcept
{
   mDefaultStaleCallTime = secs;
   mHasDefaultStaleCallTime = true;
}

int 
Profile::getDefaultStaleCallTime() const noexcept
{
   // Fall through setting (if required)
   if (!mHasDefaultStaleCallTime && mBaseProfile)
   {
       return mBaseProfile->getDefaultStaleCallTime();
   }
   return mDefaultStaleCallTime;
}

void
Profile::unsetDefaultStaleCallTime() noexcept
{
   if (mBaseProfile) 
   {
      mHasDefaultStaleCallTime = false;
   }
   else // No Base profile - so return to default setting
   {
      mHasDefaultStaleCallTime = true;
      mDefaultStaleCallTime = 180;        // 3 minutes
   }
}

void
Profile::setDefaultStaleReInviteTime(int secs) noexcept
{
   mDefaultStaleReInviteTime = secs;
   mHasDefaultStaleReInviteTime = true;
}

int 
Profile::getDefaultStaleReInviteTime() const noexcept
{
   // Fall through setting (if required)
   if (!mHasDefaultStaleReInviteTime && mBaseProfile)
   {
       return mBaseProfile->getDefaultStaleReInviteTime();
   }
   return mDefaultStaleReInviteTime;
}

void
Profile::unsetDefaultStaleReInviteTime() noexcept
{
   if (mBaseProfile) 
   {
      mHasDefaultStaleReInviteTime = false;
   }
   else // No Base profile - so return to default setting
   {
      mHasDefaultStaleReInviteTime = true;
      mDefaultStaleReInviteTime = 40;        // 40 Seconds (slightly longer than T1*64)
   }
}

void
Profile::setDefaultSessionTime(UInt32 secs) noexcept
{
   mDefaultSessionExpires = secs;
   mHasDefaultSessionExpires = true;
}

UInt32 
Profile::getDefaultSessionTime() const noexcept
{
   // Fall through setting (if required)
   if (!mHasDefaultSessionExpires && mBaseProfile)
   {
       return mBaseProfile->getDefaultSessionTime();
   }
   return mDefaultSessionExpires;
}

void
Profile::unsetDefaultSessionTime() noexcept
{
   if (mBaseProfile) 
   {
      mHasDefaultSessionExpires = false;
   }
   else // No Base profile - so return to default setting
   {
      mHasDefaultSessionExpires = true;
      mDefaultSessionExpires = 1800;      // 30 minutes
   }
}

void
Profile::setDefaultSessionTimerMode(Profile::SessionTimerMode mode) noexcept
{
   mDefaultSessionTimerMode = mode;
   mHasDefaultSessionTimerMode = true;
}

Profile::SessionTimerMode 
Profile::getDefaultSessionTimerMode() const noexcept
{
   // Fall through setting (if required)
   if (!mHasDefaultSessionTimerMode && mBaseProfile)
   {
       return mBaseProfile->getDefaultSessionTimerMode();
   }
   return mDefaultSessionTimerMode;
}

void
Profile::unsetDefaultSessionTimerMode() noexcept
{
   if (mBaseProfile) 
   {
      mHasDefaultSessionTimerMode = false;
   }
   else // No Base profile - so return to default setting
   {
      mHasDefaultSessionTimerMode = true;
      mDefaultSessionTimerMode = Profile::PreferCallerRefreshes;
   }
}

void
Profile::set1xxRetransmissionTime(int secs) noexcept
{
   m1xxRetransmissionTime = secs;
   mHas1xxRetransmissionTime = true;
}

int 
Profile::get1xxRetransmissionTime() const noexcept
{
   // Fall through setting (if required)
   if (!mHas1xxRetransmissionTime && mBaseProfile)
   {
       return mBaseProfile->get1xxRetransmissionTime();
   }
   return m1xxRetransmissionTime;
}

void
Profile::unset1xxRetransmissionTime() noexcept
{
   if (mBaseProfile) 
   {
      mHas1xxRetransmissionTime = false;
   }
   else // No Base profile - so return to default setting
   {
      mHas1xxRetransmissionTime = true;
      m1xxRetransmissionTime = 60;  // RFC3261 13.3.1 specifies this timeout should be 1 minute
   }
}

void
Profile::set1xxRelResubmitTime(int secs) noexcept
{
   m1xxRelResubmitTime = secs;
   mHas1xxRelResubmitTime = true;
}

int 
Profile::get1xxRelResubmitTime() const noexcept
{
   // Fall through setting (if required)
   if (!mHas1xxRelResubmitTime && mBaseProfile)
   {
       return mBaseProfile->get1xxRelResubmitTime();
   }
   return m1xxRelResubmitTime;
}

void
Profile::unset1xxRelResubmitTime() noexcept
{
   if (mBaseProfile) 
   {
      mHas1xxRelResubmitTime = false;
   }
   else // No Base profile - so return to default setting
   {
      mHas1xxRelResubmitTime = true;
      m1xxRelResubmitTime = 150;  // RFC3262 section says the UAS SHOULD send provisional reliable responses once every two and half minutes
   }
}

void 
Profile::setOverrideHostAndPort(const Uri& hostPort)
{
   mOverrideHostPort = hostPort;   
   mHasOverrideHostPort = true;   
}

bool 
Profile::hasOverrideHostAndPort() const noexcept
{
   // Fall through setting (if required)
   if (!mHasOverrideHostPort && mBaseProfile)
   {
       return mBaseProfile->hasOverrideHostAndPort();
   }
   return mHasOverrideHostPort;
}

const Uri& 
Profile::getOverrideHostAndPort() const
{
   // Fall through setting (if required)
   if (!mHasOverrideHostPort && mBaseProfile)
   {
       return mBaseProfile->getOverrideHostAndPort();
   }
   return mOverrideHostPort;
}

void
Profile::unsetOverrideHostAndPort() noexcept
{
   mHasOverrideHostPort = false;
}

void 
Profile::addAdvertisedCapability(const Headers::Type header)
{
   resip_assert(header == Headers::Allow ||
          header == Headers::AcceptEncoding ||
          header == Headers::AcceptLanguage ||
          header == Headers::AllowEvents ||
          header == Headers::Supported);

   mAdvertisedCapabilities.insert(header);
   mHasAdvertisedCapabilities = true;
}
 
bool 
Profile::isAdvertisedCapability(const Headers::Type header) const
{
   // Fall through setting (if required)
   if (!mHasAdvertisedCapabilities && mBaseProfile)
   {
       return mBaseProfile->isAdvertisedCapability(header);
   }
   return mAdvertisedCapabilities.count(header) != 0;
}

void 
Profile::clearAdvertisedCapabilities() noexcept
{
   mHasAdvertisedCapabilities = true;
   return mAdvertisedCapabilities.clear();
}

void
Profile::unsetAdvertisedCapabilities()
{
   if (mBaseProfile) 
   {
      mHasAdvertisedCapabilities = false;
   }
   else // No Base profile - so return to default setting
   {
      mHasAdvertisedCapabilities = true;
      addAdvertisedCapability(Headers::Allow);  
      addAdvertisedCapability(Headers::Supported);  
   }
}

void 
Profile::setOutboundProxy( const Uri& uri )
{
   Uri tmpUri(uri);
   tmpUri.param(p_lr);
   mOutboundProxy = NameAddr(tmpUri);
   mHasOutboundProxy = true;
}

const NameAddr&
Profile::getOutboundProxy() const
{
   // Fall through setting (if required)
   if (!mHasOutboundProxy && mBaseProfile)
   {
       return mBaseProfile->getOutboundProxy();
   }
   resip_assert(mHasOutboundProxy);
   return mOutboundProxy;
}

bool
Profile::hasOutboundProxy() const noexcept
{
   // Fall through setting (if required)
   if (!mHasOutboundProxy && mBaseProfile)
   {
       return mBaseProfile->hasOutboundProxy();
   }
   return mHasOutboundProxy;
}

void
Profile::unsetOutboundProxy() noexcept
{
   mHasOutboundProxy = false;
}

void 
Profile::setForceOutboundProxyOnAllRequestsEnabled(bool enabled) noexcept
{
   mForceOutboundProxyOnAllRequestsEnabled = enabled;
   mHasForceOutboundProxyOnAllRequestsEnabled = true;
}

bool 
Profile::getForceOutboundProxyOnAllRequestsEnabled() const noexcept
{
   // Fall through setting (if required)
   if (!mHasForceOutboundProxyOnAllRequestsEnabled && mBaseProfile)
   {
       return mBaseProfile->getForceOutboundProxyOnAllRequestsEnabled();
   }
   return mForceOutboundProxyOnAllRequestsEnabled;
}

void
Profile::unsetForceOutboundProxyOnAllRequestsEnabled() noexcept
{
   if (mBaseProfile) 
   {
      mHasForceOutboundProxyOnAllRequestsEnabled = false;
   }
   else
   {
      mHasForceOutboundProxyOnAllRequestsEnabled = true;
      mForceOutboundProxyOnAllRequestsEnabled = false;
   }
}

void 
Profile::setExpressOutboundAsRouteSetEnabled(bool enabled) noexcept
{
   mExpressOutboundAsRouteSetEnabled = enabled;
   mHasExpressOutboundAsRouteSetEnabled = true;
}

bool 
Profile::getExpressOutboundAsRouteSetEnabled() const noexcept
{
   // Fall through setting (if required)
   if (!mHasExpressOutboundAsRouteSetEnabled && mBaseProfile)
   {
       return mBaseProfile->getExpressOutboundAsRouteSetEnabled();
   }
   return mExpressOutboundAsRouteSetEnabled;
}

void
Profile::unsetExpressOutboundAsRouteSetEnabled() noexcept
{
   if (mBaseProfile) 
   {
      mHasExpressOutboundAsRouteSetEnabled = false;
   }
   else
   {
      mHasExpressOutboundAsRouteSetEnabled = true;
      mExpressOutboundAsRouteSetEnabled = false;
   }
}

void 
Profile::setRportEnabled(bool enabled) noexcept
{
   mRportEnabled = enabled;
   mHasRportEnabled = true;
}

bool 
Profile::getRportEnabled() const noexcept
{
   // Fall through setting (if required)
   if (!mHasRportEnabled && mBaseProfile)
   {
       return mBaseProfile->getRportEnabled();
   }
   return mRportEnabled;
}

void
Profile::unsetRportEnabled() noexcept
{
   if (mBaseProfile) 
   {
      mHasRportEnabled = false;
   }
   else // No Base profile - so return to default setting
   {
      mHasRportEnabled = true;
      mRportEnabled = true;
   }
}

void 
Profile::setUserAgent( const Data& userAgent )
{
   mUserAgent = userAgent;   
   mHasUserAgent = true;   
}

const Data& 
Profile::getUserAgent() const
{
   // Fall through setting (if required)
   if (!mHasUserAgent && mBaseProfile)
   {
       return mBaseProfile->getUserAgent();
   }
   resip_assert(mHasUserAgent);
   return mUserAgent;
}
 
bool 
Profile::hasUserAgent() const noexcept
{
   // Fall through setting (if required)
   if (!mHasUserAgent && mBaseProfile)
   {
       return mBaseProfile->hasUserAgent();
   }
   return mHasUserAgent;
}

void
Profile::unsetUserAgent() noexcept
{
   mHasUserAgent = false;
}

void 
Profile::setProxyRequires( const Tokens& proxyRequires )
{
   mProxyRequires = proxyRequires;   
   mHasProxyRequires = true;   
}

const Tokens& 
Profile::getProxyRequires() const
{
   // Fall through setting (if required)
   if (!mHasProxyRequires && mBaseProfile)
   {
       return mBaseProfile->getProxyRequires();
   }
   resip_assert(mHasProxyRequires);
   return mProxyRequires;
}
 
bool 
Profile::hasProxyRequires() const noexcept
{
   // Fall through setting (if required)
   if (!mHasProxyRequires && mBaseProfile)
   {
       return mBaseProfile->hasProxyRequires();
   }
   return mHasProxyRequires;
}

void
Profile::unsetProxyRequires() noexcept
{
   mHasProxyRequires = false;
}

void 
Profile::setKeepAliveTimeForDatagram(int keepAliveTime) noexcept
{
   mKeepAliveTimeForDatagram = keepAliveTime;
   mHasKeepAliveTimeForDatagram = true;
}

int 
Profile::getKeepAliveTimeForDatagram() const noexcept
{
   // Fall through setting (if required)
   if (!mHasKeepAliveTimeForDatagram && mBaseProfile)
   {
       return mBaseProfile->getKeepAliveTimeForDatagram();
   }
   return mKeepAliveTimeForDatagram;
}

void
Profile::unsetKeepAliveTimeForDatagram() noexcept
{
   if (mBaseProfile) 
   {
      mHasKeepAliveTimeForDatagram = false;
   }
   else // No Base profile - so return to default setting
   {
      mHasKeepAliveTimeForDatagram = true;
      mKeepAliveTimeForDatagram = 30; // 30 seconds.
   }
}

void 
Profile::setKeepAliveTimeForStream(int keepAliveTime) noexcept
{
   mKeepAliveTimeForStream = keepAliveTime;
   mHasKeepAliveTimeForStream = true;
}

int 
Profile::getKeepAliveTimeForStream() const noexcept
{
   // Fall through setting (if required)
   if (!mHasKeepAliveTimeForStream && mBaseProfile)
   {
       return mBaseProfile->getKeepAliveTimeForStream();
   }
   return mKeepAliveTimeForStream;
}

void
Profile::unsetKeepAliveTimeForStream() noexcept
{
   if (mBaseProfile) 
   {
      mHasKeepAliveTimeForStream = false;
   }
   else // No Base profile - so return to default setting
   {
      mHasKeepAliveTimeForStream = true;
      mKeepAliveTimeForStream = 180; // 3 minutes.
   }
}

void 
Profile::setFixedTransportPort(int fixedTransportPort) noexcept
{
   mFixedTransportPort = fixedTransportPort;
   mHasFixedTransportPort = true;
}

int 
Profile::getFixedTransportPort() const noexcept
{
   // Fall through setting (if required)
   if (!mHasFixedTransportPort && mBaseProfile)
   {
       return mBaseProfile->getFixedTransportPort();
   }
   return mFixedTransportPort;
}

void
Profile::unsetFixedTransportPort() noexcept
{
   if (mBaseProfile) 
   {
      mHasFixedTransportPort = false;
   }
   else // No Base profile - so return to default setting
   {
      mHasFixedTransportPort = true;
      mFixedTransportPort = 0;
   }
}

void 
Profile::setFixedTransportInterface(const Data& fixedTransportInterface)
{
   mFixedTransportInterface = fixedTransportInterface;
   mHasFixedTransportInterface = true;
}

const Data&
Profile::getFixedTransportInterface() const
{
   // Fall through setting (if required)
   if (!mHasFixedTransportInterface && mBaseProfile)
   {
       return mBaseProfile->getFixedTransportInterface();
   }
   return mFixedTransportInterface;
}

void
Profile::unsetFixedTransportInterface()
{
   if (mBaseProfile) 
   {
      mHasFixedTransportInterface = false;
   }
   else // No Base profile - so return to default setting
   {
      mHasFixedTransportInterface = true;
      mFixedTransportInterface = Data::Empty;
   }
}


void 
Profile::setRinstanceEnabled(bool enabled) noexcept
{
   mRinstanceEnabled = enabled;
   mHasRinstanceEnabled = true;
}

bool 
Profile::getRinstanceEnabled() const noexcept
{
   // Fall through setting (if required)
   if (!mHasRinstanceEnabled && mBaseProfile)
   {
       return mBaseProfile->getRinstanceEnabled();
   }
   return mRinstanceEnabled;
}

void
Profile::unsetRinstanceEnabled() noexcept
{
   if (mBaseProfile) 
   {
      mHasRinstanceEnabled = false;
   }
   else
   {
      mHasRinstanceEnabled = true;
      mRinstanceEnabled = true;
   }
}

	  ////If set then dum will add this MessageDecorator to all outbound messages
	  //virtual void setOutboundDecorator(std::shared_ptr<MessageDecorator> outboundDecorator);
	  //virtual std::shared_ptr<MessageDecorator> getOutboundDecorator();
	  //virtual void unsetOutboundDecorator();

void 
Profile::setOutboundDecorator(std::shared_ptr<MessageDecorator> outboundDecorator) noexcept
{
   mOutboundDecorator = std::move(outboundDecorator);
   mHasOutboundDecorator = true;
}

std::shared_ptr<MessageDecorator>
Profile::getOutboundDecorator() noexcept
{
   // Fall through setting (if required)
   if (!mHasOutboundDecorator && mBaseProfile)
   {
       return mBaseProfile->getOutboundDecorator();
   }
   return mOutboundDecorator;
}

void
Profile::unsetOutboundDecorator() noexcept
{
   if (mHasOutboundDecorator)
   {
      mOutboundDecorator.reset();
   }

   mHasOutboundDecorator = false;
}

void 
Profile::setMethodsParamEnabled(bool enabled) noexcept
{
   mMethodsParamEnabled = enabled;
   mHasMethodsParamEnabled = true;
}

bool 
Profile::getMethodsParamEnabled() const noexcept
{
   // Fall through setting (if required)
   if (!mHasMethodsParamEnabled && mBaseProfile)
   {
       return mBaseProfile->getMethodsParamEnabled();
   }
   return mMethodsParamEnabled;
}

void
Profile::unsetMethodsParamEnabled() noexcept
{
   if (mBaseProfile) 
   {
      mHasMethodsParamEnabled = false;
   }
   else
   {
      mHasMethodsParamEnabled = true;
      mMethodsParamEnabled = false;
   }
}

void 
Profile::setUserAgentCapabilities(const NameAddr& capabilities)
{
   mUserAgentCapabilities = capabilities;
   mHasUserAgentCapabilities = true;
}

const NameAddr&
Profile::getUserAgentCapabilities() const
{
   // Fall through setting (if required)
   if (!mHasUserAgentCapabilities && mBaseProfile)
   {
       return mBaseProfile->getUserAgentCapabilities();
   }
   resip_assert(mHasUserAgentCapabilities);
   return mUserAgentCapabilities;
}

bool
Profile::hasUserAgentCapabilities() const noexcept
{
   // Fall through setting (if required)
   if (!mHasUserAgentCapabilities && mBaseProfile)
   {
       return mBaseProfile->hasUserAgentCapabilities();
   }
   return mHasUserAgentCapabilities;
}

void
Profile::unsetUserAgentCapabilities() noexcept
{
   mHasUserAgentCapabilities = false;
}

void 
Profile::setExtraHeadersInReferNotifySipFragEnabled(bool enabled) noexcept
{
   mExtraHeadersInReferNotifySipFragEnabled = enabled;
   mHasExtraHeadersInReferNotifySipFragEnabled = true;
}

bool 
Profile::getExtraHeadersInReferNotifySipFragEnabled() const noexcept
{
   // Fall through setting (if required)
   if (!mHasExtraHeadersInReferNotifySipFragEnabled && mBaseProfile)
   {
       return mBaseProfile->getExtraHeadersInReferNotifySipFragEnabled();
   }
   return mExtraHeadersInReferNotifySipFragEnabled;
}

void
Profile::unsetExtraHeadersInReferNotifySipFragEnabled() noexcept
{
   if (mBaseProfile) 
   {
      mHasExtraHeadersInReferNotifySipFragEnabled = false;
   }
   else
   {
      mHasExtraHeadersInReferNotifySipFragEnabled = true;
      mExtraHeadersInReferNotifySipFragEnabled = false;
   }
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
