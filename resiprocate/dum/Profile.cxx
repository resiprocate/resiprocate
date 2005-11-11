
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/HeaderTypes.hxx"

using namespace resip;
#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

Profile::Profile(Profile *baseProfile) : 
   mBaseProfile(baseProfile)
{
   // Default settings - if a fall through profile was not provided
   if(!baseProfile)
   {
      mHasDefaultRegistrationExpires = true;
      mDefaultRegistrationExpires = 3600; // 1 hour

      mHasDefaultMaxRegistrationExpires = true;
      mDefaultMaxRegistrationExpires = 0; // No restriction

      mHasDefaultRegistrationRetryInterval = true;
      mDefaultRegistrationRetryInterval = 0; // Retries disabled

      mHasDefaultSubscriptionExpires = true;
      mDefaultSubscriptionExpires = 3600; // 1 hour

      mHasDefaultPublicationExpires = true;
      mDefaultPublicationExpires = 3600;  // 1 hour

      mHasDefaultStaleCallTime = true;
      mDefaultStaleCallTime = 180;        // 3 minutes

      mHasDefaultSessionExpires = true;
      mDefaultSessionExpires = 1800;      // 30 minutes

      mHasDefaultSessionTimerMode = true;
      mDefaultSessionTimerMode = Profile::PreferUACRefreshes;

      mHas1xxRetransmissionTime = true;
      m1xxRetransmissionTime = 60;  // RFC3261 13.3.1 specifies this timeout should be 1 minute

      mHasOutboundProxy = false;

	  mHasAdvertisedCapabilities = true;
      addAdvertisedCapability(Headers::Allow);  
      addAdvertisedCapability(Headers::Supported);  

	  mHasRportEnabled = true;
      mRportEnabled = true;

      mHasUserAgent = false;

      mHasOverrideHostPort = false;

      mHasKeepAliveTime = true;
      mKeepAliveTime = 30;      // 30 seconds

      mHasFixedTransportPort = true;
      mFixedTransportPort = 0;
   }
   else
   {
      mHasDefaultRegistrationExpires = false;
      mHasDefaultMaxRegistrationExpires = false;
      mHasDefaultRegistrationRetryInterval = false;
      mHasDefaultSubscriptionExpires = false;
      mHasDefaultPublicationExpires = false;
      mHasDefaultStaleCallTime = false;
      mHasDefaultSessionExpires = false;
      mHasDefaultSessionTimerMode = false;
      mHas1xxRetransmissionTime = false;
      mHasOutboundProxy = false;
	  mHasAdvertisedCapabilities = false;
	  mHasRportEnabled = false;
      mHasUserAgent = false;
      mHasOverrideHostPort = false;
      mHasKeepAliveTime = false;
      mHasFixedTransportPort = false;
   }
}

Profile::~Profile()
{
}

void
Profile::setDefaultRegistrationTime(int secs)
{
   mDefaultRegistrationExpires = secs;
   mHasDefaultRegistrationExpires = true;
}

int 
Profile::getDefaultRegistrationTime() const
{
   // Fall through seting (if required)
   if(!mHasDefaultRegistrationExpires && mBaseProfile)
   {
       return mBaseProfile->getDefaultRegistrationTime();
   }
   return mDefaultRegistrationExpires;
}

void
Profile::unsetDefaultRegistrationTime()
{
   if(mBaseProfile) 
   {
      mHasDefaultRegistrationExpires = false;
   }
}

void
Profile::setDefaultMaxRegistrationTime(int secs)
{
   mDefaultMaxRegistrationExpires = secs;
   mHasDefaultMaxRegistrationExpires = true;
}

int 
Profile::getDefaultMaxRegistrationTime() const
{
   // Fall through seting (if required)
   if(!mHasDefaultMaxRegistrationExpires && mBaseProfile)
   {
       return mBaseProfile->getDefaultMaxRegistrationTime();
   }
   return mDefaultMaxRegistrationExpires;
}

void
Profile::unsetDefaultMaxRegistrationTime()
{
   if(mBaseProfile) 
   {
      mHasDefaultMaxRegistrationExpires = false;
   }
}

void
Profile::setDefaultRegistrationRetryTime(int secs)
{
   mDefaultRegistrationRetryInterval = secs;
   mHasDefaultRegistrationRetryInterval = true;
}

int 
Profile::getDefaultRegistrationRetryTime() const
{
   // Fall through seting (if required)
   if(!mHasDefaultRegistrationRetryInterval && mBaseProfile)
   {
       return mBaseProfile->getDefaultRegistrationRetryTime();
   }
   return mDefaultRegistrationRetryInterval;
}

