#if !defined(TestClientPublication_hxx)
#define TestClientPublication_hxx

#include "TestUsage.hxx"
#include "resip/dum/Handles.hxx"
#include "tfm/EndPoint.hxx"
#include "ClientPublicationEvent.hxx"

class DumUserAgent;
class MessageMatcher;

class CommonAction;

class TestClientPublication : public TestUsage
{
   public:
      TestClientPublication(DumUserAgent*);
      virtual ~TestClientPublication();

      resip::Data getName() const { return "TestClientPublication"; }

      CommonAction* refresh(unsigned int expiration=0);
      CommonAction* update(const resip::Contents* body);
      CommonAction* end();

      bool isMyEvent(Event*);

      resip::ClientPublicationHandle getHandle() const { return mHandle; }

      ExpectBase* expect(ClientPublicationEvent::Type,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ClientPublicationEvent::Type,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ClientPublicationEvent::Type,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

   private:
      friend class DumUserAgent;
      resip::ClientPublicationHandle& getHandleRef() { return mHandle; }
      resip::ClientPublicationHandle mHandle;

};

#endif 
