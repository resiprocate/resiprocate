#include "resiprocate/dum/SubscriptionState.hxx"
#include "resiprocate/os/Data.hxx"

using namespace resip;

Data SubscriptionStates[] = 
{
   "invalid",
   "init",
   "pending",
   "active",
   "waiting",
   "terminated",
   "unknown"
};

Data TerminateReasons[] = 
{
   "deactivated",
   "probation",
   "rejected",
   "timeout",
   "giveup",
   "noResource",
   "unknown"
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
resip::getTerminateReasonString(TerminateReason state) 
{
//    if (state > Unknown ||  state < Deactivated) 
//    {
//       state = Unknown;
//    }
   return TerminateReasons[state];
}
