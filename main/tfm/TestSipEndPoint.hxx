#ifndef TestSipEndPoint_hxx
#define TestSipEndPoint_hxx

#include <list>
#include <set>
#include <boost/shared_ptr.hpp>

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/ParserCategories.hxx"
#include "resip/stack/SdpContents.hxx"
#include "resip/stack/Transport.hxx"
#include "rutil/HashMap.hxx"
#include "tfm/DialogSet.hxx"
#include "tfm/ExpectFunctorDefs.hxx"
#include "tfm/ExpectAction.hxx"
#include "tfm/Event.hxx"
#include "tfm/TestEndPoint.hxx"
#include "tfm/TransportDriver.hxx"

class TestProxy;
class TestSipEndPoint;
class TestUser;
// vk
enum messageType {
     textPlain    = 0 ,
     messageCpim  = 1
};
// end - vk

namespace resip
{
class SdpContents;
class NameAddr;
class DeprecatedDialog;
class Security;
}

class TestSipEndPoint : public TestEndPoint, public TransportDriver::Client
{
   public:
      static resip::Uri NoOutboundProxy;
      
      TestSipEndPoint(const resip::Uri& from,
                      const resip::Uri& contact, 
                      const resip::Uri& outboundProxy=NoOutboundProxy,
                      bool hasStack=true,
                      const resip::Data& interfaceObj = resip::Data::Empty,
                      resip::Security* security = 0);
      
      explicit TestSipEndPoint(const resip::Uri& contact, 
                               const resip::Uri& outboundProxy=NoOutboundProxy,
                               bool hasStack=true,
                               const resip::Data& interfaceObj = resip::Data::Empty,
                               resip::Security* security = 0);
      
      virtual ~TestSipEndPoint();
      virtual void clean();
      void setTransport(resip::Transport*);

      class SipEndPointAction : public Action
      {
         public:
            explicit SipEndPointAction(TestSipEndPoint* endPoint);
            virtual ~SipEndPointAction();
            virtual void operator()();
            virtual void operator()(TestSipEndPoint& endPoint) = 0;
         protected:
            TestSipEndPoint* mEndPoint;
      };

      class Refer : public SipEndPointAction
      {
         public:
            Refer(TestSipEndPoint* endPoint, 
                  const resip::Uri& who, 
                  const resip::Uri& to, 
                  bool replaces = false);
            
            virtual void operator()(TestSipEndPoint& endPoint);
            virtual resip::Data toString() const;

            resip::Uri mTo;
            resip::Uri mWho;
            bool mReplaces;
      };
      Refer* refer(const resip::Uri& who, const resip::Uri& to);
      Refer* referReplaces(const resip::Uri& who, const resip::Uri& to);
      
      class ReInvite : public Action
      {
         public:
            ReInvite(TestSipEndPoint* from, const resip::Uri& to, bool matchUserOnly = false, boost::shared_ptr<resip::SdpContents> sdp =
                   boost::shared_ptr<resip::SdpContents>());
            virtual void operator()();
            virtual void operator()(boost::shared_ptr<Event> event);
            virtual resip::Data toString() const;
            virtual void go();

         private:

            TestSipEndPoint & mEndPoint;
            resip::NameAddr mTo;
            bool mMatchUserOnly;
            boost::shared_ptr<resip::SdpContents> mSdp;
      };
      friend class ReInvite;
      ReInvite* reInvite(const TestSipEndPoint& endPoint);
      ReInvite* reInvite(resip::Uri& url);
      ReInvite* reInvite(const resip::Data& user);
      ReInvite* reInvite(const resip::Data& user, const boost::shared_ptr<resip::SdpContents>& sdp);
               
      class InviteReferReplaces : public ExpectAction
      {
         public:
            InviteReferReplaces(TestSipEndPoint* from, bool replaces);
            virtual void operator()(boost::shared_ptr<Event> event);
            virtual resip::Data toString() const;
         private:
            TestSipEndPoint& mEndPoint;
            bool mReplaces;
      };
      friend class InviteReferReplaces;
      InviteReferReplaces* inviteReferReplaces();
      InviteReferReplaces* inviteReferredBy();

      typedef boost::function<boost::shared_ptr<resip::SipMessage> 
      ( boost::shared_ptr<resip::SipMessage> msg) > 
      MessageConditionerFn;

      typedef boost::function<resip::Data 
      ( boost::shared_ptr<resip::SipMessage> msg) > 
      RawConditionerFn;

      class IdentityMessageConditioner
      {
         public:
            boost::shared_ptr<resip::SipMessage> operator()(boost::shared_ptr<resip::SipMessage> msg);
      };
      static IdentityMessageConditioner identity;

      class IdentityRawConditioner
      {
         public:
            resip::Data operator()(boost::shared_ptr<resip::SipMessage> msg);
      };
      static IdentityRawConditioner raw_identity;

      
      class ChainConditions
      {
         public:
            ChainConditions(MessageConditionerFn fn1, MessageConditionerFn fn2);
            boost::shared_ptr<resip::SipMessage> operator()(boost::shared_ptr<resip::SipMessage> msg);

