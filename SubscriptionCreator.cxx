#include "SubscriptionCreator.hxx"
#include "DialogUsageManager.hxx"
#include "Profile.hxx"

using namespace resip;

SubscriptionCreator::SubscriptionCreator(DialogUsageManager& dum, const NameAddr& target, const NameAddr& from, const Data& event)
   : BaseCreator(dum)
{
   makeInitialRequest(target, from, SUBSCRIBE);

   mLastRequest.header(h_Event).value() = event; 
   mLastRequest.header(h_Expires).value() =  mDum.getProfile()->getDefaultSubscriptionTime();
}

