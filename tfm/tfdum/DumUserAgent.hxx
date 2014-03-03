#if !defined(DumUserAgent_hxx)
#define DumUserAgent_hxx

#include "resip/stack/EventStackThread.hxx"
#include "resip/dum/DialogEventHandler.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/SubscriptionHandler.hxx"
#include "resip/dum/PublicationHandler.hxx"
#include "resip/dum/OutOfDialogHandler.hxx"
#include "resip/dum/PagerMessageHandler.hxx"
#include "resip/dum/Handles.hxx"
#include "resip/dum/DialogSetHandler.hxx"
#include "resip/dum/DialogEventInfo.hxx"
#include "tfm/TestEndPoint.hxx"
#include "tfm/TransportDriver.hxx"
#include "tfm/EndPoint.hxx"

#include "tfm/tfdum/ClientSubscriptionEvent.hxx"
#include "tfm/tfdum/ClientRegistrationEvent.hxx"
#include "tfm/tfdum/TestClientSubscription.hxx"
#include "tfm/tfdum/TestServerSubscription.hxx"
#include "tfm/tfdum/TestClientRegistration.hxx"
#include "tfm/tfdum/InviteSessionEvent.hxx"
#include "tfm/tfdum/TestInviteSession.hxx"
#include "tfm/tfdum/TestClientPublication.hxx"
#include "tfm/tfdum/TestServerOutOfDialogReq.hxx"
#include "tfm/tfdum/TestClientPagerMessage.hxx"
#include "tfm/tfdum/TestServerPagerMessage.hxx"

#include "tfm/tfdum/ClientOutOfDialogReqEvent.hxx"

#include "DumEvent.hxx"
#include "DumExpect.hxx"

#include "rutil/SharedPtr.hxx"

class TestProxy;
class DumUaAction;