         private:
            MessageConditionerFn mFn1;
            MessageConditionerFn mFn2;
      };

      class SaveMessage
      {
         public:
            SaveMessage(boost::shared_ptr<resip::SipMessage>& msgPtr);
            boost::shared_ptr<resip::SipMessage> operator()(boost::shared_ptr<resip::SipMessage> msg);
         private:
            boost::shared_ptr<resip::SipMessage>& mMsgPtr;
      };

      class MessageAction : public Action
      {
         public:
            MessageAction(TestSipEndPoint& from, const resip::Uri& to);
            virtual void operator()();
            void setConditioner(MessageConditionerFn conditioner);
            virtual boost::shared_ptr<resip::SipMessage> go() = 0;

         protected:
            TestSipEndPoint& mEndPoint;
            resip::Uri mTo;
            boost::shared_ptr<resip::SipMessage> mMsg;
            MessageConditionerFn mConditioner;
      };

      class Invite : public MessageAction
      {
         public:
            Invite(TestSipEndPoint* from, 
                   const resip::Uri& to, 
                   boost::shared_ptr<resip::SdpContents> sdp = 
                   boost::shared_ptr<resip::SdpContents>());
            virtual resip::Data toString() const;
            
         private:
            virtual boost::shared_ptr<resip::SipMessage> go();
            boost::shared_ptr<resip::SdpContents> mSdp;
      };
      friend class Invite;
      Invite* invite(const TestUser& endPoint);
      Invite* invite(const TestSipEndPoint& endPoint);
      Invite* invite(const resip::Uri& url);
      Invite* invite(const TestUser& endPoint, const boost::shared_ptr<resip::SdpContents>& sdp);
      Invite* invite(const resip::Uri& url, const boost::shared_ptr<resip::SdpContents>& sdp);
      Invite* invite(const resip::Data& url);

      class RawInvite : public MessageAction
      {
         public:
            RawInvite(TestSipEndPoint* from, 
                      const resip::Uri& to, 
                      const resip::Data& rawText);
            virtual resip::Data toString() const;

         private:
            virtual boost::shared_ptr<resip::SipMessage> go();
            resip::Data mRawText;
      };
      friend class RawInvite;
      RawInvite* rawInvite(const TestSipEndPoint* endPoint, const resip::Data& rawText);
      RawInvite* rawInvite(const TestUser& endPoint, const resip::Data& rawText);
      
      class RawSend : public Action
      {
         public:
            RawSend(TestSipEndPoint* from, 
                    const resip::Uri& to, 
                    const resip::Data& rawText);
            RawSend(TestSipEndPoint* from, 
                    const resip::Uri& to, 
                    boost::shared_ptr<resip::SipMessage>& msg);
            void setConditioner(RawConditionerFn conditioner);
            virtual void operator()();
            virtual void operator()(boost::shared_ptr<Event> event);
            virtual resip::Data toString() const;

         private:
            virtual void go();

         private:
            TestSipEndPoint& mEndPoint;
            resip::NameAddr mTo;
            resip::Data mRawText;
            boost::shared_ptr<resip::SipMessage>* mMsg;
            RawConditionerFn mConditioner;
            bool rawAlreadySpecified;
            
      };
      friend class RawSend;
      RawSend* rawSend(const TestSipEndPoint* endPoint, const resip::Data& rawText);
      RawSend* rawSend(const TestUser& endPoint, const resip::Data& rawText);
      RawSend* rawSend(const resip::Uri& target, const resip::Data& rawText);

      RawSend* rawSend(const TestSipEndPoint* endPoint, boost::shared_ptr<resip::SipMessage>& msg);
      RawSend* rawSend(const TestUser& endPoint, boost::shared_ptr<resip::SipMessage>& msg);
      RawSend* rawSend(const resip::Uri& target, boost::shared_ptr<resip::SipMessage>& msg);

      class Subscribe : public Action
      {
         public:
            Subscribe(TestSipEndPoint* from, const resip::Uri& to, const resip::Token& eventPackage);
            Subscribe(TestSipEndPoint* from, const resip::Uri& to, const resip::Token& eventPackage, const resip::Mime& accept, boost::shared_ptr<resip::Contents> contents = boost::shared_ptr<resip::Contents>());
            virtual void operator()();
            virtual void operator()(boost::shared_ptr<Event> event);
            virtual resip::Data toString() const;
         private:
            void go();
            
            TestSipEndPoint & mEndPoint;
            resip::Uri mTo;
            resip::Token mEventPackage;
            resip::Mime mAccept;
            boost::shared_ptr<resip::Contents> mContents;
      };
      friend class Subscribe;
      Subscribe* subscribe(const TestSipEndPoint* endPoint, const resip::Token& eventPackage);
      Subscribe* subscribe(const TestUser& endPoint, const resip::Token& eventPackage);
      Subscribe* subscribe(const resip::Uri& url, const resip::Token& eventPackage);
      Subscribe* subscribe(const resip::Uri& url, const resip::Token& eventPackage, const resip::Mime& accept, const boost::shared_ptr<resip::Contents>& contents);

