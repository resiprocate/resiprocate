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
} TerminateReason;


const Data&
getSubscriptionStateString(SubscriptionState state);

const Data&
getTerminateReasonString(TerminateReason state);


}

#endif
