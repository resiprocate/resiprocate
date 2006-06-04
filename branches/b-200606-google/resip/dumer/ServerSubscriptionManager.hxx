#if !defined(DUM_ServerSubscriptionManager_hxx)
#define DUM_ServerSubscriptionManager_hxx

namespace resip
{

class ServerSubscriptionManager
{
   public:

   private:
      // from Event-Type+document-aor -> ServerSubscription
      // Managed by ServerSubscription
      typedef std::multimap<Data, ServerSubscription*> ServerSubscriptions;
      ServerSubscriptions mServerSubscriptions;
};

}

#endif
