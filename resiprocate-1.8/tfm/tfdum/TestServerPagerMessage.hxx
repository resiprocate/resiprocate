#if !defined(TestServerPagerMessage_hxx)
#define TestServerPagerMessage_hxx

#include "TestUsage.hxx"
#include "resip/dum/Handles.hxx"
#include "tfm/EndPoint.hxx"
#include "DumUaAction.hxx"
#include "ServerPagerMessageEvent.hxx"

class DumUserAgent;
class MessageMatcher;

class TestServerPagerMessage : public TestUsage
{
   public:
      TestServerPagerMessage(DumUserAgent*);
      ~TestServerPagerMessage() {}

      resip::Data getName() const { return "TestServerPagerMessage"; }

      SendingAction<resip::ServerPagerMessageHandle>* accept(int statusCode = 200);
      SendingAction<resip::ServerPagerMessageHandle>* reject(int responseCode);
      CommonAction* end();
      CommonAction* send(resip::SharedPtr<resip::SipMessage> msg);

      bool isMyEvent(Event*);

      resip::ServerPagerMessageHandle getHandle() const { return mHandle; }

      ExpectBase* expect(ServerPagerMessageEvent::Type,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ServerPagerMessageEvent::Type,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ServerPagerMessageEvent::Type,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

   private:
      friend class DumUserAgent;
      resip::ServerPagerMessageHandle& getHandleRef() { return mHandle; }
      resip::ServerPagerMessageHandle mHandle;

};

#endif 