      class Request : public MessageAction
      {
         public:
            Request(TestSipEndPoint* from, const resip::Uri& to, 
                    resip::MethodTypes type,
                    boost::shared_ptr<resip::Contents> contents = 
                    boost::shared_ptr<resip::Contents>());
            virtual resip::Data toString() const;

         private:
            virtual boost::shared_ptr<resip::SipMessage> go();
            resip::MethodTypes mType;
            boost::shared_ptr<resip::Contents> mContents;
      };
      friend class Request;
      Request* request(const TestUser& endPoint, 
                       resip::MethodTypes method, 
                       boost::shared_ptr<resip::Contents> c = 
                       boost::shared_ptr<resip::Contents>());

      Request* request(const resip::Uri& to, 
                       resip::MethodTypes method, 
                       boost::shared_ptr<resip::Contents> c = 
                       boost::shared_ptr<resip::Contents>());
      
      Request* info(const TestSipEndPoint* endPoint);
      Request* info(const TestUser& endPoint);
      Request* info(const resip::Uri& url);
      Request* info(const resip::Uri& url, const boost::shared_ptr<resip::Contents>& contents);

      Request* message(const TestSipEndPoint* endPoint, const resip::Data& text);
      Request* message(const TestUser& endPoint, const resip::Data& text);
      // vk
      Request* message(const resip::NameAddr& target, const resip::Data& text, const messageType contentType = textPlain);
      // end - vk

      //create message that do not require exsistance of other endpoint
      Request* message(const resip::Uri& target, const resip::Data& text);
      Request* message(const resip::Uri& target, const boost::shared_ptr<resip::Contents>& contents);

      // vk
      Request* options(const resip::Uri& target);
      // end - vk
      
      class Retransmit : public ExpectAction
      {
         public:
            Retransmit(TestSipEndPoint* endPoint, 
                       boost::shared_ptr<resip::SipMessage>& msg);
            virtual void operator()(boost::shared_ptr<Event> event);
            virtual resip::Data toString() const;

         private:
            TestSipEndPoint* mEndPoint;
            boost::shared_ptr<resip::SipMessage>& mMsgToRetransmit;
      };
      friend class Retransmit;     
      Retransmit* retransmit(boost::shared_ptr<resip::SipMessage>& msg);

      class CloseTransport : public Action
      {
         public:
            CloseTransport(TestSipEndPoint* endPoint);
            virtual void operator()(boost::shared_ptr<Event> event);
            virtual void operator()();
            virtual resip::Data toString() const;

         private:
            TestSipEndPoint* mEndPoint;
      };
      friend class CloseTransport;     
      CloseTransport* closeTransport();
      // JF

      class MessageExpectAction : public ExpectAction
      {
         public:
            MessageExpectAction(TestSipEndPoint& from);
            virtual void operator()(boost::shared_ptr<Event> event);
            virtual boost::shared_ptr<resip::SipMessage> go(boost::shared_ptr<resip::SipMessage>) = 0;
            void setConditioner(MessageConditionerFn conditioner);

         protected:
            TestSipEndPoint& mEndPoint;
            boost::shared_ptr<resip::SipMessage> mMsg;
            MessageConditionerFn mConditioner;
      };

      class Send300 : public MessageExpectAction
      {
         public:
            explicit Send300(TestSipEndPoint& endpoint, std::set<resip::NameAddr> alternates);
            virtual boost::shared_ptr<resip::SipMessage>
            go(boost::shared_ptr<resip::SipMessage> msg);
            std::set<resip::NameAddr> mAlternates;
            TestSipEndPoint& mEndpoint;
      };
      MessageExpectAction* send300(std::set<resip::NameAddr> alternates);

      // vk
      class Send301 : public MessageExpectAction
      {
         public:
            explicit Send301(TestSipEndPoint& endpoint, std::set<resip::NameAddr> alternates);
            virtual boost::shared_ptr<resip::SipMessage>
            go(boost::shared_ptr<resip::SipMessage> msg);
            std::set<resip::NameAddr> mAlternates;
            TestSipEndPoint& mEndPoint;
      };
      MessageExpectAction* send301(std::set<resip::NameAddr> alternates);

      class Send400 : public MessageExpectAction
      {
         public:
            explicit Send400(TestSipEndPoint & endPoint);
            virtual boost::shared_ptr<resip::SipMessage>
            go(boost::shared_ptr<resip::SipMessage> msg);
            TestSipEndPoint& mEndPoint;

      };
      MessageExpectAction* send400();

      class Send407 : public MessageExpectAction
      {
         public:
            explicit Send407(TestSipEndPoint & endPoint);
            virtual boost::shared_ptr<resip::SipMessage>
            go(boost::shared_ptr<resip::SipMessage> msg);
            TestSipEndPoint& mEndPoint;

      };
      MessageExpectAction* send407();

      class Send408 : public MessageExpectAction
      {
         public:
            explicit Send408(TestSipEndPoint & endPoint);
            virtual boost::shared_ptr<resip::SipMessage>
            go(boost::shared_ptr<resip::SipMessage> msg);
            TestSipEndPoint& mEndPoint;

      };
      MessageExpectAction* send408();

