#include "DumUserAgent.hxx"
#include "DialogEventExpect.hxx"
#include "TestUsage.hxx"

#include <iostream>

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

DialogEventExpect::DialogEventExpect(TestEndPoint& testEndpoint,
                        DialogEventMatcher* matcher,
                        DialogEventPred& pred,
                        //TestEndPoint::ExpectPreCon& preCon,
                     int timeoutMs,
                     ActionBase* expectAction,
                     ActionBase* matchTimeAction)
: mDialogEventMatcher(matcher),
  mPredicate(pred),
  mMatcher(0),
  mTimeoutMs(timeoutMs*TestEndPoint::DebugTimeMult()),
  mExpectAction(expectAction),
  mMatchTimeAction(matchTimeAction),
  mTestEndPoint(testEndpoint),
  mTerminatedReason(InviteSessionHandler::Error)
{
}

DialogEventExpect::DialogEventExpect(TestEndPoint& testEndpoint,
                        DialogEventMatcher* matcher,
                        DialogEventPred& pred,
                        resip::InviteSessionHandler::TerminatedReason reason,
                        int timeoutMs,
                        ActionBase* expectAction,
                        ActionBase* matchTimeAction)
: mDialogEventMatcher(matcher),
  mPredicate(pred),
  mMatcher(0),
  mTimeoutMs(timeoutMs*TestEndPoint::DebugTimeMult()),
  mExpectAction(expectAction),
  mMatchTimeAction(matchTimeAction),
  mTestEndPoint(testEndpoint),
  mTerminatedReason(reason)
{
}

DialogEventExpect::~DialogEventExpect()
{
   delete mExpectAction;
   delete mDialogEventMatcher;   
}

TestEndPoint* 
DialogEventExpect::getEndPoint() const
{
   return &mTestEndPoint;
}

unsigned int 
DialogEventExpect::getTimeout() const
{
   return mTimeoutMs; 
}

void
DialogEventExpect::onEvent(TestEndPoint& endPoint, boost::shared_ptr<Event> event)
{
   mExpectAction->exec(event);
}

resip::Data
DialogEventExpect::getMsgTypeString() const
{
   return mDialogEventMatcher->getMsgTypeString();
}

//TODO fix this
ostream& 
DialogEventExpect::output(ostream& s) const
{
   if (isOptional())
   {
      s << "opt(";
   }

   s << ".expect(" << mDialogEventMatcher->getMsgTypeString();   
   s << ")";   

   if (isOptional())
   {
      s << ")";
   }

   return s;
}


bool
DialogEventExpect::isMatch(boost::shared_ptr<Event> event) const
{
   
   bool matches = match(event);
   if (matches && mMatchTimeAction)
   {
      StackLog (<< "Executing match time action");      
      (*mMatchTimeAction)(event);
      delete mMatchTimeAction;
      DialogEventExpect* ncThis = const_cast<DialogEventExpect*>(this);
      ncThis->mMatchTimeAction = 0;
   }
   return matches;   
}

//!dcm! TODO quiter, but information is lost; save what is now StackLog and use
//!in explainMismatch
bool
DialogEventExpect::match(boost::shared_ptr<Event> event) const
{
   StackLog (<< "matching: " << *event);
   DialogEventHandlerEvent* dumEvent = dynamic_cast<DialogEventHandlerEvent*>(event.get());
   if (!dumEvent)
   {
      StackLog(<< "ismatch failed, not a dialog event");
      return false;
   }   
   
   if(!(*mDialogEventMatcher)(event))
   {
      DebugLog(<< "isMatch failed on dialogEventMatcher");
      return false;      
   }

   if (dumEvent->getDialogEventInfo().getState() == DialogEventInfo::Terminated)
   {
      if (dumEvent->getTerminatedReason() != mTerminatedReason)
      {
         DebugLog(<< "TerminatedReason " << dumEvent->getTerminatedReason() 
                  << " does not match expected " << mTerminatedReason);
         return false;
      }
   }

   //call dialog event specific precon, save error for explainMismatch
   if (!mPredicate.passes(event))
   {
      DebugLog(<< "isMatch failed on predicate: " << mPredicate.explainMismatch());
      return false;
   }

   return true;   
}

resip::Data
DialogEventExpect::explainMismatch(boost::shared_ptr<Event> event) const
{
   DialogEventHandlerEvent* dumEvent = dynamic_cast<DialogEventHandlerEvent*>(event.get());
   resip_assert(dumEvent);
   //boost::shared_ptr<SipMessage> msg = dumEvent->getMessage();

   return mMismatchComplaints;
}


DialogEventExpect::Exception::Exception(const resip::Data& msg,
                                const resip::Data& file,
                                const int line)
   : resip::BaseException(msg, file, line)
{
}

resip::Data 
DialogEventExpect::Exception::getName() const 
{
   return "DialogEventExpect::Exception";
}

const char* 
DialogEventExpect::Exception::name() const 
{
   return "DialogEventExpect::Exception";
}


Box
DialogEventExpect::layout() const
{
   mBox.mX = 1;
   mBox.mY = 0;

   //     |
   // ext17->100
   mBox.mHeight = 2;

   Data out;
   {
      DataStream str(out);
      bool previousActive = false;
      prettyPrint(str, previousActive, 0);
   }
   mBox.mWidth = out.size()+2;

   //InfoLog(<< "TestSipEndPoint::SipExpect::layout: " << mBox);

   return mBox;
}

void
DialogEventExpect::render(CharRaster& out) const
{
   //InfoLog(<< "TestSipEndPoint::SipExpect::render");
   out[mBox.mY][mBox.mX + mBox.mWidth/2] = '|';

   Data s(" ");
   {
      DataStream str(s);

      bool previousActive = false;
      prettyPrint(str, previousActive, 0);
   }
   s += " ";

   //InfoLog(<< "size of SipExpect::render size = " << s.size());
   int x = 0;
   for (unsigned int i=0; i<s.size(); i++)
   {
      out[mBox.mY+1][mBox.mX + x] = s[i];
      x++;
   }
}

