#if !defined(TestClientPagerMessage_hxx)
#define TestClientPagerMessage_hxx

#include "TestUsage.hxx"
#include "resip/dum/Handles.hxx"
#include "tfm/EndPoint.hxx"
#include "ClientPagerMessageEvent.hxx"

class DumUserAgent;
class MessageMatcher;

class CommonAction;

class TestClientPagerMessage : public TestUsage
{
   public:
      TestClientPagerMessage(DumUserAgent*, resip::ClientPagerMessageHandle);
      ~TestClientPagerMessage() {}

      resip::Data getName() const { return "TestClientPagerMessage"; }

      CommonAction* page(std::auto_ptr<resip::Contents>& contents, resip::DialogUsageManager::EncryptionLevel level=resip::DialogUsageManager::None);
      CommonAction* end();

      bool isMyEvent(Event*);

      resip::ClientPagerMessageHandle getHandle() const { return mHandle; }

      ExpectBase* expect(ClientPagerMessageEvent::Type,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ClientPagerMessageEvent::Type,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ClientPagerMessageEvent::Type,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

   private:
      friend class DumUserAgent;
      resip::ClientPagerMessageHandle& getHandleRef() { return mHandle; }
      resip::ClientPagerMessageHandle mHandle;

};

#endif 