      class Send410 : public MessageExpectAction
      {
         public:
            explicit Send410(TestSipEndPoint & endPoint);
            virtual boost::shared_ptr<resip::SipMessage>
            go(boost::shared_ptr<resip::SipMessage> msg);
            TestSipEndPoint& mEndPoint;

      };
      MessageExpectAction* send410();

      class Send482 : public MessageExpectAction
      {
         public:
            explicit Send482(TestSipEndPoint & endPoint);
            virtual boost::shared_ptr<resip::SipMessage>
            go(boost::shared_ptr<resip::SipMessage> msg);
            TestSipEndPoint& mEndPoint;

      };
      MessageExpectAction* send482();


      class Send483 : public MessageExpectAction
      {
         public:
            explicit Send483(TestSipEndPoint & endPoint);
            virtual boost::shared_ptr<resip::SipMessage>
            go(boost::shared_ptr<resip::SipMessage> msg);
            TestSipEndPoint& mEndPoint;

      };
      MessageExpectAction* send483();

      // end - vk

      class Send302 : public MessageExpectAction
      {
         public:
            explicit Send302(TestSipEndPoint & endPoint);
            virtual boost::shared_ptr<resip::SipMessage>
            go(boost::shared_ptr<resip::SipMessage> msg);
            TestSipEndPoint & mEndPoint;                                                      
      };
      MessageExpectAction* send302();


      class RawReply : public MessageExpectAction
      {
         public:
            RawReply(TestSipEndPoint& from, const resip::Data& rawText);
            virtual boost::shared_ptr<resip::SipMessage> go(boost::shared_ptr<resip::SipMessage>);

         private:
            resip::Data mRawText;
      };



      class Respond : public MessageExpectAction
      {
         public:
            Respond(TestSipEndPoint& endPoint, int responseCode);

            virtual boost::shared_ptr<resip::SipMessage>
            go(boost::shared_ptr<resip::SipMessage> msg);

         private:
            TestSipEndPoint& mEndPoint;
            const int mCode;
      };
      MessageExpectAction* respond(int code);

      // 202 to SUBSCRIBE
      class Send202ToSubscribe : public MessageExpectAction
      {
         public:
            Send202ToSubscribe(TestSipEndPoint& endPoint);
            virtual boost::shared_ptr<resip::SipMessage>
            go(boost::shared_ptr<resip::SipMessage> msg);
            TestSipEndPoint& mEndPoint;
      };
      MessageExpectAction* send202ToSubscribe();

      // 200 to PUBLISH
      class Send200ToPublish : public MessageExpectAction
      {
         public:
            Send200ToPublish(TestSipEndPoint& endPoint);
            virtual boost::shared_ptr<resip::SipMessage> go(boost::shared_ptr<resip::SipMessage> msg);
            TestSipEndPoint& mEndPoint;
      };
      MessageExpectAction* send200ToPublish();

      // 
      class Send423Or200ToPublish : public MessageExpectAction
      {
         public:
            Send423Or200ToPublish(TestSipEndPoint& endPoint, int minExpres);
            virtual boost::shared_ptr<resip::SipMessage> go(boost::shared_ptr<resip::SipMessage> msg);
            TestSipEndPoint& mEndPoint;
            UInt32 mMinExpires;
      };
      MessageExpectAction* send423Or200ToPublish(int minExpires);

      // 200 to REGISTER
      class Send200ToRegister : public MessageExpectAction
      {
         public:
            Send200ToRegister(TestSipEndPoint& endPoint, const resip::NameAddr& contact);
            Send200ToRegister(TestSipEndPoint& endPoint);
            virtual boost::shared_ptr<resip::SipMessage>
            go(boost::shared_ptr<resip::SipMessage> msg);

            TestSipEndPoint& mEndPoint;
            bool mUseContact;
            resip::NameAddr mContact;
      };
      MessageExpectAction* send200ToRegister(const resip::NameAddr& contact);
      MessageExpectAction* send200ToRegister();

      class Send401 : public MessageExpectAction
      {
         public:
            Send401(TestSipEndPoint& endPoint);
            virtual boost::shared_ptr<resip::SipMessage>
            go(boost::shared_ptr<resip::SipMessage> msg);
            TestSipEndPoint& mEndPoint;
      };
      MessageExpectAction* send401();

      class Notify : public MessageExpectAction
      {
         public:
            Notify(TestSipEndPoint& endPoint, boost::shared_ptr<resip::Contents> contents, const resip::Data& eventPackage, const resip::Data& subscriptionState, int expires, int minExpires, bool firstNotify=false);
            virtual boost::shared_ptr<resip::SipMessage>
            go(boost::shared_ptr<resip::SipMessage> msg);

            TestSipEndPoint& mEndPoint;
            boost::shared_ptr<resip::Contents> mContents;
            resip::Data mEventPackage;
            resip::Data mSubscriptionState;
            UInt32 mExpires;
            UInt32 mMinExpires;
            bool mFirstNotify;
      };
      MessageExpectAction* notify(boost::shared_ptr<resip::Contents> contents, const resip::Data& eventPackage, const resip::Data& subscriptionState, int expires, int minExpires, bool firstNotify=false);

