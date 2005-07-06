#include "SubscriptionCreator.hxx"
#include "DialogUsageManager.hxx"
#include "Profile.hxx"

using namespace resip;

SubscriptionCreator::SubscriptionCreator(DialogUsageManager& dum, const NameAddr& target, const NameAddr& from, const Data& event, int subscriptionTime)
   : BaseCreator(dum),
     mRefreshInterval(-1)
{
   makeInitialRequest(target, from, SUBSCRIBE);

   mLastRequest.header(h_Event).value() = event; 
   mLastRequest.header(h_Expires).value() = subscriptionTime;
}

SubscriptionCreator::SubscriptionCreator(DialogUsageManager& dum, const NameAddr& target, const NameAddr& from, const Data& event, int subscriptionTime, int refreshInterval)
   : BaseCreator(dum),
     mRefreshInterval(refreshInterval)
{
   makeInitialRequest(target, from, SUBSCRIBE);

   mLastRequest.header(h_Event).value() = event; 
   mLastRequest.header(h_Expires).value() = subscriptionTime;
}

bool 
SubscriptionCreator::hasRefreshInterval() const
{
   return mRefreshInterval > 0;   
}

int 
SubscriptionCreator::getRefreshInterval() const
{
   return mRefreshInterval;
}
