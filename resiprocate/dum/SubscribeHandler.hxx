
class SubscribeHandler {

  public:
    void onSubscribe(ServerSubscription::Handle sub, SipMessage& msg);

    virtual void 
      onNotifyReady(ServerSubscription::Handle sub,
                              SipMessage& notify)
      {
        sub->sendNotify(notify);
      }    

};
