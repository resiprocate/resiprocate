
class SubscribeHandler 
{

  public:
      virtual void onSubscribe(ServerSubscription::Handle sub, SipMessage& msg);

      virtual void onNotifyReady(ServerSubscription::Handle sub,
                                 SipMessage& notify)
      {
         sub->sendNotify(notify);
      }    
      
};
