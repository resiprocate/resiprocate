#if !defined(TestInviteSession_hxx)
#define TestInviteSession_hxx

#include "resip/dum/Handles.hxx"
#include "TestUsage.hxx"
#include "tfm/EndPoint.hxx"
#include "InviteSessionEvent.hxx"
#include "DumUaAction.hxx"

class DumUserAgent;
class MessageMatcher;
class WarningCategory;

class CommonAction;

class TestInviteSession : public TestUsage
{
   public:
      TestInviteSession(DumUserAgent*);
      virtual ~TestInviteSession() {}

      //resip::Data getName() const { return "TestInviteSession"; }
      
      virtual CommonAction* provideOffer(const resip::SdpContents& offer, resip::DialogUsageManager::EncryptionLevel level, resip::SdpContents* alternative);
      virtual CommonAction* provideAnswer(const resip::SdpContents& answer);
      virtual CommonAction* end(resip::InviteSession::EndReason = resip::InviteSession::NotSpecified);
      virtual CommonAction* reject(int statusCode, resip::WarningCategory* warning);

      CommonAction* requestOffer();
      CommonAction* targetRefresh(const resip::NameAddr& localUri);
      CommonAction* refer(const resip::NameAddr& referTo, bool referSub=true);
      CommonAction* refer(const resip::NameAddr& referTo, resip::InviteSessionHandle sessionToReplace, bool referSub = true);
      CommonAction* info(const resip::Contents& contents);
      CommonAction* message(const resip::Contents& contents);
      CommonAction* acceptNIT(int statusCode, const resip::Contents* contents);
      CommonAction* rejectNIT(int statusCode);

      CommonAction* acceptReferNoSub(int statusCode=200);
      CommonAction* rejectReferNoSub(int responseCode);

      SendingAction<resip::ServerSubscriptionHandle>* acceptRefer(int statusCode = 200);
      SendingAction<resip::ServerSubscriptionHandle>* rejectRefer(int responseCode);

      //SendingCommand* inviteFromRefer(const resip::SdpContents*);

      resip::InviteSessionHandle getHandle() const { return mSessionHandle; }
      resip::ClientSubscriptionHandle& getClientSubscription() { return mClientSubscription; }
      resip::ServerSubscriptionHandle& getServerSubscription() { return mServerSubscription; }
      resip::SipMessage& getReferMessage() { return mReferMessage; }

      virtual bool isMyEvent(Event*) = 0;

   protected:
      friend class DumUserAgent;      
      resip::InviteSessionHandle& getSessionHandleRef() { return mSessionHandle; }
      void setClientSubscription(resip::ClientSubscriptionHandle h) { mClientSubscription = h; }
      void setServerSubscription(resip::ServerSubscriptionHandle h) { mServerSubscription = h; }
      void setReferMessage(const resip::SipMessage& refer) { mReferMessage = refer; }

      resip::InviteSessionHandle mSessionHandle;
      resip::ClientSubscriptionHandle mClientSubscription;
      resip::ServerSubscriptionHandle mServerSubscription;
      resip::SipMessage mReferMessage;
};

class TestClientInviteSession : public TestInviteSession
{
   public:
      TestClientInviteSession(DumUserAgent*);
      virtual ~TestClientInviteSession() {}

      resip::Data getName() const { return "TestClientInviteSession"; }

      CommonAction* provideOffer(const resip::SdpContents& offer, resip::DialogUsageManager::EncryptionLevel level = resip::DialogUsageManager::None, 
                                 resip::SdpContents* alternative = 0);
      CommonAction* provideAnswer(const resip::SdpContents& answer);
      CommonAction* end(resip::InviteSession::EndReason = resip::InviteSession::NotSpecified);
      CommonAction* reject(int statusCode, resip::WarningCategory* warning=0);

      bool isMyEvent(Event*);

      ExpectBase* expect(InviteEvent::Type,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(InviteEvent::Type,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(InviteEvent::Type,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      resip::ClientInviteSessionHandle getHandle() const { return mHandle; }
      resip::InviteSessionHandle getSessionHandle() const { return mSessionHandle; }
            
   private:
      friend class DumUserAgent;
      resip::ClientInviteSessionHandle& getHandleRef() { return mHandle; }
      resip::ClientInviteSessionHandle mHandle;
};

class TestServerInviteSession : public TestInviteSession
{
   public:
      TestServerInviteSession(DumUserAgent*);
      virtual ~TestServerInviteSession() {}

      resip::Data getName() const { return "TestServerInviteSession"; }

      CommonAction* provideOffer(const resip::SdpContents& offer, resip::DialogUsageManager::EncryptionLevel level = resip::DialogUsageManager::None, 
                                 resip::SdpContents* alternative = 0, bool sendOfferAtAccept = false);
      CommonAction* provideAnswer(const resip::SdpContents& answer);
      CommonAction* end(resip::InviteSession::EndReason = resip::InviteSession::NotSpecified);
      CommonAction* reject(int statusCode, resip::WarningCategory* warning=0);
      CommonAction* accept(int statusCode=200);
      CommonAction* redirect(const resip::NameAddrs& contacts, int code=302);
      CommonAction* provisional(int code=180, bool earlyFlag = false);

      bool isMyEvent(Event*);

      ExpectBase* expect(InviteEvent::Type,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(InviteEvent::Type,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(InviteEvent::Type,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      resip::ServerInviteSessionHandle getHandle() const { return mHandle; }
      resip::InviteSessionHandle getSessionHandle() const { return mSessionHandle; }
      
   private:
      friend class DumUserAgent;
      resip::ServerInviteSessionHandle& getHandleRef() { return mHandle; }
      resip::ServerInviteSessionHandle mHandle;
};

#endif 
