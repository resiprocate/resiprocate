#include "DumUserAgent.hxx"
#include "tfm/PortAllocator.hxx"
#include "tfm/SequenceSet.hxx"
#include "tfm/ActionBase.hxx"
#include "tfm/TestProxy.hxx"

#include "DumEvent.hxx"
#include "DumExpect.hxx"
#include "DumUaAction.hxx"

#include "BindHandle.hxx"
#include "ClientSubscriptionEvent.hxx"
#include "ServerOutOfDialogReqEvent.hxx"
#include "ClientOutOfDialogReqEvent.hxx"
#include "TestClientSubscription.hxx"
#include "TestServerSubscription.hxx"
#include "TestServerOutOfDialogReq.hxx"
#include "TestClientPagerMessage.hxx"
#include "TestServerPagerMessage.hxx"
#include "tfm/EndPoint.hxx"
#include "ClientRegistrationEvent.hxx"
#include "InviteSessionEvent.hxx"
#include "ClientPagerMessageEvent.hxx"
#include "ServerPagerMessageEvent.hxx"
#include "DialogEventHandlerEvent.hxx"

#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/AppDialogSetFactory.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/UserProfile.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SdpContents.hxx"

#if defined (USE_SSL)
#if defined(WIN32) 
#include "resip/stack/ssl/WinSecurity.hxx"
#else
#include "resip/stack/ssl/Security.hxx"
#endif
#endif

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "rutil/Random.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;

template<class C, class E> void 
dispatchEventHelper(E* event, C& container)
{   
   StackLog(<<"dispatchEventHelper: " << *event << " looking through: " << Inserter(container));
   for (typename C::iterator it = container.begin(); it != container.end(); ++it)
   {
      if ((*it)->isMyEvent(event))
      {
         (*it)->handleEvent(event);
         return;
      }
   }

   ErrLog(<< "No matching endpoint for event " << typeid(*event).name());
   resip_assert(0);
   throw TestEndPoint::GlobalFailure("No matching endpoint for the event", __FILE__, __LINE__);
}

resip::SharedPtr<resip::MasterProfile>
DumUserAgent::makeProfile(const resip::Uri& aor, const Data& password)
{
   resip::SharedPtr<resip::MasterProfile> profile(new MasterProfile);

   //profile->addAllowedEvent(resip::Token("conference"));
   profile->validateAcceptEnabled() = false;
   profile->validateContentEnabled() = false;
   profile->setDefaultFrom(NameAddr(aor));
   profile->setUserAgent("dum/tfm");
   profile->setDigestCredential(aor.host(), aor.user(), aor.password());   
   profile->setInstanceId(resip::Random::getRandomHex(4));   
   profile->gruuEnabled() = true;
   
   return profile;
}

const resip::NameAddr& 
DumUserAgent::getAor() const
{
   return getProfile()->getDefaultFrom();
}

ExpectAction* DumUserAgent::setOfferToProvideInNextOnAnswerCallback(boost::shared_ptr<resip::SdpContents> offer) 
{ 
   mOfferToProvideInNextOnAnswerCallback = offer; 
   return new NoAction();
}

// DumUaAction* 
// DumUserAgent::start()
// {
//    return new Start(*this);
// }

DumUaAction* 
DumUserAgent::shutdownUa()
{
   return new Shutdown(*this);
}

void 
DumUserAgent::clean()
{
   mTestUsages.clear();
   getDum().createDialogEventStateManager(0);
   //clear out handles/etc here
}


DumUserAgent::DumUserAgent(resip::SharedPtr<resip::MasterProfile> profile) :
   mProfile(profile),
   mPollGrp(FdPollGrp::create()),  // Will create EPoll implementation if available, otherwise FdPoll
   mInterruptor(new EventThreadInterruptor(*mPollGrp)),
#if defined(USE_SSL)
   mSecurity(new Security),
#else
   mSecurity(0),
#endif
   mStack(new SipStack(mSecurity, DnsStub::EmptyNameserverList, mInterruptor)),
   mStackThread(new EventStackThread(*mStack, *mInterruptor, *mPollGrp)),
   mDum(new DialogUsageManager(*mStack, true)),
   mTestProxy(0),
   mAppDialogSet(0),
   mNatNavigator(false),
   mStunAddr(),
   mStunPort(0),
   mLocalPort(0)
{
   registerWithTransportDriver();
}

DumUserAgent::DumUserAgent(resip::SharedPtr<resip::MasterProfile> profile,
                           TestProxy* proxy) : 
   mProfile(profile),
   mPollGrp(FdPollGrp::create()),  // Will create EPoll implementation if available, otherwise FdPoll
   mInterruptor(new EventThreadInterruptor(*mPollGrp)),
#if defined(USE_SSL)
   mSecurity(new Security),
#else
   mSecurity(0),
#endif
   mStack(new SipStack(mSecurity, DnsStub::EmptyNameserverList, mInterruptor)),
   mStackThread(new EventStackThread(*mStack, *mInterruptor, *mPollGrp)),
   mDum(new DialogUsageManager(*mStack, true)),
   mTestProxy(proxy),
   mAppDialogSet(0),
   mNatNavigator(false),
   mStunAddr(),
   mStunPort(0),
   mLocalPort(0)
{
   proxy->addUser(mProfile->getDefaultFrom().uri().user(), 
                  mProfile->getDefaultFrom().uri(), 
                  mProfile->getDigestCredential(mProfile->getDefaultFrom().uri().host()).password);   
   registerWithTransportDriver();
}

DumUserAgent::~DumUserAgent()
{
   if(mTestProxy)
   {
      mTestProxy->deleteUser(mProfile->getDefaultFrom().uri().user(), 
                             mProfile->getDefaultFrom().uri());
   }
   unregisterFromTransportDriver();

   mStack->shutdownAndJoinThreads();
   mStackThread->shutdown();
   mStackThread->join();
   mStack->setCongestionManager(0);

   delete mDum;
   delete mStack;
   delete mStackThread;
   delete mInterruptor;
   delete mPollGrp;
   // Note:  mStack descructor will delete mSecurity
}

