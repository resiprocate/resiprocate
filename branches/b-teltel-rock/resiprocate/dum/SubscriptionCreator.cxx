#include "SubscriptionCreator.hxx"
#include "DialogUsageManager.hxx"

using namespace resip;

SubscriptionCreator::SubscriptionCreator(DialogUsageManager& dum, const NameAddr& target, UserProfile &userProfile, const Data& event, int subscriptionTime)
   : BaseCreator(dum, userProfile),
     mRefreshInterval(-1)
{
   makeInitialRequest(target, SUBSCRIBE);

   mLastRequest.header(h_Event).value() = event; 
   mLastRequest.header(h_Expires).value() = subscriptionTime;
}

SubscriptionCreator::SubscriptionCreator(DialogUsageManager& dum, const NameAddr& target, UserProfile &userProfile, const Data& event, int subscriptionTime, int refreshInterval)
   : BaseCreator(dum, userProfile),
     mRefreshInterval(refreshInterval)
{
   makeInitialRequest(target, SUBSCRIBE);

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
