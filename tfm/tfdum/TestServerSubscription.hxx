#if !defined(TestServerSubscription_hxx)
#define TestServerSubscription_hxx

#include "TestUsage.hxx"
#include "resip/dum/Handles.hxx"
#include "tfm/EndPoint.hxx"
#include "ServerSubscriptionEvent.hxx"
#include "DumUaAction.hxx"
#include "resip/dum/Handles.hxx"

class DumUserAgent;
class MessageMatcher;
class ServerSubscriptionAction;

class TestServerSubscription : public TestUsage
{
   public:
      TestServerSubscription(DumUserAgent*);
      ~TestServerSubscription() {}

      resip::Data getName() const { return "TestServerSubscription"; }

      SendingAction<resip::ServerSubscriptionHandle>* accept(int statusCode = 200);
      SendingAction<resip::ServerSubscriptionHandle>* reject(int responseCode);
      SendingAction<resip::ServerSubscriptionHandle>* neutralNotify();
      SendingAction<resip::ServerSubscriptionHandle>* update(const resip::Contents* document);

      CommonAction* setSubscriptionState(resip::SubscriptionState state);
      CommonAction* end();
      CommonAction* send(resip::SharedPtr<resip::SipMessage> msg);

      bool isMyEvent(Event*);

      resip::ServerSubscriptionHandle getHandle() const { return mHandle; }

      ExpectBase* expect(ServerSubscriptionEvent::Type,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ServerSubscriptionEvent::Type,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ServerSubscriptionEvent::Type,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

   private:
      friend class DumUserAgent;
      resip::ServerSubscriptionHandle& getHandleRef() { return mHandle; }
      resip::ServerSubscriptionHandle mHandle;

};

#endif 
