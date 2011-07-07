#if !defined DialogEventExpect_hxx
#define DialogEventExpect_hxx

#include "tfm/Event.hxx"
#include "tfm/TestEndPoint.hxx"

#include "resip/dum/Handles.hxx"
#include "resip/stack/SipMessage.hxx"

#include <boost/shared_ptr.hpp>

#include "rutil/Logger.hxx"

#include "DumExpect.hxx"

#include "TestDialogEvent.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

class DialogEventMatcher
{
   public:
      DialogEventMatcher(DialogEventHandlerEvent::Type t) : mType(t)
      {
      }

      virtual bool operator()(boost::shared_ptr<Event> event)
      {
         DialogEventHandlerEvent* specificEvent = dynamic_cast<DialogEventHandlerEvent*>(event.get());
         if (specificEvent)
         {
            if (specificEvent->getType() == mType)
            {
               StackLog( << "DumEventMatcherSpecific::operator() matched for : " << *event);
               return true;
            }
            else
            {
               StackLog( << "DumEventMatcherSpecific::operator() did not match for : " << *event);
               return false;
            }
         }
         else
         {
            StackLog( << "Type of event is not : " << DialogEventHandlerEvent::getName());
            return false;
         }
      }
      
      virtual resip::Data getMsgTypeString() const
      {
         resip::Data d;
         {
            resip::DataStream ds(d);
            ds << DialogEventHandlerEvent::getName() << "Event type: " << DialogEventHandlerEvent::getTypeName(mType);
        }
         return d;
      }
      
      virtual ~DialogEventMatcher() {}

private:
   DialogEventHandlerEvent::Type mType;
};

class DialogEventExpect : public TestEndPoint::ExpectBase
{
   public:
      DialogEventExpect(TestEndPoint& testEndpoint,
                        DialogEventMatcher* matcher,
                        DialogEventPred& pred,
                        //TestEndPoint::ExpectPreCon& preCon,
                        int timeoutMs,
                        ActionBase* expectAction,
                        ActionBase* matchTimeAction=0);

      DialogEventExpect(TestEndPoint& testEndpoint,
                        DialogEventMatcher* matcher,
                        DialogEventPred& pred,
                        resip::InviteSessionHandler::TerminatedReason reason,
                        int timeoutMs,
                        ActionBase* expectAction,
                        ActionBase* matchTimeAction=0);

      virtual ~DialogEventExpect();

      //virtual DumUserAgent* getUserAgent() const;
      virtual TestEndPoint* getEndPoint() const;

      virtual unsigned int getTimeout() const;
      
      virtual bool isMatch(boost::shared_ptr<Event> event) const;
      virtual resip::Data explainMismatch(boost::shared_ptr<Event> event) const;
      
      virtual void onEvent(TestEndPoint&, boost::shared_ptr<Event> event);

      virtual resip::Data getMsgTypeString() const;      
      virtual std::ostream& output(std::ostream& s) const;
      
      class Exception : public resip::BaseException
      {
         public:
            Exception(const resip::Data& msg,
                      const resip::Data& file,
                      const int line);
            virtual resip::Data getName() const ;
            virtual const char* name() const ;
      };
      
      virtual Box layout() const;
      virtual void render(AsciiGraphic::CharRaster &out) const;
   private:
      bool match(boost::shared_ptr<Event> event) const;
      DialogEventMatcher* mDialogEventMatcher;
      DialogEventPred& mPredicate;
      MessageMatcher* mMatcher;
      unsigned int mTimeoutMs;
      ActionBase* mExpectAction;
      ActionBase* mMatchTimeAction;      
      TestEndPoint& mTestEndPoint;
      resip::InviteSessionHandler::TerminatedReason mTerminatedReason;
      mutable resip::Data mMismatchComplaints;
};
#undef RESIPROCATE_SUBSYSTEM

#endif
