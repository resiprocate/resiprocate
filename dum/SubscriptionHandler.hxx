#if !defined(RESIP_SUBSCRIPTIONHANDLER_HXX)
#define RESIP_SUBSCRIPTIONHANDLER_HXX

#include "resiprocate/dum/Handles.hxx"

namespace resip
{
class SipMessage;

class ClientSubscriptionHandler
{
  public:
      virtual void onRefreshRejected(ClientSubscriptionHandle, SipMessage& rejection)=0;
      virtual void onUpdatePending(ClientSubscriptionHandle, SipMessage& notify)=0;
      virtual void onUpdateActive(ClientSubscriptionHandle, SipMessage& notify)=0;      
      virtual void onTerminated(ClientSubscriptionHandle, SipMessage& notify)=0;      
};

class ServerSubscriptionHandler
{
  public:
      virtual void onNewSubscription(ServerSubscriptionHandle, SipMessage& sub)=0;
      virtual void onRefresh(ServerSubscriptionHandle, SipMessage& sub)=0;
      virtual void onTerminated(ServerSubscriptionHandle, SipMessage& sub)=0;
};
 
}