      class Answer : public MessageExpectAction 
      { 
         public:
            explicit Answer(TestSipEndPoint & endPoint, boost::shared_ptr<resip::SdpContents> sdp = boost::shared_ptr<resip::SdpContents>());
            virtual boost::shared_ptr<resip::SipMessage>                                
            go(boost::shared_ptr<resip::SipMessage> msg);
         private:
            TestSipEndPoint& mEndPoint;
            boost::shared_ptr<resip::SdpContents> mSdp;
      };
      MessageExpectAction* answer();
      MessageExpectAction* answer(const boost::shared_ptr<resip::SdpContents>& sdp);

      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Ring, 180);
      MessageExpectAction* ring();

//      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Ring183, 183);
      class Ring183 : public MessageExpectAction
      {
         public:
            explicit Ring183(TestSipEndPoint & endPoint, boost::shared_ptr<resip::SdpContents> sdp = boost::shared_ptr<resip::SdpContents>());
            explicit Ring183(TestSipEndPoint & endPoint, boost::shared_ptr<resip::SdpContents> sdp, int rseq);
            virtual boost::shared_ptr<resip::SipMessage> go(boost::shared_ptr<resip::SipMessage> msg);
         private:
            TestSipEndPoint& mEndPoint;
            boost::shared_ptr<resip::SdpContents> mSdp;
            bool mReliable;
            int mRseq;
      };
      MessageExpectAction* ring183();
      MessageExpectAction* ring183(const boost::shared_ptr<resip::SdpContents>& sdp);
      MessageExpectAction* reliableProvisional(const boost::shared_ptr<resip::SdpContents>& sdp, int rseq);
      
      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Ok, 200);
      MessageExpectAction* ok();

      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send403, 403);
      MessageExpectAction* send403();

      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send404, 404);
      MessageExpectAction* send404();

      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send415, 415);
      MessageExpectAction* send415();

      class Send420 : public MessageExpectAction
      {
         public:
            explicit Send420(TestSipEndPoint& endPoint, const resip::Token& unsupported);
            virtual boost::shared_ptr<resip::SipMessage> go(boost::shared_ptr<resip::SipMessage> msg);
         private:
            TestSipEndPoint& mEndPoint;
            resip::Token mUnsupported;
      };
      MessageExpectAction* send420(const resip::Token& unsupported);
      
      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send480, 480);
      MessageExpectAction* send480();

      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send486, 486);
      MessageExpectAction* send486();

      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send487, 487);
      MessageExpectAction* send487();

      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send488, 488);
      MessageExpectAction* send488();

      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send202, 202);
      MessageExpectAction* send202();

      // vk
      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send200, 200);
      MessageExpectAction* send200();
      // end - vk

      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send100, 100);
      MessageExpectAction* send100();

      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send503, 503);
      MessageExpectAction* send503();

      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send500, 500);
      MessageExpectAction* send500();

      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send513, 513);
      MessageExpectAction* send513();

      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send504, 504);
      MessageExpectAction* send504();

      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send600, 600);
      MessageExpectAction* send600();

      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send603, 603);
      MessageExpectAction* send603();

      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send604, 604);
      MessageExpectAction* send604();

      EXPECT_FUNCTOR_RESPONSE(TestSipEndPoint, Send606, 606);
      MessageExpectAction* send606();

      EXPECT_FUNCTOR(TestSipEndPoint, Dump);
      MessageExpectAction* dump();

      class Ack : public MessageExpectAction
      {
         public:
            explicit Ack(TestSipEndPoint& endPoint, boost::shared_ptr<resip::SdpContents> sdp = boost::shared_ptr<resip::SdpContents>());
            virtual boost::shared_ptr<resip::SipMessage> go(boost::shared_ptr<resip::SipMessage> msg);
         private:
            TestSipEndPoint& mEndPoint;
            boost::shared_ptr<resip::SdpContents> mSdp;
      };
      MessageExpectAction* ack();
      MessageExpectAction* ack(const boost::shared_ptr<resip::SdpContents>& sdp);

      EXPECT_FUNCTOR(TestSipEndPoint, AckReferred);
      MessageExpectAction* ackReferred();

      EXPECT_FUNCTOR_TARGETED(TestSipEndPoint, ByeTo);
      ExpectAction* bye(const resip::Uri& target);
      ExpectAction* bye(const TestSipEndPoint* target);
      ExpectAction* bye(const TestSipEndPoint& target);

      EXPECT_FUNCTOR(TestSipEndPoint, Bye);
      MessageExpectAction* bye();

      EXPECT_FUNCTOR(TestSipEndPoint, Cancel);
      MessageExpectAction* cancel();

      EXPECT_FUNCTOR_TARGETED(TestSipEndPoint, Notify200To);
      ExpectAction* notify200(const resip::Uri& target);
      ExpectAction* notify200(const TestSipEndPoint& target);

      EXPECT_FUNCTOR(TestSipEndPoint, Notify200); 
      MessageExpectAction* notify200();

      class Matcher
      {
         public:
            virtual ~Matcher() {}
            virtual bool isMatch(boost::shared_ptr<resip::SipMessage>& message) const = 0;
            virtual resip::Data toString() const = 0;
      };

      class AlwaysMatches : public Matcher
      {
         public:
            AlwaysMatches(){}

            virtual bool isMatch(boost::shared_ptr<resip::SipMessage>& message) const { return true; }
            virtual resip::Data toString() const { return "AlwayMatches";}
      };

      class MatchNonceCount : public Matcher
      {
         public:
            MatchNonceCount(int count) : mCount(count) {}
            virtual bool isMatch(boost::shared_ptr<resip::SipMessage>& message) const ;
            virtual resip::Data toString() const;
         private:
            int mCount;
      };

      class SaveMatcher : public Matcher
      {
         public:
            SaveMatcher(Matcher* matcher, boost::shared_ptr<resip::SipMessage>& msg);
            virtual bool isMatch(boost::shared_ptr<resip::SipMessage>& message) const;
            virtual resip::Data toString() const;

         private:
            boost::shared_ptr<resip::SipMessage>& mMsg;
            Matcher* mMatcher;
      };
         
      class From : public Matcher
      {
         public:
            From(const TestSipEndPoint& testEndPoint);
            From(TestProxy& testProxy);
            From(const resip::Uri& contact);
            From(const resip::Data& instanceId);
            
            virtual bool isMatch(boost::shared_ptr<resip::SipMessage>& message) const;
            virtual resip::Data toString() const;

         private:
            const TestSipEndPoint* mEndPoint;
            TestProxy* mProxy;
            const resip::Uri mContact;
            const resip::Data mInstanceId;
            
            // value semantics
      };

      class Contact : public Matcher
      {
         public:
            Contact(const TestSipEndPoint& testEndPoint);
            Contact(TestProxy& testProxy);
            Contact(const resip::Uri& contact);

            virtual bool isMatch(boost::shared_ptr<resip::SipMessage>& message) const;
            virtual resip::Data toString() const;

         private:
            const TestSipEndPoint* mEndPoint;
            TestProxy* mProxy;
            const resip::Uri mContact;
            
            // value semantics
      };

      class HasMessageBodyMatch : public Matcher
      {
         public:
            HasMessageBodyMatch() {}

            virtual bool isMatch(boost::shared_ptr<resip::SipMessage>& message) const;
            virtual resip::Data toString() const;
      };

      class ToMatch : public TestEndPoint::ExpectPreCon
      {
         public:
            ToMatch(const TestSipEndPoint& tua, const resip::Uri& to);
            virtual resip::Data toString() const;
            virtual bool passes(boost::shared_ptr<Event> event);

         private:
            const TestSipEndPoint* mEndPoint;
            const resip::Uri mTo;
      };
      // predicate to examine subject of Pidf
      ToMatch& toMatch(const resip::Uri& to);

      class SipExpect : public ExpectBase
      {
         public:
            SipExpect(TestSipEndPoint& endPoint, 
                      std::pair<resip::MethodTypes, int> msgTypeCode, 
                      Matcher* matcher,
                      ExpectPreCon& preCon,
                      int timeoutMs,
                      ActionBase* expectAction);
            virtual ~SipExpect();
            virtual TestEndPoint* getEndPoint() const;
            virtual unsigned int getTimeout() const;

            /// determine if the message matches (checks from and message type)
            virtual bool isMatch(boost::shared_ptr<Event> event) const;
            virtual resip::Data explainMismatch(boost::shared_ptr<Event> event) const;

            virtual void onEvent(TestEndPoint&, boost::shared_ptr<Event> event);
            
            int getStatusCode() const { return mMsgTypeCode.second; }
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
            TestSipEndPoint& mEndPoint;
            std::pair<resip::MethodTypes, int> mMsgTypeCode;
            Matcher* mMatcher;
            ExpectPreCon& mPreCon;
            unsigned int mTimeoutMs;
            ActionBase* mExpectAction;
      };

      // syntactic sugar
      ExpectBase* expect(resip::MethodTypes msgType, 
                         Matcher* matcher, 
                         int timeoutMs, 
                         ActionBase* expectAction);
      ExpectBase* expect(resip::MethodTypes msgType, 
                         Matcher* matcher, 
                         ExpectPreCon& pred, 
                         int timeoutMs, 
                         ActionBase* expectAction);

      ExpectBase* expect(std::pair<resip::MethodTypes, int> msgTypeCode, 
                         Matcher* matcher, 
                         int timeoutMs, 
                         ActionBase* expectAction);
      ExpectBase* expect(std::pair<resip::MethodTypes, int> msgTypeCode, 
                         Matcher* matcher, 
                         ExpectPreCon& pred, 
                         int timeoutMs, 
                         ActionBase* expectAction);

      virtual void process(resip::FdSet& fdset);
      virtual void buildFdSet(resip::FdSet& fdset);
      virtual void handleEvent(boost::shared_ptr<Event> event);
      
      virtual resip::Data getName() const;
      int getPort() const ;
      const resip::NameAddr & getContact() const;
      const std::set<resip::NameAddr> & getDefaultContacts() const;
      const resip::Uri& getAddressOfRecord() const;
      const resip::Uri& getUri() const;
      resip::Data getAddressOfRecordString() const;
      
      // !bwc! If rawData is specified, do all target resolution steps
      // based on sipMessage, but put the bits in rawData on the wire.
      virtual void send(boost::shared_ptr<resip::SipMessage>& sipMessage,
                        const resip::Data rawData=resip::Data::Empty);
      
      // !dlb! need to shove the interface through MessageAction
      void storeSentSubscribe(const boost::shared_ptr<resip::SipMessage>& subscribe);

   protected:
      void storeSentInvite(const boost::shared_ptr<resip::SipMessage>& invite);
      void storeReceivedInvite(const boost::shared_ptr<resip::SipMessage>& invite);
      
      boost::shared_ptr<resip::SipMessage> getSentInvite(const resip::CallId& callId);
      boost::shared_ptr<resip::SipMessage> getReceivedInvite(const resip::CallId& callId);

      //void storeSentSubscribe(const boost::shared_ptr<resip::SipMessage>& subscribe);
      void storeReceivedSubscribe(const boost::shared_ptr<resip::SipMessage>& subscribe);

      void storeReceivedPublish(const boost::shared_ptr<resip::SipMessage>& publish);

      boost::shared_ptr<resip::SipMessage> getSentSubscribe(boost::shared_ptr<resip::SipMessage> msg);
      boost::shared_ptr<resip::SipMessage> getReceivedSubscribe(const resip::CallId& callId);
      boost::shared_ptr<resip::SipMessage> getReceivedPublish(const resip::CallId& callId);
      boost::shared_ptr<resip::SipMessage> makeResponse(resip::SipMessage& request, int code);
      
      resip::DeprecatedDialog* getDialog(); // get the first dialog
      resip::DeprecatedDialog* getDialog(const resip::CallId& callId);
      resip::DeprecatedDialog* getDialog(const resip::Uri& target);      
      resip::DeprecatedDialog* getDialog(const resip::NameAddr& target);      
      resip::DeprecatedDialog* getDialog(const resip::Data& user);

      resip::Uri mAor;
      resip::NameAddr mContact;
      resip::Uri mOutboundProxy;
      
      std::set<resip::NameAddr> mContactSet;

      resip::Fifo<resip::TransactionMessage> mIncoming;
      resip::Transport* mTransport;
      resip::Security* mSecurity;
      
      typedef std::list< boost::shared_ptr<resip::SipMessage> > InviteList;
      typedef std::list< boost::shared_ptr<resip::SipMessage> > ReceivedSubscribeList;
      typedef std::list< boost::shared_ptr<resip::SipMessage> > ReceivedPublishList;
      typedef std::list<DialogSet> SentSubscribeList;
      typedef std::list<resip::DeprecatedDialog*> DialogList;
      typedef HashMap<resip::Data, boost::shared_ptr<resip::SipMessage> > RequestMap;

   protected:
      InviteList mInvitesSent;
      InviteList mInvitesReceived;
      
   public: // !dlb! hack
      DialogList mDialogs;
      SentSubscribeList mSubscribesSent;
   protected:
      
      ReceivedSubscribeList mSubscribesReceived;
      ReceivedPublishList mPublishReceived;
      RequestMap mRequests;
      
      static boost::shared_ptr<resip::SipMessage> nil;

      // disabled
      TestSipEndPoint(const TestSipEndPoint&);
      TestSipEndPoint& operator=(const TestSipEndPoint&);
};

