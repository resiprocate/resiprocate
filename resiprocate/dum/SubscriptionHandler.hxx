#if !defined(RESIP_SUBSCRIPTIONHANDLER_HXX)
#define RESIP_SUBSCRIPTIONHANDLER_HXX

#include "resiprocate/dum/Handles.hxx"

namespace resip
{
class SipMessage;

class ClientSubscriptionHandler
{
  public:
      virtual void onRefreshRejected(ClientSubscriptionHandle, const SipMessage& rejection)=0;
      virtual void onUpdatePending(ClientSubscriptionHandle, const SipMessage& notify)=0;
      virtual void onUpdateActive(ClientSubscriptionHandle, const SipMessage& notify)=0;      
      virtual void onTerminated(ClientSubscriptionHandle, const SipMessage& notify)=0;   
      //not sure if this has any value
      virtual void onNewSubscription(ClientSubscriptionHandle, const SipMessage& sub)=0; 
};

class ServerSubscriptionHandler
{
  public:
      virtual void onNewSubscription(ServerSubscriptionHandle, const SipMessage& sub)=0;
      virtual void onRefresh(ServerSubscriptionHandle, const SipMessage& sub)=0;
      virtual void onTerminated(ServerSubscriptionHandle, const SipMessage& sub)=0;
};
 
}

#endif
