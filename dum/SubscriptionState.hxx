#ifndef SUBSCRIPTIONSTATE_HXX
#define SUSBCRIPTIONSTATE_HXX

namespace resip
{
   typedef enum {
     Invalid=0,
     Init,
     Pending,
     Active,
     Waiting,
     Terminated
   } SubscriptionState;

   typedef enum {
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