void
Profile::unsetDefaultRegistrationRetryTime()
{
   if(mBaseProfile) 
   {
      mHasDefaultRegistrationRetryInterval = false;
   }
}

void
Profile::setDefaultSubscriptionTime(int secs)
{
   mDefaultSubscriptionExpires = secs;
   mHasDefaultSubscriptionExpires = true;
}

int 
Profile::getDefaultSubscriptionTime() const
{
   // Fall through seting (if required)
   if(!mHasDefaultSubscriptionExpires && mBaseProfile)
   {
       return mBaseProfile->getDefaultSubscriptionTime();
   }
   return mDefaultSubscriptionExpires;
}

void
Profile::unsetDefaultSubscriptionTime()
{
   if(mBaseProfile) 
   {
      mHasDefaultSubscriptionExpires = false;
   }
}

void
Profile::setDefaultPublicationTime(int secs)
{
   mDefaultPublicationExpires = secs;
   mHasDefaultPublicationExpires = true;
}

int 
Profile::getDefaultPublicationTime() const
{
   // Fall through seting (if required)
   if(!mHasDefaultPublicationExpires && mBaseProfile)
   {
       return mBaseProfile->getDefaultPublicationTime();
   }
   return mDefaultPublicationExpires;
}

void
Profile::unsetDefaultPublicationTime()
{
   if(mBaseProfile) 
   {
      mHasDefaultPublicationExpires = false;
   }
}

void
Profile::setDefaultStaleCallTime(int secs)
{
   mDefaultStaleCallTime = secs;
   mHasDefaultStaleCallTime = true;
}

int 
Profile::getDefaultStaleCallTime() const
{
   // Fall through seting (if required)
   if(!mHasDefaultStaleCallTime && mBaseProfile)
   {
       return mBaseProfile->getDefaultStaleCallTime();
   }
   return mDefaultStaleCallTime;
}

void
Profile::unsetDefaultStaleCallTime()
{
   if(mBaseProfile) 
   {
      mHasDefaultStaleCallTime = false;
   }
}

void
Profile::setDefaultSessionTime(int secs)
{
   mDefaultSessionExpires = secs;
   mHasDefaultSessionExpires = true;
}

int 
Profile::getDefaultSessionTime() const
{
   // Fall through seting (if required)
   if(!mHasDefaultSessionExpires && mBaseProfile)
   {
       return mBaseProfile->getDefaultSessionTime();
   }
   return mDefaultSessionExpires;
}

void
Profile::unsetDefaultSessionTime()
{
   if(mBaseProfile) 
   {
      mHasDefaultSessionExpires = false;
   }
}

void
Profile::setDefaultSessionTimerMode(Profile::SessionTimerMode mode)
{
   mDefaultSessionTimerMode = mode;
   mHasDefaultSessionTimerMode = true;
}

Profile::SessionTimerMode 
Profile::getDefaultSessionTimerMode() const
{
   // Fall through seting (if required)
   if(!mHasDefaultSessionTimerMode && mBaseProfile)
   {
       return mBaseProfile->getDefaultSessionTimerMode();
   }
   return mDefaultSessionTimerMode;
}

void
Profile::unsetDefaultSessionTimerMode()
{
   if(mBaseProfile) 
   {
      mHasDefaultSessionTimerMode = false;
   }
}

void
Profile::set1xxRetransmissionTime(int secs)
{
   m1xxRetransmissionTime = secs;
   mHas1xxRetransmissionTime = true;
}

int 
Profile::get1xxRetransmissionTime() const
{
   // Fall through seting (if required)
   if(!mHas1xxRetransmissionTime && mBaseProfile)
   {
       return mBaseProfile->get1xxRetransmissionTime();
   }
   return m1xxRetransmissionTime;
}

void
Profile::unset1xxRetransmissionTime()
{
   if(mBaseProfile) 
   {
      mHas1xxRetransmissionTime = false;
   }
}

void 
Profile::setOverrideHostAndPort(const Uri& hostPort)
{
   mOverrideHostPort = hostPort;   
   mHasOverrideHostPort = true;   
}

bool 
Profile::hasOverrideHostAndPort() const
{
   // Fall through seting (if required)
   if(!mHasOverrideHostPort && mBaseProfile)
   {
       return mBaseProfile->hasOverrideHostAndPort();
   }
   return mHasOverrideHostPort;
}

