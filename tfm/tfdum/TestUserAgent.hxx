#ifndef TestUserAgent_hxx
#define TestUserAgent_hxx

#include <set>
#include <vector>

#include "rutil/BaseException.hxx"
#include "rutil/ThreadIf.hxx"
#include "tfm/TestEndPoint.hxx"

class TestUserAgent;

class TestUserAgent : public TestEndPoint, public ThreadableUserAgent
{
   public:
      static const resip::Uri EmptyUri;

      TestUserAgent(const resip::Data& name,
                    const resip::Uri& proxy,
                    PicassoAccountGuard& account,
                    const resip::Data& interface = Data::Empty);

      TestUserAgent(const resip::Data& name,
                    const resip::Uri& proxy,
                    const resip::Uri& outboundProxy,
                    PicassoAccountGuard& account,
                    const Data& interface = Data::Empty);

      virtual ~TestUserAgent();

      void setPassword(const Data& password);

      virtual Data getName() const {return mName;}
      virtual void clean();




//--
      class ExpectException : public resip::BaseException
      {
         public:
            ExpectException(const Data& msg, const Data& file, const int line);
            virtual const char* name() const;
      };

      class Expect : public TestEndPoint::ExpectBase
      {
         public:
            Expect(TestUserAgent& endPoint, 
                   TestEndPoint::ExpectPreCon& preCon,
                   int timeoutMs,
                   ActionBase* expectAction);
            virtual TestEndPoint* getEndPoint() const;
            virtual void onEvent(TestEndPoint&, boost::shared_ptr<Event> event);
            virtual Box layout() const;
            virtual void render(AsciiGraphic::CharRaster &out) const;
            virtual unsigned int getTimeout() const;

         protected:
            TestUserAgent* mTua;
            TestEndPoint::ExpectPreCon& mPreCon;
            unsigned int mTimeoutMs;
            ActionBase* mExpectAction;
      };

      class ExpectMsgEvent : public Expect
      {
         public:
            ExpectMsgEvent(TestUserAgent& endPoint, 
                           std::pair<TUAMsgEventType, int> msgEventCode, 
                           TestEndPoint::ExpectPreCon& preCon,
                           int timeoutMs,
                           ActionBase* expectAction);
            virtual ~ExpectMsgEvent();
            virtual bool isMatch(boost::shared_ptr<Event> event) const;
            virtual Data explainMismatch(boost::shared_ptr<Event> event) const;
            int getStatusCode() const { return mMsgEventCode.second; }
            virtual Data getMsgTypeString() const;
            virtual std::ostream& output(std::ostream& s) const;

         private:
            std::pair<TUAMsgEventType, int> mMsgEventCode;
      };

      // syntactic sugar
      TestEndPoint::ExpectBase* expect(TUAMsgEventType eventType,int timeoutMs, ActionBase* expectAction);
      TestEndPoint::ExpectBase* expect(TUAMsgEventType eventType,TestEndPoint::ExpectPreCon& pred,int timeoutMs,ActionBase* expectAction);
      TestEndPoint::ExpectBase* expect(std::pair<TUAMsgEventType, int> eventTypeCode, int timeoutMs, ActionBase* expectAction);
      TestEndPoint::ExpectBase* expect(std::pair<TUAMsgEventType, int> eventTypeCode,
                                       TestEndPoint::ExpectPreCon& pred, int timeoutMs, ActionBase* expectAction);

      class ExpectEvent : public Expect
      {
         public:
            ExpectEvent(TestUserAgent& endPoint, 
                        TUAEventType eventCode, 
                        TestEndPoint::ExpectPreCon& preCon,
                        int timeoutMs,
                        ActionBase* expectAction);
            virtual ~ExpectEvent();
            virtual bool isMatch(boost::shared_ptr<Event> event) const;
            virtual Data explainMismatch(boost::shared_ptr<Event> event) const;
            virtual Data getMsgTypeString() const;
            virtual std::ostream& output(std::ostream& s) const;

         private:
            TestUserAgent* mEndPoint;
            TUAEventType mEventCode;
      };

      // syntactic sugar
      TestEndPoint::ExpectBase* expect(TUAEventType eventType, int timeoutMs, ActionBase* expectAction);
      TestEndPoint::ExpectBase* expect(TUAEventType eventType, TestEndPoint::ExpectPreCon& pred, int timeoutMs, ActionBase* expectAction);

      class ExpectSubscribeEvent : public Expect
      {
         public:
            ExpectSubscribeEvent(TestUserAgent& endPoint, 
                            TestEndPoint::ExpectPreCon& preCon,
                            int timeoutMs,
                            ActionBase* expectAction);
            virtual ~ExpectSubscribeEvent();
            virtual bool isMatch(boost::shared_ptr<Event> event) const;
            virtual Data explainMismatch(boost::shared_ptr<Event> event) const;
            virtual Data getMsgTypeString() const;
            virtual std::ostream& output(std::ostream& s) const;
      };

      // syntactic sugar
      TestEndPoint::ExpectBase* expect(TUASubscribeEventType eventType, int timeoutMs, ActionBase* expectAction);
      TestEndPoint::ExpectBase* expect(TUASubscribeEventType eventType, TestEndPoint::ExpectPreCon& pred, int timeoutMs, ActionBase* expectAction);
      
      class ExpectMessageEvent : public Expect
      {
         public:
            ExpectMessageEvent(TestUserAgent& endPoint, 
                               TestEndPoint::ExpectPreCon& preCon,
                               int timeoutMs,
                               ActionBase* expectAction)
               : Expect(endPoint, 
                        preCon,
                        timeoutMs,
                        expectAction)
            {
            }

            virtual ~ExpectMessageEvent()
            {
               delete mExpectAction;
            }
            
            /// determine if the message matches
            virtual bool isMatch(boost::shared_ptr<Event> event) const;
            virtual Data explainMismatch(boost::shared_ptr<Event> event) const;
            virtual Data getMsgTypeString() const;
            virtual std::ostream& output(std::ostream& s) const;
      };
      // syntactic sugar
      TestEndPoint::ExpectBase* expect(TUAMessageEventType eventType, int timeoutMs, ActionBase* expectAction);
      TestEndPoint::ExpectBase* expect(TUAMessageEventType eventType, TestEndPoint::ExpectPreCon& pred, int timeoutMs, ActionBase* expectAction);

   private:
      // Put virtual handlers here
      virtual resip::Mutex& getMutex();

      resip::Uri mUri;

      // a gc issue -- someone has to get rid of these
      std::list<About*> mAbouts;

      class UAThread : public resip::ThreadIf
      {
         public:
            UAThread(resip::Mutex* mutex,
                     TestUserAgent* tua);

            virtual void thread();

         private:
            resip::Mutex* mMutex;
            TestUserAgent* mTua;
      };

      resip::Mutex mMutex;
      UAThread mUAThread;
      
      friend class DragonTestHolder;
};

// syntactic sugar for passing message events and response code
// as e.g.
// TUA_RESPONSE/180
inline
std::pair<TUAMsgEventType, int>
operator/(TUAMsgEventType event,
          int code)
{
   return std::make_pair(event, code);
}

#endif