void
DumUserAgent::init()
{
   mIp = PortAllocator::getNextLocalIpAddress();
   mPort = PortAllocator::getNextPort();
   if( !mNatNavigator )
   {
      mStack->addTransport(resip::UDP, mPort, resip::V4, StunDisabled, mIp);
   }
   mStack->addTransport(resip::TCP, mPort, resip::V4, StunDisabled, mIp);

   mProfile->setFixedTransportInterface(mIp);

   mProfile->addSupportedMethod(SUBSCRIBE);
   mProfile->addSupportedMethod(NOTIFY);
   mProfile->addSupportedMethod(INFO);
   mProfile->addSupportedMethod(MESSAGE);
   mProfile->addSupportedMethod(REFER);
   mProfile->addSupportedMethod(PRACK);
  
   mDum->setMasterProfile(mProfile);

   mDum->setClientRegistrationHandler(this);

   mDum->setDialogSetHandler(this);
   //mDum->addServerSubscriptionHandler("presence", this);
   //mDum->addClientSubscriptionHandler("presence", this);
   //mDum->addClientSubscriptionHandler("message-summary", this);
   //mDum->addServerSubscriptionHandler("message-summary", this);
   mDum->setInviteSessionHandler(this);

   std::auto_ptr<resip::ClientAuthManager> clam(new resip::ClientAuthManager());
   mDum->setClientAuthManager(clam);
   
   //!dcm! -- use TestAP/dumv2 pattern
//   mDum->addOutOfDialogHandler(REFER, this);

   mStack->run();
   mStackThread->run();
}

void
DumUserAgent::addClientSubscriptionHandler(const Data& eventType, ClientSubscriptionHandler* h)
{
   mDum->addClientSubscriptionHandler(eventType, h);
}

void
DumUserAgent::addServerSubscriptionHandler(const Data& eventType, ServerSubscriptionHandler* h)
{
   mDum->addServerSubscriptionHandler(eventType, h);
}

void
DumUserAgent::addClientPublicationHandler(const Data& eventType, ClientPublicationHandler* h)
{
   mDum->addClientPublicationHandler(eventType, h);
}

void
DumUserAgent::addServerPublicationHandler(const Data& eventType, ServerPublicationHandler* h)
{
   mDum->addServerPublicationHandler(eventType, h);
}

void
DumUserAgent::addClientPagerMessageHandler(ClientPagerMessageHandler* h)
{
   mDum->setClientPagerMessageHandler(h);
}

void
DumUserAgent::addServerPagerMessageHandler(ServerPagerMessageHandler* h)
{
   mDum->setServerPagerMessageHandler(h);
}

void
DumUserAgent::addOutOfDialogHandler(MethodTypes methodType, OutOfDialogHandler* h)
{
   mDum->addOutOfDialogHandler(methodType, h);
}

void
DumUserAgent::addSupportedOptionTag(const Token& tag)
{
   mDum->getMasterProfile()->addSupportedOptionTag(tag);
}

void
DumUserAgent::setOutboundProxy(const Uri& proxy)
{
   mDum->getMasterProfile()->setOutboundProxy(proxy);
}

void
DumUserAgent::unsetOutboundProxy()
{
   mDum->getMasterProfile()->unsetOutboundProxy();
}

void
DumUserAgent::setDefaultFrom(const Uri& from)
{
   mDum->getMasterProfile()->setDefaultFrom(NameAddr(from));
}

ClientPagerMessageHandle
DumUserAgent::makePagerMessage(const resip::NameAddr& target)
{
   return mDum->makePagerMessage(target);
}

// void 
// DumUserAgent::stop()
// {
// //    mStackThread.shutdown();
// //    shutdown();
// }

void 
DumUserAgent::buildFdSet(FdSet& fdset)
{
}

void 
DumUserAgent::process(FdSet&)
{
   try
   {
      while(mDum->process());
   }
   catch (BaseException& e)
   {
      WarningLog (<< "Unhandled exception: " << e);
      
      // should cause the test to fail
      resip_assert(0);
   }
}

const Data& 
DumUserAgent::getInstanceId() const
{
   return mProfile->getInstanceId();
}

resip::Uri 
DumUserAgent::getContact() const
{
   Uri localContact = getProfile()->getDefaultFrom().uri();
   localContact.host() = getIp();
   localContact.port() = getPort();
   return localContact;
}


resip::Data 
DumUserAgent::getName() const
{
   return mProfile->getDefaultFrom().uri().getAor();
}

/*
void
DumUserAgent::handleEvent(Event* eventRaw)
{
   boost::shared_ptr<Event> event(eventRaw);
   DebugLog(<< "DumUserAgent::handeEvent: " << *eventRaw);    
   if (getSequenceSet())
   {
      getSequenceSet()->enqueue(event);
   }
   else
   {
      WarningLog(<< *this << " has no associated SequenceSet: discarding event " << *event);
   }
}
*/

DumUaAction*
DumUserAgent::invite(const NameAddr& target, 
                     const SdpContents* initialOffer,
                     const SdpContents* alternative,
                     DialogUsageManager::EncryptionLevel level)
{
  resip::SharedPtr<resip::SipMessage> (resip::DialogUsageManager::*fn)(const NameAddr&, const Contents*, DialogUsageManager::EncryptionLevel, const Contents*, AppDialogSet*) = &resip::DialogUsageManager::makeInviteSession;
  return new DumUaSendingCommand(this, boost::bind(fn, mDum,
                                                   target, initialOffer, level, alternative, (AppDialogSet*)0));
}

DumUaAction*
DumUserAgent::inviteFromRefer(const resip::SipMessage& refer, 
                              resip::ServerSubscriptionHandle& h, 
                              const resip::SdpContents* initialOffer,
                              resip::DialogUsageManager::EncryptionLevel level,
                              const resip::SdpContents* alternative)
{
  return new DumUaSendingCommand(this, boost::bind(&resip::DialogUsageManager::makeInviteSessionFromRefer, mDum,
                                                   boost::ref(refer), boost::ref(h), initialOffer, level, alternative, (AppDialogSet*)0));
}

DumUaAction*
DumUserAgent::subscribe(const NameAddr& target, const Data& eventType)
{
   return new DumUaSendingCommand(this, boost::bind(&resip::DialogUsageManager::makeSubscription, mDum, 
                                                    target, eventType, (AppDialogSet*)0));
}

DumUaAction*
DumUserAgent::publish(const resip::NameAddr& target, const resip::Contents& body, 
                      const resip::Data& eventType, unsigned expiresSeconds)
{
   return new DumUaSendingCommand(this, boost::bind(&resip::DialogUsageManager::makePublication, mDum,
                                                    target, boost::ref(body), eventType, expiresSeconds, (AppDialogSet*)0));
}

DumUaAction*
DumUserAgent::registerUa(bool tcp)
{
   NameAddr& target = mDum->getMasterUserProfile()->getDefaultFrom();
   if (tcp)
   {
      target.uri().param(resip::p_transport) = "tcp";
   }
   
   return new DumUaSendingCommand(this, boost::bind(&resip::DialogUsageManager::makeRegistration, mDum, 
                                                    target, (AppDialogSet*)0));   
}

