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

Data TerminateReasons[] = 
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
resip::getTerminateReasonString(TerminateReason state) 
{
//    if (state > Unknown ||  state < Deactivated) 
//    {
//       state = Unknown;
//    }
   return TerminateReasons[state];
}
