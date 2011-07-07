#if !defined(TestServerOutOfDialogReq_hxx)
#define TestServerOutOfDialogReq_hxx

#include "TestUsage.hxx"
#include "resip/dum/Handles.hxx"
#include "tfm/EndPoint.hxx"
#include "ServerOutOfDialogReqEvent.hxx"

class DumUserAgent;
class MessageMatcher;

class CommonAction;

class TestServerOutOfDialogReq : public TestUsage
{
   public:
      TestServerOutOfDialogReq(DumUserAgent*);
      ~TestServerOutOfDialogReq() {}

      resip::Data getName() const { return "TestServerOutOfDialogReq"; }

      SendingAction<resip::ServerOutOfDialogReqHandle>* accept(int statusCode = 200);
      SendingAction<resip::ServerOutOfDialogReqHandle>* reject(int statusCode);
      SendingAction<resip::ServerOutOfDialogReqHandle>* answerOptions();

      CommonAction* send(resip::SharedPtr<resip::SipMessage> msg);
      CommonAction* end();

      bool isMyEvent(Event*);

      resip::ServerOutOfDialogReqHandle getHandle() const { return mHandle; }

      ExpectBase* expect(ServerOutOfDialogReqEvent::Type,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ServerOutOfDialogReqEvent::Type,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ServerOutOfDialogReqEvent::Type,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

   private:
      friend class DumUserAgent;
      resip::ServerOutOfDialogReqHandle& getHandleRef() { return mHandle; }
      resip::ServerOutOfDialogReqHandle mHandle;

};

#endif 
