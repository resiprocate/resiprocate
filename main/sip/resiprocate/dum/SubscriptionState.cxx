#include "resiprocate/dum/SubscriptionState.hxx"
#include "resiprocate/os/Data.hxx"

using namespace resip;

Data SubscriptionStates[] = 
{
   "Invalid",
   "Init",
   "Pending",
   "Active",
   "Waiting",
   "Terminated",
   "Unknown"
};

Data SubscriptionStateReasons[] = 
{
   "Deactivated",
   "Probation",
   "Rejected",
   "Timeout",
   "Giveup",
   "NoResource",
   "Unknown"
};

const Data&
resip::getSubscriptionStateString(SubscriptionState state) 
{
   if (state > Unknown ||  state < Invalid) 
   {
      state = Unknown;
   }
   return SubscriptionStates[state];
}

const Data&
resip::getSubscriptionStateReasonString(Reason state) 
{
//    if (state > Unknown ||  state < Deactivated) 
//    {
//       state = Unknown;
//    }
   return SubscriptionStateReasons[state];
}
