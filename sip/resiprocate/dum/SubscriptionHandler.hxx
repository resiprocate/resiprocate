#if !defined(RESIP_SUBSCRIPTIONHANDLER_HXX)
#define RESIP_SUBSCRIPTIONHANDLER_HXX

#include "resiprocate/dum/ClientSubscription.hxx"
#include "resiprocate/dum/ServerSubscription.hxx"

namespace resip
{
class SipMessage;

class ClientSubscriptionHandler
{
  public:
      virtual void onRefreshRejected(ClientSubscription::Handle, SipMessage& rejection)=0;
      virtual void onUpdatePending(ClientSubscription::Handle, SipMessage& notify)=0;
      virtual void onUpdateActive(ClientSubscription::Handle, SipMessage& notify)=0;      
      virtual void onTerminated(ClientSubscription::Handle, SipMessage& notify)=0;      
};

class ServerSubscriptionHandler
{
  public:
      virtual void onNewSubscription(ServerSubscription::Handle, SipMessage& sub)=0;
      virtual void onRefresh(ServerSubscription::Handle, SipMessage& sub)=0;
      virtual void onTerminated(ServerSubscription::Handle, SipMessage& sub)=0;
};
 
}

