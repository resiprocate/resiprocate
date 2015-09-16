#if !defined Expect_hxx
#define Expect_hxx

#include "tfm/Event.hxx"
#include "tfm/TestEndPoint.hxx"

#include <boost/shared_ptr.hpp>

class EventMatcher
{
   public:
      virtual bool isMatch(boost::shared_ptr<Event> event)
      {
         resip_assert(0);
         return false;
      }

      virtual resip::Data getMsgTypeString() const
      {
         resip_assert(0);
         return "";
      }

      virtual const resip::Data& explainMismatch() const = 0;

      virtual ~EventMatcher() {}
};

template<class T>
class EventMatcherSpecific : public EventMatcher
{
   public:
      EventMatcherSpecific(typename T::Type t) :
         mEventType(t)
      {
      }

      virtual bool isMatch(boost::shared_ptr<Event> event)
      {
         T* specificEvent = dynamic_cast<T*>(event.get());
         if (specificEvent)
         {
            if (specificEvent->getType() == mEventType)
            {
               return true;
            }
            else
            {
               {
                  resip::DataStream ds(mMessage);
                  ds << "Expected " << T::getTypeName(mEventType) << ", Received " << *event;
               }
               return false;
            }
         }
         else
         {
            {
               resip::DataStream ds(mMessage);
               ds << "Wrong event type";
            }
            return false;
         }
      }

      virtual resip::Data getMsgTypeString() const
      {
         resip::Data d;
         {
            resip::DataStream ds(d);
            ds << T::getName() << ": " << T::getTypeName(mEventType);
         }
         return d;
      }

      const resip::Data& explainMismatch() const { return mMessage; }

   private:
      typename T::Type mEventType;
      resip::Data mMessage;
};


class ExpectPredicate
{
   public:
      virtual ~ExpectPredicate() 
      {
      }
      
      virtual bool passes(boost::shared_ptr<Event>) = 0;
      virtual const resip::Data& explainMismatch() const = 0;
};

class Expect : public TestEndPoint::ExpectBase
{
   public:
      Expect(TestEndPoint& endPoint,
             EventMatcher* em,
             int timeoutMs,
             ActionBase* expectAction);

      Expect(TestEndPoint& endPoint,
             EventMatcher* em,
             ExpectPredicate* pred,
             int timeoutMs,
             ActionBase* expectAction);
      ~Expect();

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
      TestEndPoint& mEndPoint;
      EventMatcher* mEventMatcher;
      typedef std::vector<ExpectPredicate*> ExpectPredicates;
      ExpectPredicates mExpectPredicates;
      unsigned int mTimeoutMs;
      ActionBase* mExpectAction;
      resip::Data mMessage;
};

#endif