/// SIP status codes in similar form to requests
enum {SIP_100 = 100,
      SIP_180 = 180,
      SIP_183 = 183,
      SIP_200 = 200,
      SIP_202 = 202,
      SIP_301 = 301, // vk
      SIP_302 = 302,
      SIP_400 = 400,
      SIP_402 = 402,
      SIP_403 = 403,
      SIP_404 = 404,
      SIP_407 = 407,
      SIP_408 = 408,
      SIP_413 = 413,
      SIP_480 = 480,
      SIP_481 = 481,
      SIP_482 = 482,
      SIP_483 = 483,
      SIP_486 = 486,
      SIP_487 = 487,
      SIP_500 = 500,
      SIP_503 = 503};

TestSipEndPoint::From* from(const TestSipEndPoint& testEndPoint);
TestSipEndPoint::From* from(TestProxy& testProxy);
TestSipEndPoint::From* from(const resip::NameAddr& contact);
TestSipEndPoint::From* from(const TestSipEndPoint* testEndPoint);
TestSipEndPoint::From* from(TestProxy* testProxy);
TestSipEndPoint::From* from(const resip::NameAddr* contact);
TestSipEndPoint::From* from(const resip::Uri& clientUri);
TestSipEndPoint::From* from(const resip::Data& instanceId);

