#include "SubscriptionCreator.hxx"
#include "DialogUsageManager.hxx"
#include "Profile.hxx"

using namespace resip;

SubscriptionCreator::SubscriptionCreator(DialogUsageManager& dum, const Data& event)
   : BaseCreator(dum)
{
   makeInitialRequest(mDum.getProfile()->getDefaultAor(), SUBSCRIBE);
}