class DumUserAgent : public EndPoint,
                     public TransportDriver::Client,
                     public resip::ClientRegistrationHandler, 
                     public resip::InviteSessionHandler,
                     public resip::ClientSubscriptionHandler,
                     public resip::ServerSubscriptionHandler,
                     public resip::ClientPublicationHandler,
                     public resip::DialogSetHandler,
                     public resip::OutOfDialogHandler,
                     public resip::ClientPagerMessageHandler,
                     public resip::ServerPagerMessageHandler,
                     public resip::DialogEventHandler
{
   public:
      DumUserAgent(resip::SharedPtr<resip::MasterProfile> profile);

      DumUserAgent(resip::SharedPtr<resip::MasterProfile> profile,
                   TestProxy* proxy);

      ~DumUserAgent();

      void init();
      void add(TestUsage*);
      void remove(TestUsage*);

      void addClientSubscriptionHandler(const resip::Data& eventType, resip::ClientSubscriptionHandler*);
      void addClientPublicationHandler(const resip::Data& eventType, resip::ClientPublicationHandler*);
      void addServerSubscriptionHandler(const resip::Data& eventType, resip::ServerSubscriptionHandler*);
      void addServerPublicationHandler(const resip::Data& eventType, resip::ServerPublicationHandler*);
      void addClientPagerMessageHandler(resip::ClientPagerMessageHandler*);
      void addServerPagerMessageHandler(resip::ServerPagerMessageHandler*);
      void addOutOfDialogHandler(resip::MethodTypes methodType, resip::OutOfDialogHandler*);

      void addSupportedOptionTag(const resip::Token& tag);

      void setOutboundProxy(const resip::Uri& proxy);
      void unsetOutboundProxy();
      void setDefaultFrom(const resip::Uri& from);

      resip::ClientPagerMessageHandle makePagerMessage(const resip::NameAddr& target);

      bool matchEvent(Event*);

      void handleEvent(Event* eventRaw);

      void buildFdSet(resip::FdSet& fdset);

      void process(resip::FdSet& fdset);

      const resip::Data& getInstanceId() const;
      resip::Uri getContact() const;

      static resip::SharedPtr<resip::MasterProfile> makeProfile(const resip::Uri& aor, const resip::Data& password);

      resip::SharedPtr<resip::MasterProfile> getProfile() const 
      {
         return mProfile;
      }

      const resip::NameAddr& getAor() const;

      // special handling required for UAC Prack scenario - must call provideOffer from 
      // onAnswer callback in order to get Offer to be provided in first PRACK
      ExpectAction* setOfferToProvideInNextOnAnswerCallback(boost::shared_ptr<resip::SdpContents> offer);

//      DumUaAction* start();
      DumUaAction* shutdownUa();

      DumUaAction* invite(const resip::NameAddr& target, 
                              const resip::SdpContents* initialOffer, 
                              const resip::SdpContents* alternative = 0, 
                              resip::DialogUsageManager::EncryptionLevel level = resip::DialogUsageManager::None);


      DumUaAction* anonInvite(const resip::NameAddr& target, 
                              const resip::SdpContents* initialOffer, 
                              const resip::SdpContents* alternative = 0, 
                              resip::DialogUsageManager::EncryptionLevel level = resip::DialogUsageManager::None);

      DumUaAction* inviteFromRefer(const resip::SipMessage& refer, resip::ServerSubscriptionHandle& h, 
                                   const resip::SdpContents* initialOffer,
                                   resip::DialogUsageManager::EncryptionLevel level = resip::DialogUsageManager::None,
                                   const resip::SdpContents* alternative = 0);
            
      DumUaAction* subscribe(const resip::NameAddr& target, const resip::Data& eventType);
      DumUaAction* publish(const resip::NameAddr& target, const resip::Contents& body, 
                           const resip::Data& eventType, unsigned expiresSeconds);

      DumUaAction* cancelInvite();

      DumUaAction* refer(const resip::NameAddr& target, const resip::NameAddr& referTo);
      DumUaAction* referNoReferSub(const resip::NameAddr& target, const resip::NameAddr& referTo);
      DumUaAction* referNoReferSubWithoutReferSubHeader(const resip::NameAddr& target, const resip::NameAddr& referTo);

      DumUaAction* send(resip::SharedPtr<resip::SipMessage> msg);

      class From : public MessageMatcher
      {
         public:
            From(const DumUserAgent& dua);
            From(TestProxy* proxy);
            //From(const resip::Uri& contact);

            bool isMatch(const boost::shared_ptr<resip::SipMessage>& message) const;
            resip::Data toString() const;

         private:
            const DumUserAgent* mUa;
            TestProxy* mProxy;
            //const resip::Uri mContact;
      };

      /*
      class HasInstanceId : public MessageMatcher
      {
         public:
            HasInstanceId() {}

            bool isMatch(const boost::shared_ptr<resip::SipMessage>& message) const;
            resip::Data toString() const;

         private:
      };

      class NoInstanceId : public MessageMatcher
      {
         public:
            NoInstanceId() {}

            bool isMatch(const boost::shared_ptr<resip::SipMessage>& message) const;
            resip::Data toString() const;
      };
      */

      class FindMatchingDialogToReplace : public MessageMatcher
      {
         public:
            FindMatchingDialogToReplace(DumUserAgent& dua);
            bool isMatch(const boost::shared_ptr<resip::SipMessage>& message) const;
            resip::Data toString() const;

         private:
            DumUserAgent* mUa;
      };

      class HasRinstance : public MessageMatcher
      {
         public:
            HasRinstance() {}

            bool isMatch(const boost::shared_ptr<resip::SipMessage>& message) const;
            resip::Data toString() const;

         private:
      };

      class NoRinstance : public MessageMatcher
      {
         public:
            NoRinstance() {}

            bool isMatch(const boost::shared_ptr<resip::SipMessage>& message) const;
            resip::Data toString() const;
      };

      class HasMethodsParam : public MessageMatcher
      {
         public:
            HasMethodsParam() {}

            bool isMatch(const boost::shared_ptr<resip::SipMessage>& message) const;
            resip::Data toString() const;

         private:
      };

      class NoMethodsParam : public MessageMatcher
      {
         public:
            NoMethodsParam() {}

            bool isMatch(const boost::shared_ptr<resip::SipMessage>& message) const;
            resip::Data toString() const;
      };

      // expects
      // ClientRegistration

      ExpectBase* expect(ClientRegistrationEvent::Type, 
                         MessageMatcher* matcher, 
                         int timeoutMs, 
                         ActionBase* expectAction);
      
      ExpectBase* expect(ClientRegistrationEvent::Type, 
                         TestClientRegistration& clientReg,
                         MessageMatcher* matcher, 
                         int timeoutMs, 
                         ActionBase* expectAction);
      
      ExpectBase* expect(ClientRegistrationEvent::Type, 
                         TestClientRegistration& clientReg,
                         MessageMatcher* matcher, 
                         ExpectPreCon& pred,
                         int timeoutMs, 
                         ActionBase* expectAction);
      
      ExpectBase* expect(ClientRegistrationEvent::Type, 
                         TestClientRegistration& clientReg,
                         ExpectPreCon& pred, 
                         int timeoutMs, 
                         ActionBase* expectAction);

      // InviteSession
      ExpectBase* expect(InviteEvent::Type,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(InviteEvent::Type,
                         TestClientInviteSession& clientInv,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(InviteEvent::Type,
                         TestClientInviteSession& clientInv,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(InviteEvent::Type,
                         TestClientInviteSession& clientInv,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(InviteEvent::Type,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(InviteEvent::Type,
                         TestServerInviteSession& serverInv,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(InviteEvent::Type,
                         TestServerInviteSession& serverInv,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(InviteEvent::Type,
                         TestServerInviteSession& serverInv,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      // ClientPublication
      ExpectBase* expect(ClientPublicationEvent::Type,
                         TestClientPublication& clientPub,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ClientPublicationEvent::Type,
                         TestClientPublication& clientPub,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ClientPublicationEvent::Type,
                         TestClientPublication& clientPub,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      // ClientSubscription
      ExpectBase* expect(ClientSubscriptionEvent::Type,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);
      
      ExpectBase* expect(ClientSubscriptionEvent::Type,
                         TestClientSubscription& clientSub,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ClientSubscriptionEvent::Type,
                         TestClientSubscription& clientSub,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ClientSubscriptionEvent::Type,
                         TestClientSubscription& clientSub,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      // ServerSubscription
      ExpectBase* expect(ServerSubscriptionEvent::Type,
                         TestServerSubscription& clientSub,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ServerSubscriptionEvent::Type,
                         TestServerSubscription& clientSub,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ServerSubscriptionEvent::Type,
                         TestServerSubscription& clientSub,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      // ClientPagerMessage
      ExpectBase* expect(ClientPagerMessageEvent::Type,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ClientPagerMessageEvent::Type,
                         TestClientPagerMessage& clientPager,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ClientPagerMessageEvent::Type,
                         TestClientPagerMessage& clientPager,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ClientPagerMessageEvent::Type,
                         TestClientPagerMessage& clientPager,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      // SeverPagerMessage
      ExpectBase* expect(ServerPagerMessageEvent::Type,
                         TestServerPagerMessage& svrPager,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ServerPagerMessageEvent::Type,
                         TestServerPagerMessage& svrPager,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ServerPagerMessageEvent::Type,
                         TestServerPagerMessage& svrPager,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      // ClientOutOfDialog
      ExpectBase* expect(ClientOutOfDialogReqEvent::Type,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ClientOutOfDialogReqEvent::Type,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ClientOutOfDialogReqEvent::Type,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      // ServerOutOfDialogReq
      ExpectBase* expect(ServerOutOfDialogReqEvent::Type,
                         TestServerOutOfDialogReq& serverReq,
                         MessageMatcher* matcher,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ServerOutOfDialogReqEvent::Type,
                         TestServerOutOfDialogReq& serverReq,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(ServerOutOfDialogReqEvent::Type,
                         TestServerOutOfDialogReq& serverReq,
                         MessageMatcher* matcher,
                         ExpectPreCon& pred,
                         int timeoutMs,
                         ActionBase* expectAction);
            
      DumUaAction* registerUa(bool tcp=false); //initial registration

      /*
      //actions from ClintPagerMessage
      DumUaAction* getMessageRequest();
      DumUaAction* page(std::auto_ptr<resip::Contents> contenst, resip::DialogUsageManager::EncryptionLevel level=resip::DialogUsageManager::None);
      DumUaAction* endClientPagerMsg();
      
      //actions from ServerPagerMessage
      DumUaAction* acceptServerPagerMsg(int statusCode = 200);
      DumUaAction* rejectServerPagerMsg(int statusCode);
      DumUaAction* endServerPagerMsg();
      DumUaAction* sendServerPagerMsg(resip::SharedPtr<resip::SipMessage> msg);
      */

      //actions from ClientOutOfDialogReq
      //DumUaAction* endClientOutOfDialogReq();

      /*
      //actions from ServerOutOfDialogReq
      DumUaAction* acceptServerOutOfDialogReq(int statusCode = 200);
      DumUaAction* rejectServerOutOfDialogReq(int statusCode);
      DumUaAction* endServerOutOfDialogReq();
      DumUaAction* answerOptions();
	  DumUaAction* sendServerOutOfDialogReq(resip::SharedPtr<resip::SipMessage>
      msg);
      */

      virtual resip::Data getName() const;

      //resip::ClientPagerMessageHandle mClientPagerMessage;
      //resip::ServerPagerMessageHandle mServerPagerMessage;
      //resip::ClientOutOfDialogReqHandle mClientOutOfDialogReq;
      //resip::ServerOutOfDialogReqHandle mServerOutOfDialogReq;

      resip::DialogUsageManager& getDum() { return *mDum; }
      const resip::Data& getIp() const { return mIp; }
      int getPort() const { return mPort; }

      virtual void clean();

      resip::ServerSubscriptionHandle& getServerSubscription() { return mServerSubscription; }
      resip::SipMessage& getReferMessage() { return mReferMessage; }

   protected:

      // resip::Client Registration Handler
      virtual void onSuccess(resip::ClientRegistrationHandle, const resip::SipMessage& response);
      virtual void onRemoved(resip::ClientRegistrationHandle, const resip::SipMessage& response);
      virtual void onFailure(resip::ClientRegistrationHandle, const resip::SipMessage& response);      

      virtual int onRequestRetry(resip::ClientRegistrationHandle, int retrySeconds, const resip::SipMessage& response) 
      {
         return -1;
      }

      // InviteSessionHandler
      virtual void onNewSession(resip::ClientInviteSessionHandle, resip::InviteSession::OfferAnswerType oat, const resip::SipMessage& msg);
      virtual void onNewSession(resip::ServerInviteSessionHandle, resip::InviteSession::OfferAnswerType oat, const resip::SipMessage& msg);
      virtual void onFailure(resip::ClientInviteSessionHandle, const resip::SipMessage& msg);
      virtual void onEarlyMedia(resip::ClientInviteSessionHandle, const resip::SipMessage&, const resip::SdpContents&);
      virtual void onProvisional(resip::ClientInviteSessionHandle, const resip::SipMessage&);
      virtual void onConnected(resip::ClientInviteSessionHandle, const resip::SipMessage& msg);
      virtual void onConnected(resip::InviteSessionHandle, const resip::SipMessage& msg);
      virtual void onStaleCallTimeout(resip::ClientInviteSessionHandle h);
      virtual void onTerminated(resip::InviteSessionHandle, resip::InviteSessionHandler::TerminatedReason reason, const resip::SipMessage* related=0);
      virtual void onForkDestroyed(resip::ClientInviteSessionHandle);
      virtual void onRedirected(resip::ClientInviteSessionHandle, const resip::SipMessage& msg);
      //virtual void onReadyToSend(resip::InviteSessionHandle, const resip::SipMessage& msg);
      virtual void onAnswer(resip::InviteSessionHandle, const resip::SipMessage& msg, const resip::SdpContents&);
      virtual void onOffer(resip::InviteSessionHandle, const resip::SipMessage& msg, const resip::SdpContents&);
      virtual void onRemoteSdpChanged(resip::InviteSessionHandle, const resip::SipMessage& msg, const resip::SdpContents&);
      virtual void onOfferRequestRejected(resip::InviteSessionHandle, const resip::SipMessage& msg);
      virtual void onOfferRequired(resip::InviteSessionHandle, const resip::SipMessage& msg);
      virtual void onOfferRejected(resip::InviteSessionHandle, const resip::SipMessage* msg);
      virtual void onInfo(resip::InviteSessionHandle, const resip::SipMessage& msg);
      virtual void onInfoSuccess(resip::InviteSessionHandle, const resip::SipMessage& msg);
      virtual void onInfoFailure(resip::InviteSessionHandle, const resip::SipMessage& msg);
      virtual void onMessage(resip::InviteSessionHandle, const resip::SipMessage& msg);
      virtual void onMessageSuccess(resip::InviteSessionHandle, const resip::SipMessage& msg);
      virtual void onMessageFailure(resip::InviteSessionHandle, const resip::SipMessage& msg);
      virtual void onRefer(resip::InviteSessionHandle, resip::ServerSubscriptionHandle, const resip::SipMessage& msg);
      virtual void onReferRejected(resip::InviteSessionHandle, const resip::SipMessage& msg);
      virtual void onReferAccepted(resip::InviteSessionHandle, resip::ClientSubscriptionHandle, const resip::SipMessage& msg);
//      virtual void onAckNotReceived(resip::InviteSessionHandle);
      virtual void onIllegalNegotiation(resip::InviteSessionHandle, const resip::SipMessage& msg);
      virtual void onSessionExpired(resip::InviteSessionHandle);
      virtual void onReferNoSub(resip::InviteSessionHandle, const resip::SipMessage& msg);
      virtual void onPrack(resip::ServerInviteSessionHandle, const resip::SipMessage &msg);

      // ClientPublicationHandler
      virtual void onSuccess(resip::ClientPublicationHandle, const resip::SipMessage&);
      virtual void onRemove(resip::ClientPublicationHandle, const resip::SipMessage&);
      virtual void onFailure(resip::ClientPublicationHandle, const resip::SipMessage&);
      virtual int onRequestRetry(resip::ClientPublicationHandle, int, const resip::SipMessage&) { return -1; }
      virtual void onStaleUpdate(resip::ClientPublicationHandle, const resip::SipMessage&);


      // ClientSubscriptionHandler
      //virtual void onRefreshRejected(resip::ClientSubscriptionHandle, const resip::SipMessage& rejection);
      virtual void onUpdatePending(resip::ClientSubscriptionHandle, const resip::SipMessage& notify, bool outOfOrder);
      virtual void onUpdateActive(resip::ClientSubscriptionHandle, const resip::SipMessage& notify, bool outOfOrder);
      virtual void onUpdateExtension(resip::ClientSubscriptionHandle, const resip::SipMessage& notify, bool outOfOurde);
      virtual int onRequestRetry(resip::ClientSubscriptionHandle, int retrySeconds, const resip::SipMessage& notify) { return -1; }
      virtual void onTerminated(resip::ClientSubscriptionHandle,const resip::SipMessage *);
      virtual void onNewSubscription(resip::ClientSubscriptionHandle, const resip::SipMessage& notify);
      //virtual void onReadyToSend(resip::ClientSubscriptionHandle, resip::SipMessage& msg);

      // ServerSubscriptionHandler
      virtual void onNewSubscription(resip::ServerSubscriptionHandle, const resip::SipMessage& sub);
      virtual void onNewSubscriptionFromRefer(resip::ServerSubscriptionHandle, const resip::SipMessage& sub);
      virtual void onTerminated(resip::ServerSubscriptionHandle);
      virtual bool hasDefaultExpires() const { return true; }
      virtual UInt32 getDefaultExpires() const { return 60; }

      // ClientPagerMessageHandler
      virtual void onSuccess(resip::ClientPagerMessageHandle, const resip::SipMessage& status);
      virtual void onFailure(resip::ClientPagerMessageHandle, const resip::SipMessage& status, std::auto_ptr<resip::Contents> contents);

      // ServerPagerMessageHandler
      virtual void onMessageArrived(resip::ServerPagerMessageHandle, const resip::SipMessage& message);

      // ClientOutOfDialogReqHandler
      virtual void onSuccess(resip::ClientOutOfDialogReqHandle, const resip::SipMessage& success);
      virtual void onFailure(resip::ClientOutOfDialogReqHandle, const resip::SipMessage& error);

      // ServerOutOfDialogReqHandler
      virtual void onReceivedRequest(resip::ServerOutOfDialogReqHandle, const resip::SipMessage& request);

      // DialogSetHandler
      virtual void onTrying(resip::AppDialogSetHandle h, const resip::SipMessage& msg);
      virtual void onNonDialogCreatingProvisional(resip::AppDialogSetHandle, const resip::SipMessage&) {}

      // DialogEventHandler
      virtual void onTrying(const resip::TryingDialogEvent &);
      virtual void onProceeding(const resip::ProceedingDialogEvent &);
      virtual void onEarly(const resip::EarlyDialogEvent &);
      virtual void onConfirmed(const resip::ConfirmedDialogEvent &);
      virtual void onTerminated(const resip::TerminatedDialogEvent &);
      virtual void onMultipleEvents(const resip::MultipleEventDialogEvent &);

      //void handleEvent(Event* eventRaw);      

      resip::SharedPtr<resip::MasterProfile> mProfile;
      resip::Security* mSecurity;
      resip::FdPollGrp* mPollGrp;
      resip::EventThreadInterruptor* mInterruptor;
      resip::SipStack* mStack;
      resip::EventStackThread* mStackThread;
      resip::DialogUsageManager* mDum;

      TestProxy* mTestProxy;
      resip::Data mIp;
      int mPort;

      resip::AppDialogSet* mAppDialogSet;

private:
      bool mNatNavigator;
      resip::Data mStunAddr;
      int mStunPort;
      int mLocalPort;
      bool mRportDiscovery;
      bool mStunDiscovery;
      bool mInstallUDPTransport;

      // out of dialog refer
      resip::ServerSubscriptionHandle mServerSubscription;
      resip::SipMessage mReferMessage;

      // special handling required for UAC Prack scenario - must call provideOffer from 
      // onAnswer callback in order to get Offer to be provided in first PRACK
      boost::shared_ptr<resip::SdpContents> mOfferToProvideInNextOnAnswerCallback;

      typedef std::set<TestUsage*> TestUsages;
      TestUsages mTestUsages;

      void dispatchEvent(ClientSubscriptionEvent*);
      void dispatchEvent(ServerSubscriptionEvent*);
      void dispatchEvent(ClientRegistrationEvent*);
      void dispatchEvent(InviteEvent*);
      void dispatchEvent(ClientPublicationEvent*);
      void dispatchEvent(ServerOutOfDialogReqEvent*);
      TestUsage* findUsage(Event*);

      friend class FindMatchingDialogToReplace;

};

DumUserAgent::From* dumFrom(const DumUserAgent& dua);
DumUserAgent::From* dumFrom(const DumUserAgent* dua);
DumUserAgent::From* dumFrom(TestProxy* proxy);

DumUserAgent::FindMatchingDialogToReplace* findMatchingDialogToReplace(DumUserAgent& dua);
DumUserAgent::FindMatchingDialogToReplace* findMatchingDialogToReplace(DumUserAgent* dua);

//DumUserAgent::HasInstanceId* hasInstanceId();
//DumUserAgent::NoInstanceId* noInstanceId();

DumUserAgent::HasRinstance* hasRinstance();
DumUserAgent::NoRinstance* noRinstance();

DumUserAgent::HasMethodsParam* hasMethodsParam();
DumUserAgent::NoMethodsParam* noMethodsParam();

#endif
