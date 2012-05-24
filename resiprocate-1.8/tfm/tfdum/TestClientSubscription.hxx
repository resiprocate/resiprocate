#if !defined(TestClientSubscription_hxx)
#define TestClientSubscription_hxx

#include "TestUsage.hxx"
#include "resip/dum/Handles.hxx"
#include "tfm/EndPoint.hxx"
#include "ClientSubscriptionEvent.hxx"

class DumUserAgent;
class MessageMatcher;

class CommonAction;

class TestClientSubscription : public TestUsage
{
   public:
      TestClientSubscription(DumUserAgent*);
      ~TestClientSubscription() {}

      resip::Data getName() const { return "TestClientSubscription"; }

      CommonAction* acceptUpdate(int statusCode = 200, const char* reason=0);
      CommonAction* rejectUpdate(int statusCode = 400, const resip::Data& reasonPhrase = resip::Data::Empty);
      CommonAction* requestRefresh(int expires = -1);
      CommonAction* end();

      bool isMyEvent(Event*);

      resip::ClientSubscriptionHandle getHandle() const { return mHandle; }

      ExpectBase* expect(ClientSubscriptionEvent::Type,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ClientSubscriptionEvent::Type,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ClientSubscriptionEvent::Type,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

   private:
      friend class DumUserAgent;
      resip::ClientSubscriptionHandle& getHandleRef() { return mHandle; }
      resip::ClientSubscriptionHandle mHandle;

};

#endif 