DumUaAction*
DumUserAgent::send(resip::SharedPtr<resip::SipMessage> msg)
{
   return new DumUaCommand(this, boost::bind(&resip::DialogUsageManager::send, mDum, msg));
}

DumUaAction*
DumUserAgent::cancelInvite()
{
   resip_assert(mAppDialogSet);
   return new DumUaCommand(this, boost::bind(&resip::AppDialogSet::end, mAppDialogSet));
}

DumUaAction*
DumUserAgent::refer(const resip::NameAddr& target, const resip::NameAddr& referTo)
{
   return new DumUaSendingCommand(this, boost::bind(&resip::DialogUsageManager::makeRefer, mDum, target, referTo, (AppDialogSet*)0));
}

DumUaAction*
DumUserAgent::referNoReferSub(const resip::NameAddr& target, const resip::NameAddr& referTo)
{
   ReferAdornment* adorner = new ReferAdornment(referTo);
   return new DumUaSendingCommand(this, boost::bind(&resip::DialogUsageManager::makeOutOfDialogRequest, mDum, target, REFER, (AppDialogSet*)0), adorner);
}

DumUaAction*
DumUserAgent::referNoReferSubWithoutReferSubHeader(const resip::NameAddr& target, const resip::NameAddr& referTo)
{
   ReferAdornmentRemoveReferSubHeader* adorner = new ReferAdornmentRemoveReferSubHeader(referTo);
   return new DumUaSendingCommand(this, boost::bind(&resip::DialogUsageManager::makeOutOfDialogRequest, mDum, target, REFER, (AppDialogSet*)0), adorner);
}

// ClientRegistrationHandler
void 
DumUserAgent::onSuccess(resip::ClientRegistrationHandle h, const resip::SipMessage& response)
{
   DebugLog(<< "ClientRegistrationHandler::onSuccess");
   handleEvent(new ClientRegistrationEvent(this, Register_Success, h, response));
}

void
DumUserAgent::onRemoved(resip::ClientRegistrationHandle h, const resip::SipMessage& response)
{
   DebugLog(<< "ClientRegistrationHandler::onRemoved");
   handleEvent(new ClientRegistrationEvent(this, Register_Removed, h, response));
}

void 
DumUserAgent::onFailure(resip::ClientRegistrationHandle h, const resip::SipMessage& response)
{
   DebugLog(<< "ClientRegistrationHandler::onFailure");
   handleEvent(new ClientRegistrationEvent(this, Register_Failure, h, response));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientRegistrationEvent::Type t, 
                      MessageMatcher* matcher, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientRegistrationEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction);
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientRegistrationEvent::Type t, 
                     TestClientRegistration& clientReg,
                     MessageMatcher* matcher, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientRegistrationEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ClientRegistrationEvent>(this, "BindClientRegistrationHandle", clientReg.getHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientRegistrationEvent::Type t, 
                     TestClientRegistration& clientReg,
                     MessageMatcher* matcher, 
                     ExpectPreCon& pred,
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientRegistrationEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ClientRegistrationEvent>(this, "BindClientRegistrationHandle", clientReg.getHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientRegistrationEvent::Type t, 
                     TestClientRegistration& clientReg,
                     ExpectPreCon& pred, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientRegistrationEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ClientRegistrationEvent>(this, "BindClientRegistrationHandle", clientReg.getHandleRef()));
}

// InviteSessionHandler
void
DumUserAgent::onNewSession(ClientInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   DebugLog(<< "onNewSession - client");   
   handleEvent(new ClientInviteEvent(this, Invite_NewClientSession, h, msg));
}

void
DumUserAgent::onNewSession(ServerInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   DebugLog(<< "onNewSession - server");
   handleEvent(new ServerInviteEvent(this, Invite_NewServerSession, h, msg));
}

void
DumUserAgent::onFailure(ClientInviteSessionHandle h, const SipMessage& msg)
{
   DebugLog(<< "onFailure");
   handleEvent(new ClientInviteEvent(this, Invite_Failure, h, msg));
}

void
DumUserAgent::onEarlyMedia(ClientInviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   DebugLog(<< "onEarlyMedia");
   handleEvent(new ClientInviteEvent(this, Invite_EarlyMedia, h, msg));
}
 
void
DumUserAgent::onProvisional(ClientInviteSessionHandle h, const SipMessage& msg)
{
   DebugLog(<< "onProvisional");
   handleEvent(new ClientInviteEvent(this, Invite_Provisional, h, msg));
}

void
DumUserAgent::onStaleCallTimeout(ClientInviteSessionHandle h)
{
   DebugLog(<< "onStaleCallTimeout");
   handleEvent(new ClientInviteEvent(this, Invite_StaleCallTimeout, h));
}

void
DumUserAgent::onConnected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   DebugLog(<< "onConnected - client");
   handleEvent(new ClientInviteEvent(this, Invite_Connected, h, msg));
}

void
DumUserAgent::onConnected(InviteSessionHandle h, const SipMessage& msg)
{
   DebugLog(<< "onConnected");
   handleEvent(new InviteEvent(this, Invite_Connected, h, msg));
}

void 
DumUserAgent::onTerminated(InviteSessionHandle h, InviteSessionHandler::TerminatedReason reason, const SipMessage* related)
{
   DebugLog(<< "onTerminated");
//   dispatchEvent(new InviteEvent(this, Invite_Terminated, h));
   handleEvent(new InviteEvent(this, Invite_Terminated, h));
}

void
DumUserAgent::onForkDestroyed(ClientInviteSessionHandle h)
{
   DebugLog(<< "onForkDestroyed");
   handleEvent(new ClientInviteEvent(this, Invite_ForkDestroyed, h));
}

void
DumUserAgent::onRedirected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   DebugLog(<< "onRedirected");
   handleEvent(new ClientInviteEvent(this, Invite_Redirected, h, msg));
}

void
DumUserAgent::onAnswer(InviteSessionHandle h, const SipMessage& msg, const SdpContents&)
{
   DebugLog(<< "onAnswer");
   handleEvent(new InviteEvent(this, Invite_Answer, h, msg));
   if(mOfferToProvideInNextOnAnswerCallback.get())
   {
      DebugLog(<< "onAnswer - calling provideOffer from onAnswer callback");
      h->provideOffer(*mOfferToProvideInNextOnAnswerCallback);
      mOfferToProvideInNextOnAnswerCallback.reset();
   }
}

void 
DumUserAgent::onOffer(InviteSessionHandle h, const SipMessage& msg, const SdpContents&)
{
   DebugLog(<< "onOffer");
   handleEvent(new InviteEvent(this, Invite_Offer, h, msg));
}

