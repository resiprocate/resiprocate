#if !defined(RESIP_SERVERSUBSCRIPTION_HXX)
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

      typedef enum
      {
         Rejected,
         NoResource,
         Deactivated,
         Probation,
         Timeout,
         Giveup
      } TerminateReason;

      void acceptPending(SipMessage* notify);
      void acceptActive(SipMessage* notify);
      void update(const Contents* document);
      void end(TerminateReason reason);
      void reject(int responseCode);
      
      void setCurrentEventDocument(const Contents* document);
      void setSubscriptionState(SubscriptionState state,Reason reason);
};
 
}

#endif