TestSipEndPoint::SaveMatcher* saveMatcher(TestSipEndPoint::Matcher* matcher, boost::shared_ptr<resip::SipMessage>& msg);

TestSipEndPoint::AlwaysMatches* alwaysMatches();

inline TestSipEndPoint::MatchNonceCount* matchNonceCount(int count) { return new TestSipEndPoint::MatchNonceCount(count); } 

TestSipEndPoint::Contact* contact(const TestSipEndPoint& testEndPoint);
TestSipEndPoint::Contact* contact(TestProxy& testProxy);
TestSipEndPoint::Contact* contact(const resip::NameAddr& contact);
TestSipEndPoint::Contact* contact(const TestSipEndPoint* testEndPoint);
TestSipEndPoint::Contact* contact(TestProxy* testProxy);
TestSipEndPoint::Contact* contact(const resip::NameAddr* contact);

TestSipEndPoint::HasMessageBodyMatch* hasMessageBodyMatch();

TestSipEndPoint::MessageAction*
condition(TestSipEndPoint::MessageConditionerFn fn, 
          TestSipEndPoint::MessageAction* action);

TestSipEndPoint::RawSend*
condition(TestSipEndPoint::RawConditionerFn fn, 
          TestSipEndPoint::RawSend* action);

TestSipEndPoint::MessageAction*
save(boost::shared_ptr<resip::SipMessage>& msgPtr, 
     TestSipEndPoint::MessageAction* action);

