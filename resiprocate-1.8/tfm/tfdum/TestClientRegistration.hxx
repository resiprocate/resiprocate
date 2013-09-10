#if !defined(TestClientRegistration_hxx)
#define TestClientRegistration_hxx

#include "TestUsage.hxx"
#include "resip/dum/Handles.hxx"
#include "tfm/EndPoint.hxx"
#include "ClientRegistrationEvent.hxx"

class DumUserAgent;
class MessageMatcher;

class CommonAction;

class TestClientRegistration : public TestUsage
{
   public:
      TestClientRegistration(DumUserAgent*);
      ~TestClientRegistration() {};

      resip::Data getName() const { return "TestClientRegistration"; }
      
      CommonAction* addBinding(const resip::NameAddr& contact);
      CommonAction* addBinding(const resip::NameAddr& contact, int registrationTime);
      CommonAction* removeBinding(const resip::NameAddr& contact);
      CommonAction* removeAll(bool stopRegisteringWhenDone=false);
      CommonAction* removeMyBindings(bool stopRegisteringWhenDone=false);
      CommonAction* requestRefresh(int expires = -1);  
      CommonAction* stopRegistering();
      CommonAction* end();

      bool isMyEvent(Event*);

      resip::ClientRegistrationHandle getHandle() const { return mHandle; }

      ExpectBase* expect(ClientRegistrationEvent::Type,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ClientRegistrationEvent::Type,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ClientRegistrationEvent::Type,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

   private:
      friend class DumUserAgent;
      resip::ClientRegistrationHandle& getHandleRef() { return mHandle; }
      resip::ClientRegistrationHandle mHandle;

};

#endif 
