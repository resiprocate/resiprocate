#ifndef RESIP_SubscriptionState_hxx
#define RESIP_SubscriptionState_hxx

namespace resip
{
class Data;

typedef enum 
{
   Invalid=0,
   Init,
   Pending,
   Active,
   Waiting,
   Terminated,
   Unknown
} SubscriptionState;

typedef enum 
{
   //Invalid=0,
   Deactivated=0,
   Probation,
   Rejected,
   Timeout,
   Giveup,
   NoResource
} Reason;


const Data&
resip::getSubscriptionStateString(SubscriptionState state);

const Data&
resip::getSubscriptionStateReasonString(Reason state);


}

#endif