const Uri& 
Profile::getOverrideHostAndPort() const
{
   // Fall through seting (if required)
   if(!mHasOverrideHostPort && mBaseProfile)
   {
       return mBaseProfile->getOverrideHostAndPort();
   }
   return mOverrideHostPort;
}

void
Profile::unsetOverrideHostAndPort()
{
   if(mBaseProfile) 
   {
      mHasOverrideHostPort = false;
   }
}

void 
Profile::addAdvertisedCapability(const Headers::Type header)
{
   assert(header == Headers::Allow ||
		  header == Headers::AcceptEncoding ||
		  header == Headers::AcceptLanguage ||
		  header == Headers::Supported);

   mAdvertisedCapabilities.insert(header);
   mHasAdvertisedCapabilities = true;
}
 
bool 
Profile::isAdvertisedCapability(const Headers::Type header) const
{
   // Fall through seting (if required)
   if(!mHasAdvertisedCapabilities && mBaseProfile)
   {
       return mBaseProfile->isAdvertisedCapability(header);
   }
   return mAdvertisedCapabilities.count(header) != 0;
}

void 
Profile::clearAdvertisedCapabilities(void)
{
   mHasAdvertisedCapabilities = true;
   return mAdvertisedCapabilities.clear();
}

void
Profile::unsetAdvertisedCapabilities()
{
   if(mBaseProfile) 
   {
      mHasAdvertisedCapabilities = false;
   }
}

void 
Profile::setOutboundProxy( const Uri& uri )
{
   mOutboundProxy = NameAddr(uri);
   mHasOutboundProxy = true;
}

const NameAddr&
Profile::getOutboundProxy() const
{
   // Fall through seting (if required)
   if(!mHasOutboundProxy && mBaseProfile)
   {
       return mBaseProfile->getOutboundProxy();
   }
   assert(mHasOutboundProxy);
   return mOutboundProxy;
}

bool
Profile::hasOutboundProxy() const
{
   // Fall through seting (if required)
   if(!mHasOutboundProxy && mBaseProfile)
   {
       return mBaseProfile->hasOutboundProxy();
   }
   return mHasOutboundProxy;
}

void
Profile::unsetOutboundProxy()
{
   if(mBaseProfile) 
   {
      mHasOutboundProxy = false;
   }
}

void 
Profile::setRportEnabled(bool enabled)
{
   mRportEnabled = enabled;
   mHasRportEnabled = true;
}

bool 
Profile::getRportEnabled() const
{
   // Fall through seting (if required)
   if(!mHasRportEnabled && mBaseProfile)
   {
       return mBaseProfile->getRportEnabled();
   }
   return mRportEnabled;
}

void
Profile::unsetRportEnabled()
{
   if(mBaseProfile) 
   {
      mHasRportEnabled = false;
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
   // Fall through seting (if required)
   if(!mHasUserAgent && mBaseProfile)
   {
       return mBaseProfile->getUserAgent();
   }
   assert(mHasUserAgent);
   return mUserAgent;
}
 
bool 
Profile::hasUserAgent() const
{
   // Fall through seting (if required)
   if(!mHasUserAgent && mBaseProfile)
   {
       return mBaseProfile->hasUserAgent();
   }
   return mHasUserAgent;
}

void
Profile::unsetUserAgent()
{
   if(mBaseProfile) 
   {
      mHasUserAgent = false;
   }
}

void 
Profile::setKeepAliveTime(int keepAliveTime)
{
   mKeepAliveTime = keepAliveTime;
   mHasKeepAliveTime = true;
}

int 
Profile::getKeepAliveTime() const
{
   // Fall through seting (if required)
   if(!mHasKeepAliveTime && mBaseProfile)
   {
       return mBaseProfile->getKeepAliveTime();
   }
   return mKeepAliveTime;
}

void
Profile::unsetKeepAliveTime()
{
   if(mBaseProfile) 
   {
      mHasKeepAliveTime = false;
   }
}

void 
Profile::setFixedTransportPort(int fixedTransportPort)
{
   mFixedTransportPort = fixedTransportPort;
   mHasFixedTransportPort = true;
}

int 
Profile::getFixedTransportPort() const
{
   // Fall through seting (if required)
   if(!mHasFixedTransportPort && mBaseProfile)
   {
       return mBaseProfile->getFixedTransportPort();
   }
   return mFixedTransportPort;
}

void
Profile::unsetFixedTransportPort()
{
   if(mBaseProfile) 
   {
      mHasFixedTransportPort = false;
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
