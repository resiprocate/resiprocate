d#if !defined(RESIP_SERVERSUBSCRIPTION_HXX)
#define RESIP_SERVERSUBSCRIPTION_HXX

namespace resip
{

class ServerSubscription: public BaseUsage 
{
   public:
      class Handle
      {
      };
      
      ServerSubscription(DialogUsageManager& dum, const SipMessage& req);
    
      void sendResponse(SipMessage& msg);
      void sendNotify(SipMessage& msg);
      void setCurrentEventDocument(const Contents* document);
      void setSubscriptionState(SubscriptionState state,Reason reason);
};
 
}

#endif
