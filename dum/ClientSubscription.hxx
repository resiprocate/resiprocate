#if !defined(RESIP_CLIENTSUBSCRIPTION_HXX)
#define RESIP_CLIENTSUBSCRIPTION_HXX

namespace resip
{

class ClientSubscription: public BaseUsage
{
   public:
      class Handle
      {
      };
      
      void end();
      void requestRefresh();

      bool matches(const SipMessage& subOrNotify);
      void process(const SipMessage& subOrNotify);
      
   private:
      Data mEventType;
      Data mSubscriptionId;
      SubscriptionState mSubState;
      const Contents* mCurrentEventDocument;
      UInt64 mExpirationTime;
};
 
}

#endif
