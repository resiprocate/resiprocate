
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

      mHasDefaultSubscriptionExpires = true;
      mDefaultSubscriptionExpires = 3600; // 1 hour

      mHasDefaultPublicationExpires = true;
      mDefaultPublicationExpires = 3600; // 1 hour

      mHasDefaultStaleCallTime = true;
      mDefaultStaleCallTime = 3600;       // 1 hour

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

	  mHasLooseToTagMatching = true;
      mLooseToTagMatching = false;

	  mHasRportEnabled = true;
      mRportEnabled = true;

      mHasUserAgent = false;

      mHasOverrideHostPort = false;
   }
   else
   {
      mHasDefaultRegistrationExpires = false;
      mHasDefaultSubscriptionExpires = false;
      mHasDefaultPublicationExpires = false;
      mHasDefaultStaleCallTime = false;
      mHasDefaultSessionExpires = false;
      mHasDefaultSessionTimerMode = false;
      mHas1xxRetransmissionTime = false;
      mHasOutboundProxy = false;
	  mHasAdvertisedCapabilities = false;
	  mHasLooseToTagMatching = false;
	  mHasRportEnabled = false;
      mHasUserAgent = false;
      mHasOverrideHostPort = false;
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
   // !slg! do we set mHasAdvertisedCapabilities = false and allow fall through?  For now we assume that clearing means to not advertised any headers
   mHasAdvertisedCapabilities = true;
   return mAdvertisedCapabilities.clear();
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
Profile::setLooseToTagMatching(bool enabled)
{
   mLooseToTagMatching = enabled;
   mHasLooseToTagMatching = true;
}

bool 
Profile::getLooseToTagMatching() const
{
   // Fall through seting (if required)
   if(!mHasLooseToTagMatching && mBaseProfile)
   {
       return mBaseProfile->getLooseToTagMatching();
   }
   return mLooseToTagMatching;
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

   
