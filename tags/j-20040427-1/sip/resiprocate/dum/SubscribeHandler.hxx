
class SubscribeHandler 
{

  public:
      virtual void onNewSubscription(ServerSubscription::Handle, SipMessage& sub);
      virtual void onRefresh(ServerSubscription::Handle, SipMessage& sub);
      virtual void onTerminated(ServerSubscription::Handle, SipMessage& sub);
      
      virtual void onRefreshRejected(ClientSubscription::Handle, SipMessage& rejection);
      virtual void onUpdatePending(ClientSubscription::Handle, SipMessage& notify);
      virtual void onUpdateActive(ClientSubscription::Handle, SipMessage& notify);      
      virtual void onTerminated(ClientSubscription::Handle, SipMessage& notify);      
};
