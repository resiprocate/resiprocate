#if !defined(TestDialogEvent_hxx)
#define TestDialogEvent_hxx

#include "TestInviteSession.hxx"
#include "resip/dum/Handles.hxx"
#include "resip/dum/DialogEventInfo.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "tfm/EndPoint.hxx"
#include "DialogEventHandlerEvent.hxx"
#include "tfm/TestSipEndPoint.hxx"
#include "tfm/Event.hxx"


class CommonAction;

class DialogEventPred : public ExpectPredicate
{
   public:
      DialogEventPred(const resip::DialogEventInfo& dialogEventInfo);
      virtual bool passes(boost::shared_ptr<Event>);
      virtual const resip::Data& explainMismatch() const;

   private:
      const resip::DialogEventInfo& mDialogEventInfo;
      resip::Data mMismatchExplanation;
};

class TestDialogEvent : public TestUsage
{
   public:
      TestDialogEvent(DumUserAgent* dumUa);
      ~TestDialogEvent() {};

      resip::Data getName() const { return "TestDialogEvent"; }
      
      //accessor to manager
      bool isMyEvent(Event*);

      ExpectBase* expect(DialogEventHandlerEvent::Type t,
                         DialogEventPred& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(DialogEventHandlerEvent::Type t,
                         DialogEventPred& pred,
                         resip::InviteSessionHandler::TerminatedReason reason,
                         int timeoutMs,
                         ActionBase* expectAction);

      //ExpectBase* expect(DialogEventHandlerEvent::Type,
      //                   MessageMatcher* matcher,
      //                   ExpectPreCon& pred,
      //                   int timeoutMs,
      //                   ActionBase* expectAction);

   private:
      friend class DumUserAgent;
      resip::ClientRegistrationHandle& getHandleRef() { return mHandle; }
      resip::ClientRegistrationHandle mHandle;

};

#endif 