void
DumUserAgent::onRemoteSdpChanged(InviteSessionHandle h, const SipMessage& msg, const SdpContents&)
{
   DebugLog(<< "onRemoteSdpChanged");
   handleEvent(new InviteEvent(this, Invite_RemoteSdpChanged, h, msg));
}

void
DumUserAgent::onOfferRequestRejected(InviteSessionHandle h, const SipMessage& msg)
{
   DebugLog(<< "onOfferRequestRejected");
   handleEvent(new InviteEvent(this, Invite_OfferRequestRejected, h, msg));
}

void
DumUserAgent::onOfferRequired(InviteSessionHandle h, const SipMessage& msg)
{
   DebugLog(<< "onOfferRequired");
   handleEvent(new InviteEvent(this, Invite_OfferRequired, h, msg));
}

void
DumUserAgent::onOfferRejected(InviteSessionHandle h, const SipMessage* msg)
{
   DebugLog(<< "onOfferRejected");
   if( msg )
   {
      handleEvent(new InviteEvent(this, Invite_OfferRejected, h, *msg));
   }
   else
   {
      handleEvent(new InviteEvent(this, Invite_OfferRejected, h));
   }
}

void
DumUserAgent::onInfo(InviteSessionHandle h, const SipMessage& msg)
{
   DebugLog(<< "onInfo");
   handleEvent(new InviteEvent(this, Invite_Info, h, msg));
}

void
DumUserAgent::onInfoSuccess(InviteSessionHandle h, const SipMessage& msg)
{
   DebugLog(<< "onInfoSuccess");
   handleEvent(new InviteEvent(this, Invite_InfoSuccess, h, msg));
}

void
DumUserAgent::onInfoFailure(InviteSessionHandle h, const SipMessage& msg)
{
   DebugLog(<< "onInfoFailure");
   handleEvent(new InviteEvent(this, Invite_InfoFailure, h, msg));
}
 
void
DumUserAgent::onMessage(InviteSessionHandle h, const SipMessage& msg)
{
   DebugLog(<< "onMessage");
   handleEvent(new InviteEvent(this, Invite_Message, h, msg));
}

void
DumUserAgent::onMessageSuccess(InviteSessionHandle h, const SipMessage& msg)
{
   DebugLog(<< "onMessageSuccess");
   handleEvent(new InviteEvent(this, Invite_MessageSuccess, h, msg));
}
 
void
DumUserAgent::onMessageFailure(InviteSessionHandle h, const SipMessage& msg)
{
   DebugLog(<< "onMessageFailure");
   handleEvent(new InviteEvent(this, Invite_MessageFailure, h, msg));
}

void
DumUserAgent::onRefer(InviteSessionHandle h, ServerSubscriptionHandle serverSubHandle, const SipMessage& msg)
{
   DebugLog(<< "onRefer");
   InviteEvent* event = new InviteEvent(this, Invite_Refer, h, msg, serverSubHandle);
   TestUsage* usage = findUsage(event);
   resip_assert(usage);
   static_cast<TestInviteSession*>(usage)->setServerSubscription(serverSubHandle);
   static_cast<TestInviteSession*>(usage)->setReferMessage(msg);
   handleEvent(event);
}

void 
DumUserAgent::onReferNoSub(InviteSessionHandle h, const SipMessage& msg)
{
   DebugLog(<< "onReferNoSub");
   InviteEvent event(this, Invite_Refer, h, msg);
   TestUsage* usage = findUsage(&event);
   resip_assert(usage);
   static_cast<TestInviteSession*>(usage)->setReferMessage(msg);
   handleEvent(new InviteEvent(this, Invite_ReferNoSub, h, msg));
}

void
DumUserAgent::onReferRejected(InviteSessionHandle h, const SipMessage& msg)
{
   DebugLog(<< "onReferRejected");
   handleEvent(new InviteEvent(this, Invite_ReferRejected, h, msg));
}
 
void
DumUserAgent::onReferAccepted(InviteSessionHandle h, ClientSubscriptionHandle clientSubHandle, const SipMessage& msg)
{
   DebugLog(<< "onReferAccepted");
   handleEvent(new InviteEvent(this, Invite_ReferAccepted, h, msg, clientSubHandle));
}

// void
// DumUserAgent::onAckNotReceived(InviteSessionHandle h)
// {
//    handleEvent(new InviteEvent(this, Invite_AckNotReceived, h));
// }

void
DumUserAgent::onIllegalNegotiation(InviteSessionHandle h, const SipMessage& msg)
{
   DebugLog(<< "onIllegalNegotiation");
   handleEvent(new InviteEvent(this, Invite_IllegalNegotiation, h, msg));
}
 
void
DumUserAgent::onSessionExpired(InviteSessionHandle h)
{
   DebugLog(<< "onSessionExpired");
   handleEvent(new InviteEvent(this, Invite_SessionExpired, h));
}

void 
DumUserAgent::onPrack(ServerInviteSessionHandle h, const SipMessage &msg)
{
   DebugLog(<< "onPrack");
   handleEvent(new ServerInviteEvent(this, Invite_Prack, h, msg));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(InviteEvent::Type t, 
                      MessageMatcher* matcher, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<InviteEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction);
 }

