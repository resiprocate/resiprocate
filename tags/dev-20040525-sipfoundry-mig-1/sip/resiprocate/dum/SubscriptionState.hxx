#ifndef RESIP_SubscriptionState_hxx
#define RESIP_SubscriptionState_hxx

namespace resip
{

typedef enum 
{
   Invalid=0,
   Init,
   Pending,
   Active,
   Waiting,
   Terminated
} SubscriptionState;

typedef enum 
{
   //Invalid=0,
   Deactivated=0,
   Probation,
   Rejected,
   Timeout,
   Giveup,
   NoResource,
   Unknown
} Reason;

}

#endif
