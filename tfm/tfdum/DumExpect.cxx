#include "DumUserAgent.hxx"
#include "DumExpect.hxx"
#include "TestUsage.hxx"

#include <iostream>

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

DumExpect::DumExpect(TestEndPoint& endPoint, //DumUserAgent& endPoint, 
                     DumEventMatcher* dem, 
                     MessageMatcher* matcher,
                     TestEndPoint::ExpectPreCon& preCon,
                     int timeoutMs,
                     ActionBase* expectAction,
                     ActionBase* matchTimeAction)
: mUserAgent(endPoint),
  mDumEventMatcher(dem),
  mMatcher(matcher),
  mPreCon(preCon),
  mTimeoutMs(timeoutMs*TestEndPoint::DebugTimeMult()),
  mExpectAction(expectAction),
  mMatchTimeAction(matchTimeAction)
{
}

DumExpect::~DumExpect()
{
   delete mExpectAction;
   delete mMatcher;
   delete mDumEventMatcher;   
}

/*
DumUserAgent* 
DumExpect::getUserAgent() const
{
   return &mUserAgent; 
}
*/

TestEndPoint* 
DumExpect::getEndPoint() const
{
   return &mUserAgent;
}

unsigned int 
DumExpect::getTimeout() const
{
   return mTimeoutMs; 
}

void
DumExpect::onEvent(TestEndPoint& endPoint, boost::shared_ptr<Event> event)
{
   mExpectAction->exec(event);
}

resip::Data
DumExpect::getMsgTypeString() const
{
   return mDumEventMatcher->getMsgTypeString();
}

ostream& 
DumExpect::output(ostream& s) const
{
   if (isOptional())
   {
      s << "opt(";
   }

   s << mUserAgent.getName() << ".expect(" << mDumEventMatcher->getMsgTypeString();   
   if (mMatcher) 
   {
      s << ", " << mMatcher->toString();
   }
   s << ")";   

   if (isOptional())
   {
      s << ")";
   }

   return s;
}


bool
DumExpect::isMatch(boost::shared_ptr<Event> event) const
{
   
   bool matches = match(event);
   if (matches && mMatchTimeAction)
   {
      StackLog (<< "Executing match time action");      
      (*mMatchTimeAction)(event);
      delete mMatchTimeAction;
      DumExpect* ncThis = const_cast<DumExpect*>(this);
      ncThis->mMatchTimeAction = 0;
   }
   return matches;   
}

//!dcm! TODO quiter, but information is lost; save what is now StackLog and use
//!in explainMismatch
bool
DumExpect::match(boost::shared_ptr<Event> event) const
{
   StackLog (<< "matching: " << *event);
   DumEvent* dumEvent = dynamic_cast<DumEvent*>(event.get());
   if (!dumEvent)
   {
      StackLog(<< "ismatch failed, not a dum event");
      return false;
   }   
   
   bool matchEndPoint = false;
   TestUsage* tu = dynamic_cast<TestUsage*>(getEndPoint());
   if (tu)
   {
      StackLog(<< "Matching TestUsage");
      matchEndPoint = tu->getDumUserAgent()->matchEvent(event.get());
   }
   else
   {
      StackLog(<< "Matching DumUserAgent: " << getEndPoint()->getName() << dumEvent->getEndPoint()->getName());      
      matchEndPoint = (getEndPoint() == dumEvent->getEndPoint());
   }

   if (!matchEndPoint)
   {
      StackLog(<< "isMatch failed on endpoint");
      return false;
   }
   
   if(!(*mDumEventMatcher)(event))
   {
      DebugLog(<< "isMatch failed on dumEventMatcher");
      return false;      
   }

   if (mMatcher)
   {
      if (!dumEvent->hasMessage())
      {
         StackLog(<< "ismatch failed, no SipMessage in DumEvent");
         return false;         
      }
      else
      {
         if (!mMatcher->isMatch(dumEvent->getMessage())) 
         {
            StackLog(<< "isMatch failed on matcher");
            return false;
         }
      }
   }   
   
   if(!mPreCon.passes(event))
   {
      StackLog(<< "isMatch failed on precondition: " << mPreCon.toString());
      return false;
   }

   return true;   
}

resip::Data
DumExpect::explainMismatch(boost::shared_ptr<Event> event) const
{
   DumEvent* dumEvent = dynamic_cast<DumEvent*>(event.get());
   resip_assert(dumEvent);
   boost::shared_ptr<SipMessage> msg = dumEvent->getMessage();

   Data s;
   {
      DataStream str(s);
      str << "Failed expect: received " << *dumEvent
          << " expected " << mDumEventMatcher->getMsgTypeString()
          << " user=" << *dumEvent->getEndPoint();
   }
   
   return s;
}


DumExpect::Exception::Exception(const resip::Data& msg,
                                const resip::Data& file,
                                const int line)
   : resip::BaseException(msg, file, line)
{
}

resip::Data 
DumExpect::Exception::getName() const 
{
   return "DumExpect::Exception";
}

const char* 
DumExpect::Exception::name() const 
{
   return "DumExpect::Exception";
}


Box
DumExpect::layout() const
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
DumExpect::render(CharRaster& out) const
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