DumUserAgent::ExpectBase* 
DumUserAgent::expect(InviteEvent::Type t, 
                     TestClientInviteSession& clientInv,
                     MessageMatcher* matcher, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<InviteEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction,
                        new BindHandleInvite<ClientInviteEvent>(this, "BindClientInviteSessionHandle", clientInv.getHandleRef(), clientInv.getSessionHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(InviteEvent::Type t, 
                     TestClientInviteSession& clientInv,
                     MessageMatcher* matcher, 
                     ExpectPreCon& pred,
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<InviteEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandleInvite<ClientInviteEvent>(this, "BindClientInviteSessionHandle", clientInv.getHandleRef(), clientInv.getSessionHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(InviteEvent::Type t, 
                     TestClientInviteSession& clientInv,
                     ExpectPreCon& pred, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<InviteEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandleInvite<ClientInviteEvent>(this, "BindClientInviteSessionHandle", clientInv.getHandleRef(), clientInv.getSessionHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(InviteEvent::Type t, 
                     ExpectPreCon& pred, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<InviteEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction);
}

DumUserAgent::ExpectBase*
DumUserAgent::expect(InviteEvent::Type t, 
                     TestServerInviteSession& serverInv,
                     MessageMatcher* matcher, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<InviteEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction,
                        new BindHandleInvite<ServerInviteEvent>(this, "BindServerInviteSessionHandle", serverInv.getHandleRef(), serverInv.getSessionHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(InviteEvent::Type t, 
                     TestServerInviteSession& serverInv,
                     MessageMatcher* matcher, 
                     ExpectPreCon& pred,
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<InviteEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandleInvite<ServerInviteEvent>(this, "BindServerInviteSessionHandle", serverInv.getHandleRef(), serverInv.getSessionHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(InviteEvent::Type t, 
                     TestServerInviteSession& serverInv,
                     ExpectPreCon& pred, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<InviteEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandleInvite<ServerInviteEvent>(this, "BindServerInviteSessionHandle", serverInv.getHandleRef(), serverInv.getSessionHandleRef()));
}

/*
//actions from ClientPagerMessage
DumUaAction* 
DumUserAgent::getMessageRequest()
{
   return new ClientPagerMessageAction(this, boost::bind(&resip::ClientPagerMessage::getMessageRequest, _1));
}
 
DumUaAction* 
DumUserAgent::page(std::auto_ptr<resip::Contents> contents, resip::DialogUsageManager::EncryptionLevel level)
{
   // contents needs to outlive the boost::bind(...) to assure this to work.
   return new ClientPagerMessageAction(this, boost::bind(&resip::ClientPagerMessage::page, _1, boost::ref(contents), level));
}

DumUaAction* 
DumUserAgent::endClientPagerMsg()
{
   return new ClientPagerMessageAction(this, boost::bind(&resip::ClientPagerMessage::end, _1));
}
*/

/*
//actions from ServerPagerMessage
DumUaAction* 
DumUserAgent::acceptServerPagerMsg(int statusCode)
{
   return new ServerPagerMessageAction(this, boost::bind(&resip::ServerPagerMessage::accept, _1, statusCode), NoAdornment::instance());
}

DumUaAction* 
DumUserAgent::rejectServerPagerMsg(int statusCode)
{
   return new ServerPagerMessageAction(this, boost::bind(&resip::ServerPagerMessage::reject, _1, statusCode), NoAdornment::instance());
}

DumUaAction* 
DumUserAgent::endServerPagerMsg()
{
   return new ServerPagerMessageAction(this, boost::bind(&resip::ServerPagerMessage::end, _1));
}
 
DumUaAction* 
DumUserAgent::sendServerPagerMsg(resip::SharedPtr<resip::SipMessage> msg)
{
   return new ServerPagerMessageAction(this, boost::bind(&resip::ServerPagerMessage::send, _1, msg));
}
*/

// ClientPublicationHandler
void 
DumUserAgent::onSuccess(resip::ClientPublicationHandle h, const resip::SipMessage& msg)
{
   DebugLog(<< "onSuccess - ClientPublication");
   handleEvent(new ClientPublicationEvent(this, ClientPublication_Success, h, msg));
}

void 
DumUserAgent::onRemove(resip::ClientPublicationHandle h, const resip::SipMessage& msg)
{
   DebugLog(<< "onRemove - ClientPublication");
   handleEvent(new ClientPublicationEvent(this, ClientPublication_Remove, h, msg));
}

void 
DumUserAgent::onFailure(resip::ClientPublicationHandle h, const resip::SipMessage& msg)
{
   DebugLog(<< "onFailure - ClientPublication");
   handleEvent(new ClientPublicationEvent(this, ClientPublication_Failure, h, msg));
}

void
DumUserAgent::onStaleUpdate(resip::ClientPublicationHandle h, const resip::SipMessage& msg)
{
   DebugLog(<< "onStaleUpdate - ClientPublication");
   handleEvent(new ClientPublicationEvent(this, ClientPublication_StaleUpdate, h, msg));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientPublicationEvent::Type t, 
                     TestClientPublication& clientPub,
                     MessageMatcher* matcher, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientPublicationEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ClientPublicationEvent>(this, "BindClientPublicationHandle", clientPub.getHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientPublicationEvent::Type t, 
                     TestClientPublication& clientPub,
                     MessageMatcher* matcher, 
                     ExpectPreCon& pred,
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientPublicationEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ClientPublicationEvent>(this, "BindClientPublicationHandle", clientPub.getHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientPublicationEvent::Type t, 
                     TestClientPublication& clientPub,
                     ExpectPreCon& pred, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientPublicationEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ClientPublicationEvent>(this, "BindClientPublicationHandle", clientPub.getHandleRef()));
}

// ClientSubscriptionHandler
void 
DumUserAgent::onUpdatePending(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify, bool outOfOrder)
{
   DebugLog(<< "onUpdatePending");
   handleEvent(new ClientSubscriptionEvent(this, ClientSubscription_UpdatePending, h, &notify, outOfOrder));
}

void 
DumUserAgent::onUpdateActive(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify, bool outOfOrder)
{
   DebugLog(<< "onUpdateActive");
   handleEvent(new ClientSubscriptionEvent(this, ClientSubscription_UpdateActive, h, &notify, outOfOrder));
}

void 
DumUserAgent::onUpdateExtension(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify, bool outOfOrder)
{
   DebugLog(<< "onUpdateExtension");
   handleEvent(new ClientSubscriptionEvent(this, ClientSubscription_UpdateExtension, h, &notify, outOfOrder));
}

void 
DumUserAgent::onTerminated(resip::ClientSubscriptionHandle h, const resip::SipMessage* msg)
{
   DebugLog(<< "ClientSubscriptionHandler::onTerminated");
   handleEvent(new ClientSubscriptionEvent(this, ClientSubscription_Terminated, h, msg));
}

void 
DumUserAgent::onNewSubscription(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify)
{
   DebugLog(<< "ClientSubscriptionHandler:onNewSubscription");
   handleEvent(new ClientSubscriptionEvent(this, ClientSubscription_NewSubscription, h, &notify));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientSubscriptionEvent::Type t, 
                     ExpectPreCon& pred,
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientSubscriptionEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction);
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientSubscriptionEvent::Type t, 
                     TestClientSubscription& clientSub,
                     MessageMatcher* matcher, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientSubscriptionEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ClientSubscriptionEvent>(this, "BindClientSubscriptionHandle", clientSub.getHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientSubscriptionEvent::Type t, 
                     TestClientSubscription& clientSub,
                     MessageMatcher* matcher, 
                     ExpectPreCon& pred,
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientSubscriptionEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ClientSubscriptionEvent>(this, "BindClientSubscriptionHandle", clientSub.getHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientSubscriptionEvent::Type t, 
                     TestClientSubscription& clientSub,
                     ExpectPreCon& pred, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientSubscriptionEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ClientSubscriptionEvent>(this, "BindClientSubscriptionHandle", clientSub.getHandleRef()));
}

// ServerSubscriptionHandler
void 
DumUserAgent::onNewSubscription(ServerSubscriptionHandle h, const SipMessage& sub)
{
   DebugLog(<< "ServerSubscriptionHandler::onNewSubscription");
   handleEvent(new ServerSubscriptionEvent(this, ServerSubscription_NewSubscription, h, sub));
}

void 
DumUserAgent::onNewSubscriptionFromRefer(ServerSubscriptionHandle h, const SipMessage& sub)
{
   DebugLog(<< "ServerSubscriptionHandler::onNewSubscriptionFromRefer");
   mReferMessage = sub;
   mServerSubscription = h;
   handleEvent(new ServerSubscriptionEvent(this, ServerSubscription_NewSubscriptionFromRefer, h, sub));
}

void
DumUserAgent::onTerminated(ServerSubscriptionHandle h)
{
   DebugLog(<< "ServerSubscriptionHandler::onTerminated");
   handleEvent(new ServerSubscriptionEvent(this, ServerSubscription_Terminated, h));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ServerSubscriptionEvent::Type t, 
                     TestServerSubscription& serverSub,
                     MessageMatcher* matcher, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerSubscriptionEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ServerSubscriptionEvent>(this, "BindServerSubscriptionHandle", serverSub.getHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ServerSubscriptionEvent::Type t, 
                     TestServerSubscription& serverSub,
                     MessageMatcher* matcher, 
                     ExpectPreCon& pred,
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerSubscriptionEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ServerSubscriptionEvent>(this, "BindServerSubscriptionHandle", serverSub.getHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ServerSubscriptionEvent::Type t, 
                     TestServerSubscription& serverSub,
                     ExpectPreCon& pred, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerSubscriptionEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ServerSubscriptionEvent>(this, "BindServerSubscriptionHandle", serverSub.getHandleRef()));
}

// ClientPagerMessageHandler
void 
DumUserAgent::onSuccess(resip::ClientPagerMessageHandle h, const resip::SipMessage& status)
{
   DebugLog(<< "ClientPagerMessageHandler-onSuccess");
   handleEvent(new ClientPagerMessageEvent(this, ClientPagerMessage_Success, h, status));
}

void
DumUserAgent::onFailure(resip::ClientPagerMessageHandle h, const resip::SipMessage& status, std::auto_ptr<resip::Contents> contents)
{
   DebugLog(<< "ClientPagerMessageHandler-onFailure");
   handleEvent(new ClientPagerMessageEvent(this, ClientPagerMessage_Failure, h, status, contents));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientPagerMessageEvent::Type t, 
                     TestClientPagerMessage& clientPager,
                     MessageMatcher* matcher, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientPagerMessageEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ClientPagerMessageEvent>(this, "BindClientPagerMessageHandle", clientPager.getHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientPagerMessageEvent::Type t, 
                     TestClientPagerMessage& clientPager,
                     MessageMatcher* matcher, 
                     ExpectPreCon& pred,
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientPagerMessageEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ClientPagerMessageEvent>(this, "BindClientPagerMessageHandle", clientPager.getHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientPagerMessageEvent::Type t, 
                     TestClientPagerMessage& clientPager,
                     ExpectPreCon& pred, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientPagerMessageEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ClientPagerMessageEvent>(this, "BindClientPagerMessageHandle", clientPager.getHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientPagerMessageEvent::Type t, 
                      ExpectPreCon& pred, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientPagerMessageEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction);
}

// ServerPagerMessageHandler
void 
DumUserAgent::onMessageArrived(resip::ServerPagerMessageHandle h, const resip::SipMessage& message)
{
   DebugLog(<< "onMessageArrived");
   handleEvent(new ServerPagerMessageEvent(this, ServerPagerMessage_MessageArrived, h, message));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ServerPagerMessageEvent::Type t, 
                     TestServerPagerMessage& svrPager,
                     MessageMatcher* matcher, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerPagerMessageEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ServerPagerMessageEvent>(this, "BindServerPagerMessageHandle", svrPager.getHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ServerPagerMessageEvent::Type t, 
                     TestServerPagerMessage& svrPager,
                     MessageMatcher* matcher, 
                     ExpectPreCon& pred,
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerPagerMessageEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ServerPagerMessageEvent>(this, "BindServerPagerMessageHandle", svrPager.getHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ServerPagerMessageEvent::Type t, 
                     TestServerPagerMessage& svrPager,
                     ExpectPreCon& pred, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerPagerMessageEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ServerPagerMessageEvent>(this, "BindServerPagerMessageHandle", svrPager.getHandleRef()));
}

/*
// actions from ClientOutOfDialogReq
DumUaAction* 
DumUserAgent::endClientOutOfDialogReq()
{
   return new ClientOutOfDialogReqAction(this, boost::bind(&resip::ClientOutOfDialogReq::end, _1));
}
*/

// ClientOutOfDialogReqHandler
void 
DumUserAgent::onSuccess(resip::ClientOutOfDialogReqHandle h, const resip::SipMessage& success)
{
   handleEvent(new ClientOutOfDialogReqEvent(this, ClientOutOfDialogReq_Success, h, success));
}

void 
DumUserAgent::onFailure(resip::ClientOutOfDialogReqHandle h, const resip::SipMessage& error)
{
   handleEvent(new ClientOutOfDialogReqEvent(this, ClientOutOfDialogReq_Failure, h, error));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientOutOfDialogReqEvent::Type t, 
                     MessageMatcher* matcher, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientOutOfDialogReqEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction);
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientOutOfDialogReqEvent::Type t, 
                     MessageMatcher* matcher, 
                     ExpectPreCon& pred,
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientOutOfDialogReqEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction);
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ClientOutOfDialogReqEvent::Type t, 
                     ExpectPreCon& pred, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientOutOfDialogReqEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction);
}

/*
//actions from ServerOutOfDialogReq
DumUaAction* 
DumUserAgent::acceptServerOutOfDialogReq(int statusCode)
{
   return new ServerOutOfDialogReqAction(this, boost::bind(&resip::ServerOutOfDialogReq::accept, _1, statusCode), NoAdornment::instance());
}

DumUaAction* 
DumUserAgent::rejectServerOutOfDialogReq(int statusCode)
{
   return new ServerOutOfDialogReqAction(this, boost::bind(&resip::ServerOutOfDialogReq::reject, _1, statusCode), NoAdornment::instance());
}

DumUaAction* 
DumUserAgent::endServerOutOfDialogReq()
{
   return new ServerOutOfDialogReqAction(this, boost::bind(&resip::ServerOutOfDialogReq::end, _1));
}

DumUaAction*
DumUserAgent::answerOptions()
{
   return new ServerOutOfDialogReqAction(this, boost::bind(&resip::ServerOutOfDialogReq::answerOptions, _1), NoAdornment::instance());
}

DumUaAction*
DumUserAgent::sendServerOutOfDialogReq(SharedPtr<resip::SipMessage> msg)
{
   return new ServerOutOfDialogReqAction(this, boost::bind(&resip::ServerOutOfDialogReq::send, _1, msg));
}
*/

// ServerOutOfDialogReqHandler
void 
DumUserAgent::onReceivedRequest(resip::ServerOutOfDialogReqHandle h, const resip::SipMessage& request)
{
   handleEvent(new ServerOutOfDialogReqEvent(this, ServerOutOfDialogReq_ReceivedRequest, h, request));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ServerOutOfDialogReqEvent::Type t, 
                     TestServerOutOfDialogReq& serverReq,
                     MessageMatcher* matcher, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerOutOfDialogReqEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ServerOutOfDialogReqEvent>(this, "BindServerOutOfDialogReqHandle", serverReq.getHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ServerOutOfDialogReqEvent::Type t, 
                     TestServerOutOfDialogReq& serverReq,
                     MessageMatcher* matcher, 
                     ExpectPreCon& pred,
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerOutOfDialogReqEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ServerOutOfDialogReqEvent>(this, "BindServerOutOfDialogReqHandle", serverReq.getHandleRef()));
}

DumUserAgent::ExpectBase* 
DumUserAgent::expect(ServerOutOfDialogReqEvent::Type t, 
                     TestServerOutOfDialogReq& serverReq,
                     ExpectPreCon& pred, 
                     int timeoutMs, 
                     ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerOutOfDialogReqEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction,
                        new BindHandle<ServerOutOfDialogReqEvent>(this, "BindServerOutOfDialogReqHandle", serverReq.getHandleRef()));
}

// DialogSetHandler
void
DumUserAgent::onTrying(AppDialogSetHandle h, const SipMessage& msg)
{
   DebugLog(<< "onTrying called" );
   mAppDialogSet = h.get();
}


// DialogEventHandler
void
DumUserAgent::onTrying(const resip::TryingDialogEvent& evt)
{
   handleEvent(new DialogEventHandlerEvent(this, DialogEvent_Trying, evt.getEventInfo()));
}

void 
DumUserAgent::onProceeding(const resip::ProceedingDialogEvent& evt)
{
   handleEvent(new DialogEventHandlerEvent(this, DialogEvent_Proceeding, evt.getEventInfo()));
}

void 
DumUserAgent::onEarly(const resip::EarlyDialogEvent& evt)
{
   handleEvent(new DialogEventHandlerEvent(this, DialogEvent_Early, evt.getEventInfo()));
}

void 
DumUserAgent::onConfirmed(const resip::ConfirmedDialogEvent& evt)
{
   handleEvent(new DialogEventHandlerEvent(this, DialogEvent_Confirmed, evt.getEventInfo()));
}

void 
DumUserAgent::onTerminated(const resip::TerminatedDialogEvent& evt)
{
   handleEvent(new DialogEventHandlerEvent(this, DialogEvent_Terminated, evt.getEventInfo(), evt.getTerminatedReason(), evt.getResponseCode()));
}

void 
DumUserAgent::onMultipleEvents(const resip::MultipleEventDialogEvent& evt)
{
   const resip::MultipleEventDialogEvent::EventVector& v = evt.getEvents();
   resip::MultipleEventDialogEvent::EventVector::const_iterator it = v.begin();
   for (; it != v.end(); ++it)
   {
      if (resip::TerminatedDialogEvent* termEvt = dynamic_cast<resip::TerminatedDialogEvent*>(it->get()))
      {
         onTerminated(*termEvt);
      }
   }
}

DumUserAgent::From::From(const DumUserAgent& dua)
   : mUa(&dua),
     mProxy(0)
{
   resip_assert(mUa);
}

DumUserAgent::From::From(TestProxy* proxy)
   : mUa(0),
     mProxy(proxy)
{
   resip_assert(proxy);
}

Data
DumUserAgent::From::toString() const
{
   resip_assert(mUa || mProxy);

   if (mUa)
   {
      return "from(" + mUa->getName() + ")";
   }
   else
   {
      return "from(" + mProxy->toString() + ")";
   }
}

bool
DumUserAgent::From::isMatch(const boost::shared_ptr<resip::SipMessage>& message) const
{
   if (mUa)
   {
      StackLog(<< "matching stored agent...");
      Uri localContact(mUa->getContact());

      if (message->exists(h_Contacts) && message->header(h_Contacts).size() == 1)
      {
         if (localContact.getAor() == message->header(h_Contacts).front().uri().getAor())
         {
            DebugLog(<< "matched");
            return true;
         }
      }

      if (message->isRequest())
      {
         Via& via = message->header(h_Vias).back(); // via of originator
         StackLog(<< "Trying to match: " << via.sentHost() << ":" << via.sentPort() << " against: " << localContact);
         
         if ((via.sentHost() != localContact.host()
              && !(via.exists(p_received) && via.param(p_received) == localContact.host()))
             || via.sentPort() != localContact.port())
         {
            InfoLog(<< "From::isMatch failed for (UA) " 
                    << mUa->getName() 
                    << ". Via did not match in command");
            return false;
         }
         StackLog(<< "matched");
         return true;
      }
      else
      {
         StackLog(<< "matched");
         return true;
      }

   }
   else if (mProxy)
   {
      StackLog(<< "using proxy->isFromMe");
      //return mProxy->isFromMe(*message);
      return true;
   }

   resip_assert(0);
   return false;
}

DumUserAgent::FindMatchingDialogToReplace::FindMatchingDialogToReplace(DumUserAgent& dua)
   : mUa(&dua)
{
   resip_assert(mUa);
}

Data
DumUserAgent::FindMatchingDialogToReplace::toString() const
{
   return "FindMatchingDialogToReplace";
}

bool
DumUserAgent::FindMatchingDialogToReplace::isMatch(const boost::shared_ptr<resip::SipMessage>& message) const
{
   if (!message->exists(h_Replaces))
   {
      DebugLog(<< "replaces head missing");
      return false;
   }
   else
   {
      return true;
// !bwc! This code is not safe. Maybe we could fire a DumCommand that did the 
// check, and asserted if not the case? If there is no match, won't the test 
// fail anyhow?
//      std::pair<InviteSessionHandle, int> inv = mUa->getDum().findInviteSession(message->header(h_Replaces));
//      if (inv.first.isValid())
//      {
//         DebugLog(<< "matching dialog to replace found");
//         return true;
//      }
//      else
//      {
//         DebugLog(<< "matching dialog to replace not found");
//         return false;
//      }
   }
}


DumUserAgent::From*
dumFrom(const DumUserAgent& dua)
{
   return new DumUserAgent::From(dua);
}

DumUserAgent::From*
dumFrom(const DumUserAgent* dua)
{
   return new DumUserAgent::From(*dua);
}

DumUserAgent::From*
dumFrom(TestProxy* proxy)
{
   return new DumUserAgent::From(proxy);
}

DumUserAgent::FindMatchingDialogToReplace*
findMatchingDialogToReplace(DumUserAgent& dua)
{
   return new DumUserAgent::FindMatchingDialogToReplace(dua);
}

DumUserAgent::FindMatchingDialogToReplace*
findMatchingDialogToReplace(DumUserAgent* dua)
{
   return new DumUserAgent::FindMatchingDialogToReplace(*dua);
}

bool 
DumUserAgent::HasRinstance::isMatch(const boost::shared_ptr<resip::SipMessage>& message) const
{
   // assume there is only one contact.
   return message->header(h_Contacts).begin()->uri().exists(p_rinstance);
}

DumUserAgent::HasRinstance*
hasRinstance()
{
   return new DumUserAgent::HasRinstance();
}

Data
DumUserAgent::HasRinstance::toString() const
{
   return "HasRinstance";
}

bool
DumUserAgent::NoRinstance::isMatch(const boost::shared_ptr<resip::SipMessage>& message) const
{
   return !message->header(h_Contacts).begin()->uri().exists(p_rinstance);
}

Data
DumUserAgent::NoRinstance::toString() const
{
   return "NoRinstance";
}

DumUserAgent::NoRinstance*
noRinstance()
{
   return new DumUserAgent::NoRinstance();
}

bool 
DumUserAgent::HasMethodsParam::isMatch(const boost::shared_ptr<resip::SipMessage>& message) const
{
   // assume there is only one contact.
   return message->header(h_Contacts).begin()->exists(p_methods);
}

DumUserAgent::HasMethodsParam*
hasMethodsParam()
{
   return new DumUserAgent::HasMethodsParam();
}

Data
DumUserAgent::HasMethodsParam::toString() const
{
   return "HasMethodsParam";
}

bool
DumUserAgent::NoMethodsParam::isMatch(const boost::shared_ptr<resip::SipMessage>& message) const
{
   return !message->header(h_Contacts).begin()->exists(p_methods);
}

Data
DumUserAgent::NoMethodsParam::toString() const
{
   return "NoMethodsParam";
}

DumUserAgent::NoMethodsParam*
noMethodsParam()
{
   return new DumUserAgent::NoMethodsParam();
}

/*
bool 
DumUserAgent::HasInstanceId::isMatch(const boost::shared_ptr<resip::SipMessage>& message) const
{
   // assume there is only one contact.
   return message->header(h_Contacts).begin()->exists(p_Instance);
}

DumUserAgent::HasInstanceId*
hasInstanceId()
{
   return new DumUserAgent::HasInstanceId();
}

Data
DumUserAgent::HasInstanceId::toString() const
{
   return "HasInstanceId";
}

bool
DumUserAgent::NoInstanceId::isMatch(const boost::shared_ptr<resip::SipMessage>& message) const
{
   return !message->header(h_Contacts).begin()->exists(p_Instance);
}

Data
DumUserAgent::NoInstanceId::toString() const
{
   return "NoInstanceId";
}

DumUserAgent::NoInstanceId*
noInstanceId()
{
   return new DumUserAgent::NoInstanceId();
}
*/

void
DumUserAgent::dispatchEvent(ClientSubscriptionEvent* event)
{
   dispatchEventHelper(event, mTestUsages);
}

void
DumUserAgent::dispatchEvent(ServerSubscriptionEvent* event)
{
   dispatchEventHelper(event, mTestUsages);
}

void
DumUserAgent::dispatchEvent(ClientRegistrationEvent* event)
{
   StackLog(<< "dispatch ClientRegistrationEvent");
   dispatchEventHelper(event, mTestUsages);
}

void
DumUserAgent::dispatchEvent(InviteEvent* event)
{
   StackLog(<< "dispatch InviteEvent");
   dispatchEventHelper(event, mTestUsages);
}

void
DumUserAgent::dispatchEvent(ClientPublicationEvent* event)
{
   StackLog(<< "dispatch ClientPublicationEvent");
   dispatchEventHelper(event, mTestUsages);
}

void
DumUserAgent::dispatchEvent(ServerOutOfDialogReqEvent* event)
{
   StackLog(<< "dispatch ServerOutOfDialogReqEvent");
   dispatchEventHelper(event, mTestUsages);
}

void 
DumUserAgent::add(TestUsage* usage)
{
   mTestUsages.insert(usage);
}

void 
DumUserAgent::remove(TestUsage* usage)
{
   mTestUsages.erase(usage);
}

bool
DumUserAgent::matchEvent(Event* event)
{
   for (TestUsages::iterator it = mTestUsages.begin(); it != mTestUsages.end(); ++it)
   {
      if ((*it)->isMyEvent(event))
      {
         return true;
      }
   }

   return false;
}

void
DumUserAgent::handleEvent(Event* eventRaw)
{
   boost::shared_ptr<Event> event(eventRaw);
   StackLog(<< "DumUserAgent::handleEvent: " << *eventRaw);

   boost::shared_ptr<SequenceSet> sset = getSequenceSet().lock(); 
   if (sset)
   {
      StackLog(<< " DumUserAgent::handleEvent to own sequenceset");      
      sset->enqueue(event);
      return;
   }
   else
   {
      for(TestUsages::iterator it = mTestUsages.begin(); it != mTestUsages.end(); it++)
      {
         boost::shared_ptr<SequenceSet> tusset= (*it)->getSequenceSet().lock(); 
         if (tusset)
         {
            StackLog(<< " DumUserAgent::handleEvent to nested usage sequenceSet");
            tusset->enqueue(event);
            return;
         }
      }
   }
   WarningLog(<< "DumUserAgent::handleEvent has no associated SequenceSet: discarding event " << *event);
}

TestUsage*
DumUserAgent::findUsage(Event* e)
{
   for (TestUsages::iterator it = mTestUsages.begin(); it != mTestUsages.end(); ++it)
   {
      if ((*it)->isMyEvent(e))
      {
         return *it;
      }
   }

   return 0;
}
