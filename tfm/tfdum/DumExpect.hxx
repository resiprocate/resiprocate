#if !defined DumExpect_hxx
#define DumExpect_hxx

#include "tfm/Event.hxx"
#include "tfm/TestEndPoint.hxx"

#include "resip/dum/Handles.hxx"
#include "resip/stack/SipMessage.hxx"

#include <boost/shared_ptr.hpp>

#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

class DumUserAgent;

//will fail if no SipMessage is present in the event
//TODO migrate to tfm codebase
class MessageMatcher
{
   public:
      virtual ~MessageMatcher() {}
      virtual bool isMatch(const boost::shared_ptr<resip::SipMessage>& message) const = 0;
      virtual resip::Data toString() const = 0;
};

class DumEventMatcher
{
   public:
      virtual bool operator()(boost::shared_ptr<Event> event)
      {
         resip_assert(0);
         return false;
      }
      
      virtual resip::Data getMsgTypeString() const
      {
         resip_assert(0);
         return "";
      }
      
      virtual ~DumEventMatcher() {}
};

template<class T>
class DumEventMatcherSpecific : public DumEventMatcher
{
   public:
      DumEventMatcherSpecific(typename T::Type t) : 
         mEventType(t)
      {
      }

      virtual bool operator()(boost::shared_ptr<Event> event)
      {
         T* specificEvent = dynamic_cast<T*>(event.get());
         if (specificEvent)
         {
            if (specificEvent->getType() == mEventType)
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
            StackLog( << "Type of event is not : " << T::getName());
            return false;
         }
      }

      virtual resip::Data getMsgTypeString() const
      {
         resip::Data d;
         {
            resip::DataStream ds(d);
            //ds << T::getName() << " Enum val: " << mEventType;
            ds << T::getName() << "Event type: " << T::getTypeName(mEventType);
        }
         return d;
      }
   private:
      typename T::Type mEventType;
};
         
class DumExpect : public TestEndPoint::ExpectBase
{
   public:
      DumExpect(TestEndPoint& endPoint, //DumUserAgent& endPoint, 
                DumEventMatcher* dem, 
                MessageMatcher* matcher,
                TestEndPoint::ExpectPreCon& preCon,
                int timeoutMs,
                ActionBase* expectAction,
                ActionBase* matchTimeAction=0);

      virtual ~DumExpect();

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

      //DumUserAgent& mUserAgent;
      TestEndPoint& mUserAgent;
      DumEventMatcher* mDumEventMatcher;
      MessageMatcher* mMatcher;
      TestEndPoint::ExpectPreCon& mPreCon;
      unsigned int mTimeoutMs;
      ActionBase* mExpectAction;
      ActionBase* mMatchTimeAction;      
};
#undef RESIPROCATE_SUBSYSTEM

#endif