TestSipEndPoint::MessageAction*
operator<=(boost::shared_ptr<resip::SipMessage>& msgPtr, 
           TestSipEndPoint::MessageAction* action);

TestSipEndPoint::MessageExpectAction*
condition(TestSipEndPoint::MessageConditionerFn fn, 
          TestSipEndPoint::MessageExpectAction* action);

TestSipEndPoint::MessageExpectAction*
save(boost::shared_ptr<resip::SipMessage>& msgPtr, 
     TestSipEndPoint::MessageExpectAction* action);

TestSipEndPoint::MessageExpectAction*
operator<=(boost::shared_ptr<resip::SipMessage>& msgPtr, 
           TestSipEndPoint::MessageExpectAction* action);

// syntactic sugar for passing method and response code
// as e.g.
// CANCEL/200
std::pair<resip::MethodTypes, int>
operator/(resip::MethodTypes meth, int code);

// syntactic sugar for composing conditions
TestSipEndPoint::MessageConditionerFn
compose(TestSipEndPoint::MessageConditionerFn fn2,
        TestSipEndPoint::MessageConditionerFn fn1);

TestSipEndPoint::MessageConditionerFn
compose(TestSipEndPoint::MessageConditionerFn fn3,
        TestSipEndPoint::MessageConditionerFn fn2,
        TestSipEndPoint::MessageConditionerFn fn1);

TestSipEndPoint::MessageConditionerFn
compose(TestSipEndPoint::MessageConditionerFn fn4,
        TestSipEndPoint::MessageConditionerFn fn3,
        TestSipEndPoint::MessageConditionerFn fn2,
        TestSipEndPoint::MessageConditionerFn fn1);

#endif // TestSipEndPoint_hxx

#ifdef RTP_ON
class CreateRtpSession : public ExpectAction
{
   public:
      CreateRtpSession(TestSipEndPoint* endPoint, const boost::shared_ptr<resip::SdpContents>& localSdp)
         : mEndPoint(endPoint), mLocalSdp(localSdp) {}

      virtual void operator()(boost::shared_ptr<Event> event);
      virtual resip::Data toString() const;
   private:
      TestSipEndPoint* mEndPoint;
      boost::shared_ptr<resip::SdpContents> mLocalSdp;
};
friend class CreateRtpSession;     

CreateRtpSession* createRtpSession(const boost::shared_ptr<resip::SdpContents>& localSdp) 
{ 
   return new CreateRtpSession(this, localSdp);
}

class SendDtmf : public ExpectAction
{
   public:
      typedef vector< pair<char,int> > DtmfSequence;

      SendDtmf(TestSipEndPoint* endPoint, const DtmfSequence& dtmfString);


      virtual void operator()(boost::shared_ptr<Event> event);
   private:
      TestSipEndPoint* mEndPoint;
      DtmfSequence mDtmfString;

      class SendDtmfChar : public SipEndPointAction
      {
         public:
            SendDtmfChar(TestSipEndPoint* endpoint, int dtmf) 
               : SipEndPointAction(endpoint), mDtmf(dtmf) {}
                  
            virtual void operator()(TestSipEndPoint& endPoint);
                  
            int mDtmf;
      };
};
friend class SendDtmf;     

SendDtmf* sendDtmf(const SendDtmf::DtmfSequence& dSeq)
{ 
   return new SendDtmf(this, dSeq);
}
#endif

// Copyright 2005 Purplecomm, Inc.
/*
  Copyright (c) 2005, PurpleComm, Inc. 
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
  * Neither the name of PurpleComm, Inc. nor the names of its contributors may
  be used to endorse or promote products derived from this software without
  specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
