#include "Expect.hxx"

#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;
using namespace std;

Expect::Expect(TestEndPoint& endPoint,
               EventMatcher* em,
               int timeoutMs,
               ActionBase* expectAction) :
   mEndPoint(endPoint),
   mEventMatcher(em),
   mTimeoutMs(timeoutMs),
   mExpectAction(expectAction)
{
}

Expect::Expect(TestEndPoint& endPoint,
               EventMatcher* em,
               ExpectPredicate* pred,
               int timeoutMs,
               ActionBase* expectAction) :
   mEndPoint(endPoint),
   mEventMatcher(em),
   mTimeoutMs(timeoutMs),
   mExpectAction(expectAction)
{
   mExpectPredicates.push_back(pred);
}

Expect::~Expect()
{
   delete mExpectAction;
   delete mEventMatcher;

   for( ExpectPredicates::iterator i = mExpectPredicates.begin(); i != mExpectPredicates.end(); i++ )
   {
      delete *i;
   }  

}

TestEndPoint* 
Expect::getEndPoint() const
{
   return &mEndPoint;
}

unsigned int 
Expect::getTimeout() const
{
   return mTimeoutMs; 
}

bool
Expect::isMatch(boost::shared_ptr<Event> event) const
{
   bool bMatch = mEventMatcher->isMatch(event);

   Expect* ncThis = const_cast<Expect*>(this);
   if( !bMatch )
   {
      ncThis->mMessage = mEventMatcher->explainMismatch();
      return false;
   }
   

   for( ExpectPredicates::const_iterator i = mExpectPredicates.begin(); i != mExpectPredicates.end(); i++ )
   {
      if( !(*i)->passes(event) )
      {
         ncThis->mMessage = (*i)->explainMismatch();
         return false;
      }
   }
   
   return bMatch;
}

Data
Expect::explainMismatch(boost::shared_ptr<Event> event) const
{
   // .jv. maybe print out event
   return mMessage;
}

void
Expect::onEvent(TestEndPoint& endPoint, boost::shared_ptr<Event> event)
{
   mExpectAction->exec(event);
}

resip::Data
Expect::getMsgTypeString() const
{
   return mEventMatcher->getMsgTypeString();
}

ostream& 
Expect::output(ostream& s) const
{
   if (isOptional())
   {
      s << "opt(";
   }

   resip_assert(&mEndPoint);
   
   s << mEndPoint.getName();

   s << ".expect(" << mEventMatcher->getMsgTypeString() << ")";  

   for(unsigned int i = 0; i < mExpectPredicates.size(); i++)
   {
      ExpectPredicate* pred = mExpectPredicates[i];

      Data message = pred->explainMismatch();
      if(message.size() == 0)
         continue;

      s << " FAILED Pred: (" << typeid( *pred ).name() << "): " << pred->explainMismatch();
   }

   if (isOptional())
   {
      s << ")";
   }

   return s;
}

Expect::Exception::Exception(const resip::Data& msg,
                             const resip::Data& file,
                             const int line)
   : resip::BaseException(msg, file, line)
{
}

resip::Data 
Expect::Exception::getName() const
{  
   return "Expect::Exception";
}

const char* 
Expect::Exception::name() const
{  
   return "Expect::Exception";
}

Box
Expect::layout() const
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
Expect::render(CharRaster& out) const
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

