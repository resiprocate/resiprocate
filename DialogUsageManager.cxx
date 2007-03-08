 #include "resip/stack/Security.hxx"
#include "resip/stack/SecurityAttributes.hxx"
#include "resip/stack/ShutdownMessage.hxx"
#include "resip/stack/SipFrag.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/TransactionUserMessage.hxx"
#include "resip/stack/ConnectionTerminated.hxx"
#include "resip/dum/AppDialog.hxx"
#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/AppDialogSetFactory.hxx"
#include "resip/dum/BaseUsage.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/ClientInviteSession.hxx"
#include "resip/dum/ClientOutOfDialogReq.hxx"
#include "resip/dum/ClientPublication.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/ClientSubscription.hxx"
#include "resip/dum/DefaultServerReferHandler.hxx"
#include "resip/dum/DestroyUsage.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/ClientPagerMessage.hxx"
#include "resip/dum/DumException.hxx"
#include "resip/dum/DumShutdownHandler.hxx"
#include "resip/dum/DumFeatureMessage.hxx"
#include "resip/dum/InviteSessionCreator.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/KeepAliveManager.hxx"
#include "resip/dum/KeepAliveTimeout.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/OutOfDialogReqCreator.hxx"
#include "resip/dum/PagerMessageCreator.hxx"
#include "resip/dum/PublicationCreator.hxx"
#include "resip/dum/RedirectManager.hxx"
#include "resip/dum/RegistrationCreator.hxx"
#include "resip/dum/ServerAuthManager.hxx"
#include "resip/dum/ServerInviteSession.hxx"
#include "resip/dum/ServerPublication.hxx"
#include "resip/dum/ServerSubscription.hxx"
#include "resip/dum/SubscriptionCreator.hxx"
#include "resip/dum/SubscriptionHandler.hxx"
#include "resip/dum/UserAuthInfo.hxx"
#include "resip/dum/DumFeature.hxx"
#include "resip/dum/EncryptionManager.hxx"
#include "resip/dum/IdentityHandler.hxx"
#include "resip/dum/DumDecrypted.hxx"
#include "resip/dum/CertMessage.hxx"
#include "resip/dum/OutgoingEvent.hxx"
#include "resip/dum/DumHelper.hxx"
#include "resip/dum/MergedRequestRemovalCommand.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "rutil/Timer.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

DialogUsageManager::DialogUsageManager(SipStack& stack, bool createDefaultFeatures) :
   TransactionUser(TransactionUser::DoNotRegisterForTransactionTermination, TransactionUser::RegisterForConnectionTermination),
   mRedirectManager(new RedirectManager()),
   mInviteSessionHandler(0),
   mClientRegistrationHandler(0),
   mServerRegistrationHandler(0),
   mRedirectHandler(0),
   mDialogSetHandler(0),
   mRegistrationPersistenceManager(0),
   mIsDefaultServerReferHandler(true),
   mClientPagerMessageHandler(0),
   mServerPagerMessageHandler(0),
   mAppDialogSetFactory(new AppDialogSetFactory()),
   mStack(stack),
   mDumShutdownHandler(0),
   mShutdownState(Running)
{
   //TODO -- create default features
   mStack.registerTransactionUser(*this);
   addServerSubscriptionHandler("refer", new DefaultServerReferHandler());

   mIncomingTarget = new IncomingTarget(*this);
   mOutgoingTarget = new OutgoingTarget(*this);

   if (createDefaultFeatures)
   {
      SharedPtr<IdentityHandler> identity = SharedPtr<IdentityHandler>(new IdentityHandler(*this, *mIncomingTarget));

#if defined (USE_SSL)
      SharedPtr<EncryptionManager> encryptionIncoming = SharedPtr<EncryptionManager>(new EncryptionManager(*this, *mIncomingTarget));
      SharedPtr<EncryptionManager> encryptionOutgoing = SharedPtr<EncryptionManager>(new EncryptionManager(*this, *mOutgoingTarget));
#endif

      // default incoming features.
      addIncomingFeature(identity);
#if defined (USE_SSL)
      addIncomingFeature(encryptionIncoming);
#endif

      // default outgoing features.
#if defined (USE_SSL)
      addOutgoingFeature(encryptionOutgoing);
#endif

   }

}

DialogUsageManager::~DialogUsageManager()
{
   mShutdownState = Destroying;
   //InfoLog ( << "~DialogUsageManager" );

#if(0)
   // !kh!
   DialogSetMap::iterator dialogSet = mDialogSetMap.begin();
   for (; dialogSet != mDialogSetMap.end(); ++dialogSet)
   {
      delete dialogSet->second;
   }
#endif
   if(!mDialogSetMap.empty())
   {
      DebugLog(<< "DialogUsageManager::mDialogSetMap has " << mDialogSetMap.size() << " DialogSets");
      DialogSetMap::const_iterator ds = mDialogSetMap.begin();
      for(; ds != mDialogSetMap.end(); ++ds)
      {
         DebugLog(<< "DialgSetId:" << ds->first);
         DialogSet::DialogMap::const_iterator   d = ds->second->mDialogs.begin();
         for(; d != ds->second->mDialogs.end(); ++d)
         {
            //const Dialog* p = &(d->second);
            DebugLog(<<"DialogId:" << d->first << ", " << *d->second);
         }
      }
   }

   while(!mDialogSetMap.empty())
   {
      DialogSet*  ds = mDialogSetMap.begin()->second;
      delete ds;
   }

   if(mIsDefaultServerReferHandler)
   {
       delete mServerSubscriptionHandlers["refer"];
   }

   delete mIncomingTarget;
   delete mOutgoingTarget;

   //InfoLog ( << "~DialogUsageManager done" );
}

const Data& 
DialogUsageManager::name() const
{
   static Data n("DialogUsageManager");
   return n;
}

void
DialogUsageManager::addTransport( TransportType protocol,
                                  int port,
                                  IpVersion version,
                                  const Data& ipInterface,
                                  const Data& sipDomainname, // only used
                                  const Data& privateKeyPassPhrase,
                                  SecurityTypes::SSLType sslType)
{
   mStack.addTransport(protocol, port, version, StunDisabled, ipInterface,
                       sipDomainname, privateKeyPassPhrase, sslType);
}

SipStack& 
DialogUsageManager::getSipStack()
{
   return mStack;
}

Security*
DialogUsageManager::getSecurity()
{
   return mStack.getSecurity();
}

Data
DialogUsageManager::getHostAddress()
{
   return mStack.getHostAddress();
}

void
DialogUsageManager::onAllHandlesDestroyed()
{
   if (mDumShutdownHandler)
   {
      switch (mShutdownState)
      {
         case ShutdownRequested:
            InfoLog (<< "DialogUsageManager::onAllHandlesDestroyed: removing TU");
            //assert(mHandleMap.empty());
            mShutdownState = RemovingTransactionUser;
            mStack.unregisterTransactionUser(*this);
            break;
         default:
            break;
      }
   }
}


void
DialogUsageManager::shutdown(DumShutdownHandler* h, unsigned long giveUpSeconds)
{
   InfoLog (<< "shutdown giveup=" << giveUpSeconds << " dialogSets=" << mDialogSetMap.size());
   
   mDumShutdownHandler = h;
   mShutdownState = ShutdownRequested;
   mStack.requestTransactionUserShutdown(*this);
   shutdownWhenEmpty();
}

void
DialogUsageManager::shutdownIfNoUsages(DumShutdownHandler* h, unsigned long giveUpSeconds)
{
   InfoLog (<< "shutdown when no usages giveup=" << giveUpSeconds);

   mDumShutdownHandler = h;
   mShutdownState = ShutdownRequested;
   assert(0);
}

void
DialogUsageManager::forceShutdown(DumShutdownHandler* h)
{
   WarningLog (<< "force shutdown ");
   dumpHandles();
   
   mDumShutdownHandler = h;
   //HandleManager::shutdown();  // clear out usages
   mShutdownState = ShutdownRequested;
   DialogUsageManager::onAllHandlesDestroyed();
}

void DialogUsageManager::setAppDialogSetFactory(std::auto_ptr<AppDialogSetFactory> factory)
{
   mAppDialogSetFactory = factory;
}

SharedPtr<MasterProfile>&
DialogUsageManager::getMasterProfile()
{
   assert(mMasterProfile.get());
   return mMasterProfile;
}

SharedPtr<UserProfile>&
DialogUsageManager::getMasterUserProfile()
{
   assert(mMasterUserProfile.get());
   return mMasterUserProfile;
}

void DialogUsageManager::setMasterProfile(const SharedPtr<MasterProfile>& masterProfile)
{
   assert(!mMasterProfile.get());
   mMasterProfile = masterProfile;
   mMasterUserProfile = masterProfile; // required so that we can return a reference to SharedPtr<UserProfile> in getMasterUserProfile
}

void DialogUsageManager::setKeepAliveManager(std::auto_ptr<KeepAliveManager> manager)
{
   mKeepAliveManager = manager;
   mKeepAliveManager->setDialogUsageManager(this);
}

void DialogUsageManager::setRedirectManager(std::auto_ptr<RedirectManager> manager)
{
   mRedirectManager = manager;
}

void DialogUsageManager::setRedirectHandler(RedirectHandler* handler)
{
   mRedirectHandler = handler;
}

RedirectHandler* DialogUsageManager::getRedirectHandler()
{
   return mRedirectHandler;
}

void
DialogUsageManager::setClientAuthManager(std::auto_ptr<ClientAuthManager> manager)
{
   mClientAuthManager = manager;
}

void
DialogUsageManager::setServerAuthManager(SharedPtr<ServerAuthManager> manager)
{
   mIncomingFeatureList.insert(mIncomingFeatureList.begin(), manager);
}

void
DialogUsageManager::setClientRegistrationHandler(ClientRegistrationHandler* handler)
{
   assert(!mClientRegistrationHandler);
   mClientRegistrationHandler = handler;
}

void
DialogUsageManager::setServerRegistrationHandler(ServerRegistrationHandler* handler)
{
   assert(!mServerRegistrationHandler);
   mServerRegistrationHandler = handler;
}

void
DialogUsageManager::setDialogSetHandler(DialogSetHandler* handler)
{
   mDialogSetHandler = handler;
}

void
DialogUsageManager::setInviteSessionHandler(InviteSessionHandler* handler)
{
   assert(!mInviteSessionHandler);
   mInviteSessionHandler = handler;
}

void
DialogUsageManager::setRegistrationPersistenceManager(RegistrationPersistenceManager* manager)
{
   assert(!mRegistrationPersistenceManager);
   mRegistrationPersistenceManager = manager;
}

void
DialogUsageManager::setRemoteCertStore(auto_ptr<RemoteCertStore> store)
{
}

void
DialogUsageManager::addTimer(DumTimeout::Type type, unsigned long duration,
                             BaseUsageHandle target, unsigned int cseq, unsigned int rseq)
{
   DumTimeout t(type, duration, target, cseq, rseq);
   mStack.post(t, duration, this);
}

void
DialogUsageManager::addTimerMs(DumTimeout::Type type, unsigned long duration,
                               BaseUsageHandle target, unsigned int cseq, unsigned int rseq)
{
   DumTimeout t(type, duration, target, cseq, rseq);
   mStack.postMS(t, duration, this);
}

void
DialogUsageManager::addClientSubscriptionHandler(const Data& eventType, ClientSubscriptionHandler* handler)
{
   assert(handler);
   assert(mClientSubscriptionHandlers.count(eventType) == 0);
   mClientSubscriptionHandlers[eventType] = handler;
}

void
DialogUsageManager::addServerSubscriptionHandler(const Data& eventType, ServerSubscriptionHandler* handler)
{
   assert(handler);
   //default do-nothing server side refer handler can be replaced
   if (eventType == "refer" && mServerSubscriptionHandlers.count(eventType))
   {
      delete mServerSubscriptionHandlers[eventType];
      mIsDefaultServerReferHandler = false;
      //mServerSubscriptionHandlers.erase(eventType);
   }

   mServerSubscriptionHandlers[eventType] = handler;
}

void
DialogUsageManager::addClientPublicationHandler(const Data& eventType, ClientPublicationHandler* handler)
{
   assert(handler);
   assert(mClientPublicationHandlers.count(eventType) == 0);
   mClientPublicationHandlers[eventType] = handler;
}

void
DialogUsageManager::addServerPublicationHandler(const Data& eventType, ServerPublicationHandler* handler)
{
   assert(handler);
   assert(mServerPublicationHandlers.count(eventType) == 0);
   mServerPublicationHandlers[eventType] = handler;
}

void
DialogUsageManager::addOutOfDialogHandler(MethodTypes type, OutOfDialogHandler* handler)
{
   assert(handler);
   assert(mOutOfDialogHandlers.count(type) == 0);
   mOutOfDialogHandlers[type] = handler;
}

void
DialogUsageManager::setClientPagerMessageHandler(ClientPagerMessageHandler* handler)
{
   mClientPagerMessageHandler = handler;
}

void
DialogUsageManager::setServerPagerMessageHandler(ServerPagerMessageHandler* handler)
{
   mServerPagerMessageHandler = handler;
}

DialogSet*
DialogUsageManager::makeUacDialogSet(BaseCreator* creator, AppDialogSet* appDs)
{
   if (mDumShutdownHandler)
   {
      throw DumException("Cannot create new sessions when DUM is shutting down.", __FILE__, __LINE__);
   }

   if (appDs == 0)
   {
      appDs = new AppDialogSet(*this);
   }
   DialogSet* ds = new DialogSet(creator, *this);

   appDs->mDialogSet = ds;
   ds->mAppDialogSet = appDs;

   StackLog ( << "************* Adding DialogSet ***************" );
   StackLog ( << "Before: " << Inserter(mDialogSetMap) );
   mDialogSetMap[ds->getId()] = ds;
   StackLog ( << "After: " << Inserter(mDialogSetMap) );
   return ds;
}

SharedPtr<SipMessage>
DialogUsageManager::makeNewSession(BaseCreator* creator, AppDialogSet* appDs)
{
   makeUacDialogSet(creator, appDs);
   return creator->getLastRequest();
}

void
DialogUsageManager::makeResponse(SipMessage& response,
                                 const SipMessage& request,
                                 int responseCode,
                                 const Data& reason) const
{
   assert(request.isRequest());
   Helper::makeResponse(response, request, responseCode, reason);
}

void
DialogUsageManager::sendResponse(const SipMessage& response)
{
   assert(response.isResponse());
   mStack.send(response, this);
}

SharedPtr<SipMessage>
DialogUsageManager::makeInviteSession(const NameAddr& target, const SharedPtr<UserProfile>& userProfile, const SdpContents* initialOffer, AppDialogSet* appDs)
{
   return makeInviteSession(target, userProfile, initialOffer, None, 0, appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeInviteSession(const NameAddr& target, const SdpContents* initialOffer, AppDialogSet* appDs)
{
   return makeInviteSession(target, getMasterUserProfile(), initialOffer, None, 0, appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeInviteSession(const NameAddr& target, 
                                      const SharedPtr<UserProfile>& userProfile, 
                                      const SdpContents* initialOffer, 
                                      EncryptionLevel level, 
                                      const SdpContents* alternative, 
                                      AppDialogSet* appDs)
{
   SharedPtr<SipMessage> inv = makeNewSession(new InviteSessionCreator(*this, target, userProfile, initialOffer, level, alternative), appDs);
   DumHelper::setOutgoingEncryptionLevel(*inv, level);

   return inv;
}

SharedPtr<SipMessage>
DialogUsageManager::makeInviteSession(const NameAddr& target, 
                                      const SdpContents* initialOffer, 
                                      EncryptionLevel level, 
                                      const SdpContents* alternative,
                                      AppDialogSet* appDs)
{
   return makeInviteSession(target, getMasterUserProfile(), initialOffer, level, alternative, appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeInviteSessionFromRefer(const SipMessage& refer,
                                               ServerSubscriptionHandle serverSub,
                                               const SdpContents* initialOffer,
                                               AppDialogSet* appDs)
{
   return makeInviteSessionFromRefer(refer, serverSub, initialOffer, None, 0, appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeInviteSessionFromRefer(const SipMessage& refer,
                                               const SharedPtr<UserProfile>& userProfile, 
                                               const SdpContents* initialOffer,
                                               AppDialogSet* appDs)
{
   ServerSubscriptionHandle empty;
   return makeInviteSessionFromRefer(refer, userProfile, empty, initialOffer, None, 0, appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeInviteSessionFromRefer(const SipMessage& refer,
                                               ServerSubscriptionHandle serverSub,
                                               const SdpContents* initialOffer,
                                               EncryptionLevel level,
                                               const SdpContents* alternative,
                                               AppDialogSet* appDs)
{
   return makeInviteSessionFromRefer(refer, serverSub.isValid() ? serverSub->mDialog.mDialogSet.getUserProfile() : getMasterUserProfile(), serverSub, initialOffer, level, alternative, appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeInviteSessionFromRefer(const SipMessage& refer,
                                               const SharedPtr<UserProfile>& userProfile, 
                                               ServerSubscriptionHandle serverSub,
                                               const SdpContents* initialOffer,
                                               EncryptionLevel level,
                                               const SdpContents* alternative,
                                               AppDialogSet* appDs)
{
   if (serverSub.isValid())
   {
      DebugLog(<< "implicit subscription");
      //generate and send 100
      SipFrag contents;
      contents.message().header(h_StatusLine).statusCode() = 100;
      contents.message().header(h_StatusLine).reason() = "Trying";
      //will be cloned...ServerSub may not have the most efficient API possible
      serverSub->setSubscriptionState(Active);
      SharedPtr<SipMessage> notify = serverSub->update(&contents);
//   mInviteSessionHandler->onReadyToSend(InviteSessionHandle::NotValid(), notify);
      serverSub->send(notify);
   }

   //19.1.5
   NameAddr target = refer.header(h_ReferTo);
   target.uri().removeEmbedded();
   target.uri().remove(p_method);

   // !jf! this code assumes you have a UserProfile
   SharedPtr<SipMessage> inv = makeNewSession(new InviteSessionCreator(*this,
                                                                       target,
                                                                       userProfile,
                                                                       initialOffer, level, alternative, serverSub), appDs);
   DumHelper::setOutgoingEncryptionLevel(*inv, level);

   //could pass dummy target, then apply merge rules from 19.1.5...or
   //makeNewSession would use rules from 19.1.5
   if (refer.exists(h_ReferredBy))
   {
      inv->header(h_ReferredBy) = refer.header(h_ReferredBy);
   }

   const Uri& referTo = refer.header(h_ReferTo).uri();
   //19.1.5
   if (referTo.hasEmbedded() && referTo.embedded().exists(h_Replaces))
   {
      inv->header(h_Replaces) = referTo.embedded().header(h_Replaces);
   }
   return inv;
}

SharedPtr<SipMessage>
DialogUsageManager::makeRefer(const NameAddr& target, const SharedPtr<UserProfile>& userProfile, const H_ReferTo::Type& referTo, AppDialogSet* appDs)
{
   return makeNewSession(new SubscriptionCreator(*this, target, userProfile, referTo), appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeRefer(const NameAddr& target, const H_ReferTo::Type& referTo, AppDialogSet* appDs)
{
   return makeNewSession(new SubscriptionCreator(*this, target, getMasterUserProfile(), referTo), appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeSubscription(const NameAddr& target, const SharedPtr<UserProfile>& userProfile, const Data& eventType, AppDialogSet* appDs)
{
   assert(userProfile.get());
   return makeNewSession(new SubscriptionCreator(*this, target, userProfile, eventType, userProfile->getDefaultSubscriptionTime()), appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeSubscription(const NameAddr& target, const SharedPtr<UserProfile>& userProfile, const Data& eventType,
                                     UInt32 subscriptionTime, AppDialogSet* appDs)
{
   return makeNewSession(new SubscriptionCreator(*this, target, userProfile, eventType, subscriptionTime), appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeSubscription(const NameAddr& target, const SharedPtr<UserProfile>& userProfile, const Data& eventType,
                                     UInt32 subscriptionTime, int refreshInterval, AppDialogSet* appDs)
{
   return makeNewSession(new SubscriptionCreator(*this, target, userProfile, eventType, subscriptionTime, refreshInterval), appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeSubscription(const NameAddr& target, const Data& eventType, AppDialogSet* appDs)
{
   return makeNewSession(new SubscriptionCreator(*this, target, getMasterUserProfile(), eventType, getMasterProfile()->getDefaultSubscriptionTime()), appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeSubscription(const NameAddr& target, const Data& eventType,
                                     UInt32 subscriptionTime, AppDialogSet* appDs)
{
   return makeNewSession(new SubscriptionCreator(*this, target, getMasterUserProfile(), eventType, subscriptionTime), appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeSubscription(const NameAddr& target, const Data& eventType,
                                     UInt32 subscriptionTime, int refreshInterval, AppDialogSet* appDs)
{
   return makeNewSession(new SubscriptionCreator(*this, target, getMasterUserProfile(), eventType, subscriptionTime, refreshInterval), appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeRegistration(const NameAddr& target, const SharedPtr<UserProfile>& userProfile, AppDialogSet* appDs)
{
   assert(userProfile.get());
   return makeNewSession(new RegistrationCreator(*this, target, userProfile, userProfile->getDefaultRegistrationTime()), appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeRegistration(const NameAddr& target, const SharedPtr<UserProfile>& userProfile, UInt32 registrationTime, AppDialogSet* appDs)
{
   return makeNewSession(new RegistrationCreator(*this, target, userProfile, registrationTime), appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeRegistration(const NameAddr& target, AppDialogSet* appDs)
{
   return makeNewSession(new RegistrationCreator(*this, target, getMasterUserProfile(), getMasterProfile()->getDefaultRegistrationTime()), appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeRegistration(const NameAddr& target, UInt32 registrationTime, AppDialogSet* appDs)
{
   return makeNewSession(new RegistrationCreator(*this, target, getMasterUserProfile(), registrationTime), appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makePublication(const NameAddr& targetDocument,
                                    const SharedPtr<UserProfile>& userProfile,
                                    const Contents& body,
                                    const Data& eventType,
                                    UInt32 expiresSeconds,
                                    AppDialogSet* appDs)
{
   return makeNewSession(new PublicationCreator(*this, targetDocument, userProfile, body, eventType, expiresSeconds), appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makePublication(const NameAddr& targetDocument,
                                    const Contents& body,
                                    const Data& eventType,
                                    UInt32 expiresSeconds,
                                    AppDialogSet* appDs)
{
   return makeNewSession(new PublicationCreator(*this, targetDocument, getMasterUserProfile(), body, eventType, expiresSeconds), appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeOutOfDialogRequest(const NameAddr& target, const SharedPtr<UserProfile>& userProfile, const MethodTypes meth, AppDialogSet* appDs)
{
   return makeNewSession(new OutOfDialogReqCreator(*this, meth, target, userProfile), appDs);
}

SharedPtr<SipMessage>
DialogUsageManager::makeOutOfDialogRequest(const NameAddr& target, const MethodTypes meth, AppDialogSet* appDs)
{
   return makeNewSession(new OutOfDialogReqCreator(*this, meth, target, getMasterUserProfile()), appDs);
}

ClientPagerMessageHandle
DialogUsageManager::makePagerMessage(const NameAddr& target, const SharedPtr<UserProfile>& userProfile, AppDialogSet* appDs)
{
   if (!mClientPagerMessageHandler)
   {
      throw DumException("Cannot send MESSAGE messages without a ClientPagerMessageHandler", __FILE__, __LINE__);
   }
   DialogSet* ds = makeUacDialogSet(new PagerMessageCreator(*this, target, userProfile), appDs);
   ClientPagerMessage* cpm = new ClientPagerMessage(*this, *ds);
   ds->mClientPagerMessage = cpm;
   return cpm->getHandle();
}

ClientPagerMessageHandle
DialogUsageManager::makePagerMessage(const NameAddr& target, AppDialogSet* appDs)
{
   return makePagerMessage(target, getMasterUserProfile(), appDs);
}

void
DialogUsageManager::send(SharedPtr<SipMessage> msg)
{
   // !slg! There is probably a more efficient way to get the userProfile here (pass it in?)
   DialogSet* ds = findDialogSet(DialogSetId(*msg));
   UserProfile* userProfile;
   if (ds == 0)
   {
      userProfile = getMasterUserProfile().get();
   }
   else
   {
      userProfile = ds->getUserProfile().get();
   }

   assert(userProfile);
   if (!userProfile->isAnonymous() && userProfile->hasUserAgent())
   {
      msg->header(h_UserAgent).value() = userProfile->getUserAgent();
   }
   if (userProfile->isAnonymous())
   {
      msg->remove(h_ReplyTo);
      msg->remove(h_UserAgent);
      msg->remove(h_Organization);
      msg->remove(h_Server);
      msg->remove(h_Subject);
      msg->remove(h_InReplyTo);

      msg->remove(h_CallInfos);
      msg->remove(h_Warnings);
   }
   
   assert(userProfile);
   if (msg->isRequest() 
       && userProfile->hasProxyRequires() 
       && msg->header(h_RequestLine).method() != ACK 
       && msg->header(h_RequestLine).method() != CANCEL)
   {
      msg->header(h_ProxyRequires) = userProfile->getProxyRequires();
   }
   
   if (msg->isRequest())
   {
      // We may not need to call reset() if makeRequest is always used.
      if (msg->header(h_RequestLine).method() != CANCEL &&
          msg->header(h_RequestLine).method() != ACK &&
          msg->exists(h_Vias))
      {
         msg->header(h_Vias).front().param(p_branch).reset();
      }

      if (msg->exists(h_Vias))
      {
         if(!userProfile->getRportEnabled())
         {
            msg->header(h_Vias).front().remove(p_rport);
         }
         int fixedTransportPort = userProfile->getFixedTransportPort();
         if(fixedTransportPort != 0)
         {
            msg->header(h_Vias).front().sentPort() = fixedTransportPort;
         }
         const Data& fixedTransportInterface = userProfile->getFixedTransportInterface();
         if(!fixedTransportInterface.empty())
         {
            msg->header(h_Vias).front().sentHost() = fixedTransportInterface;
         }
      }

      if (mClientAuthManager.get() && msg->header(h_RequestLine).method() != ACK)
      {
         mClientAuthManager->addAuthentication(*msg);
      }

      // Add outbound decorator from userprofile
      SharedPtr<MessageDecorator> outboundDecorator = userProfile->getOutboundDecorator();
      if (outboundDecorator.get())
      {
         msg->addOutboundDecorator(outboundDecorator.get());
      }
   }

   DebugLog (<< "SEND: " << std::endl << std::endl << *msg);

   OutgoingEvent* event = new OutgoingEvent(msg);
   outgoingProcess(auto_ptr<Message>(event));
}

void DialogUsageManager::outgoingProcess(auto_ptr<Message> message)
{
   Data tid = Data::Empty;
   {
      OutgoingEvent* sipMsg = dynamic_cast<OutgoingEvent*>(message.get());
      if (sipMsg)
      {
         tid = sipMsg->getTransactionId();
      }
      
      DumFeatureMessage* featureMsg = dynamic_cast<DumFeatureMessage*>(message.get());
      if (featureMsg)
      {
         InfoLog(<<"Got a DumFeatureMessage" << featureMsg);
         tid = featureMsg->getTransactionId();
      }
   }

   if (tid == Data::Empty && mOutgoingMessageInterceptor.get())
   {
      mOutgoingMessageInterceptor->process(message.get());
      return;
   }
   else if (tid != Data::Empty && !mOutgoingFeatureList.empty())
   {
      FeatureChainMap::iterator it;
      //efficiently find or create FeatureChain, should prob. be a utility template
      {
         FeatureChainMap::iterator lb = mOutgoingFeatureChainMap.lower_bound(tid);
         if (lb != mOutgoingFeatureChainMap.end() && !(mOutgoingFeatureChainMap.key_comp()(tid, lb->first)))
         {
            it = lb;
         }
         else
         {
            it = mOutgoingFeatureChainMap.insert(lb, FeatureChainMap::value_type(tid, new DumFeatureChain(*this, mOutgoingFeatureList, *mOutgoingTarget)));
         }
      }
      
      DumFeatureChain::ProcessingResult res = it->second->process(message.get());
      
      if (res & DumFeatureChain::ChainDoneBit)
      {
         delete it->second;
         mOutgoingFeatureChainMap.erase(it);
      }

      if (res & DumFeatureChain::EventTakenBit)
      {
         message.release();
         return;
      }
   }

   OutgoingEvent* event = dynamic_cast<OutgoingEvent*>(message.get());
   assert(event);
   if (event)
   {
      if (event->message()->isRequest())
      {
         DialogSet* ds = findDialogSet(DialogSetId(*event->message()));
         UserProfile* userProfile;
         if (ds == 0)
         {
            userProfile = getMasterUserProfile().get();
         }
         else
         {
            userProfile = ds->getUserProfile().get();
         }

         assert(userProfile);

         //!dcm! -- unique SharedPtr to auto_ptr conversion prob. a worthwhile
         //optimzation here. SharedPtr would ahve to be changed; would
         //throw/assert if not unique.
         std::auto_ptr<SipMessage> toSend(static_cast<SipMessage*>(event->message()->clone()));

         if (event->message()->exists(h_Routes) &&
             !event->message()->header(h_Routes).empty() &&
             !event->message()->header(h_Routes).front().uri().exists(p_lr))
         {
            Helper::processStrictRoute(*toSend);
            sendUsingOutboundIfAppropriate(*userProfile, toSend);
         }
         else
         {
            sendUsingOutboundIfAppropriate(*userProfile, toSend);
         }
      }
      else
      {
         sendResponse(*event->message());
      }
   }
}

void
DialogUsageManager::sendUsingOutboundIfAppropriate(UserProfile& userProfile, auto_ptr<SipMessage> msg)
{
   //a little inefficient, branch parameter might be better
   DialogId id(*msg);
   if (userProfile.hasOutboundProxy() && !findDialog(id))
   {
      DebugLog ( << "Using outbound proxy: " 
                 << userProfile.getOutboundProxy().uri() 
                 << " -> " << msg->brief());

      if (userProfile.getExpressOutboundAsRouteSetEnabled())
      {
         // prepend the outbound proxy to the service route
         msg->header(h_Routes).push_front(NameAddr(userProfile.getOutboundProxy().uri()));
         mStack.send(msg, this);
      }
      else
      {
         mStack.sendTo(msg, userProfile.getOutboundProxy().uri(), this);
      }
   }
   else
   {
      DebugLog (<< "Send: " << msg->brief());
      mStack.send(msg, this);
   }
}


void
DialogUsageManager::end(DialogSetId setid)
{
   DialogSet* ds = findDialogSet(setid);
   if (ds == 0)
   {
      throw Exception("Request no longer exists", __FILE__, __LINE__);
   }
   else
   {
      ds->end();
   }
}

void
DialogUsageManager::destroy(const BaseUsage* usage)
{
   if (mShutdownState != Destroying)
   {
      post(new DestroyUsage(usage->mHandle));
   }
   else
   {
      InfoLog(<< "DialogUsageManager::destroy() not posting to stack");
   }
}

void
DialogUsageManager::destroy(DialogSet* dset)
{
   if (mShutdownState != Destroying)
   {
      post(new DestroyUsage(dset));
   }
   else
   {
      InfoLog(<< "DialogUsageManager::destroy() not posting to stack");
   }
}

void
DialogUsageManager::destroy(Dialog* d)
{
   if (mShutdownState != Destroying)
   {
      post(new DestroyUsage(d));
   }
   else
   {
      InfoLog(<< "DialogUsageManager::destroy() not posting to stack");
   }
}


Dialog*
DialogUsageManager::findDialog(const DialogId& id)
{
   DialogSet* ds = findDialogSet(id.getDialogSetId());
   if (ds)
   {
      return ds->findDialog(id);
   }
   else
   {
      return 0;
   }
}


InviteSessionHandle
DialogUsageManager::findInviteSession(DialogId id)
{
   Dialog* dialog = findDialog(id);
   if (dialog && dialog->mInviteSession)
   {
      return dialog->mInviteSession->getSessionHandle();
   }
   else
   {
      return InviteSessionHandle::NotValid();
   }
}

pair<InviteSessionHandle, int>
DialogUsageManager::findInviteSession(CallId replaces)
{
   //486/481/603 decision making logic where?  App may not wish to keep track of
   //invitesession state
   //Logic is here for now.
   InviteSessionHandle is = findInviteSession(DialogId(replaces.value(),
                                                       replaces.param(p_toTag),
                                                       replaces.param(p_fromTag)));
   int ErrorStatusCode = 481; // Call/Transaction Does Not Exist

   // If we matched a session - Do RFC3891 Section 3 Processing
   if(!(is == InviteSessionHandle::NotValid()))
   {
      // Note some missing checks are:
      // 1.  If the Replaces header field matches more than one dialog, the UA must act as
      //     if no match was found
      // 2.  Verify that the initiator of the new Invite is authorized
      if(is->isTerminated())
      {
         ErrorStatusCode = 603; // Declined
         is = InviteSessionHandle::NotValid();
      }
      else if(is->isConnected())
      {
         // Check if early-only flag is present in replaces header
         if(replaces.exists(p_earlyOnly))
         {
            ErrorStatusCode = 486; // Busy Here
            is = InviteSessionHandle::NotValid();
         }
      }      
      else if(!is->isEarly())
      {
         // replaces can't be used on early dialogs that were not initiated by this UA - ie. InviteSession::Proceeding state
         ErrorStatusCode = 481; // Call/Transaction Does Not Exist
         is = InviteSessionHandle::NotValid();
      }
   }
   return make_pair(is, ErrorStatusCode);
}

void
DialogUsageManager::internalProcess(std::auto_ptr<Message> msg)
{
   // After a Stack ShutdownMessage has been received, don't do anything else in dum
   if (mShutdownState == Shutdown)
   {
      return;
   }

   TransactionUserMessage* tuMsg = dynamic_cast<TransactionUserMessage*>(msg.get());
   if (tuMsg)
   {
      InfoLog (<< "TU unregistered ");
      assert(mShutdownState == RemovingTransactionUser);
      assert(tuMsg->type() == TransactionUserMessage::TransactionUserRemoved);
      mShutdownState = Shutdown;
      if (mDumShutdownHandler)
      {
         mDumShutdownHandler->onDumCanBeDeleted();
         mDumShutdownHandler = 0; // prevent mDumShutdownHandler getting called more than once
      }
      return;
   }
   
   DestroyUsage* destroyUsage = dynamic_cast<DestroyUsage*>(msg.get());
   if (destroyUsage)
   {
      //DebugLog(<< "Destroying usage" );
      destroyUsage->destroy();
      return;
   }

   DumTimeout* dumMsg = dynamic_cast<DumTimeout*>(msg.get());
   if (dumMsg)
   {
      //DebugLog(<< "Timeout Message" );
      if (!dumMsg->getBaseUsage().isValid())
      {
         return;
      }
      
      dumMsg->getBaseUsage()->dispatch(*dumMsg);
      return;
   }

   KeepAliveTimeout* keepAliveMsg = dynamic_cast<KeepAliveTimeout*>(msg.get());
   if (keepAliveMsg)
   {
      //DebugLog(<< "Keep Alive Message" );
      if (mKeepAliveManager.get())
      {
         mKeepAliveManager->process(*keepAliveMsg);
      }
      return;      
   }

   ConnectionTerminated* terminated = dynamic_cast<ConnectionTerminated*>(msg.get());
   if (terminated)
   {
      DebugLog(<< "connection terminated message");
      if (mConnectionTerminatedEventDispatcher.dispatch(msg.get()))
      {
         msg.release();
      }
      return;
   }
   
   DumCommand* command = dynamic_cast<DumCommand*>(msg.get());
   if (command)
   {
      //DebugLog(<< "DumCommand" );
      command->executeCommand();
      return;      
   }

   incomingProcess(msg);
}

void DialogUsageManager::incomingProcess(std::auto_ptr<Message> msg)
{
   //call or create feature chain if appropriate
   Data tid = Data::Empty;
   {
      SipMessage* sipMsg = dynamic_cast<SipMessage*>(msg.get());
      if (sipMsg)
      {
         tid = sipMsg->getTransactionId();
         bool garbage=false;
         Data reason;

         if(!sipMsg->header(h_From).isWellFormed())
         {
            garbage=true;
            reason.append("Malformed From, ",16);
         }

         if(!sipMsg->header(h_To).isWellFormed())
         {
            garbage=true;
            reason.append("Malformed To, ",14);
         }

         if(!sipMsg->header(h_CallId).isWellFormed())
         {
            garbage=true;
            reason.append("Malformed Call-Id, ",19);
         }
         
         if(garbage)
         {
            if(sipMsg->isRequest() && !sipMsg->method()==ACK)
            {
               // .bwc. Either we need to trim the last comma off, or make this 
               // a proper sentence fragment. This is more fun.
               reason.append("fix your code!",14);
               SipMessage failure;
               makeResponse(failure, *sipMsg, 400, reason);
               sendResponse(failure);
            }
            
            // .bwc. Only forge a response when appropriate, but return in any 
            // case.
            return;
         }
      }
      
      DumFeatureMessage* featureMsg = dynamic_cast<DumFeatureMessage*>(msg.get());
      if (featureMsg)
      {
         //DebugLog(<<"Got a DumFeatureMessage" << featureMsg);
         tid = featureMsg->getTransactionId();
      }
   }
   if (tid != Data::Empty && !mIncomingFeatureList.empty())
   {
      FeatureChainMap::iterator it;     
      //efficiently find or create FeatureChain, should prob. be a utility template
      {
         FeatureChainMap::iterator lb = mIncomingFeatureChainMap.lower_bound(tid);
         if (lb != mIncomingFeatureChainMap.end() && !(mIncomingFeatureChainMap.key_comp()(tid, lb->first)))
         {
            it = lb;
         }
         else
         {
            assert(dynamic_cast<SipMessage*>(msg.get()));
            it = mIncomingFeatureChainMap.insert(lb, FeatureChainMap::value_type(tid, new DumFeatureChain(*this, mIncomingFeatureList, *mIncomingTarget)));
         }
      }
      
      DumFeatureChain::ProcessingResult res = it->second->process(msg.get());
      
      if (res & DumFeatureChain::ChainDoneBit)
      {
         delete it->second;
         mIncomingFeatureChainMap.erase(it);
         //DebugLog(<< "feature chain deleted" << endl);
      }
 
      if (res & DumFeatureChain::EventTakenBit)
      {
         msg.release();
         //DebugLog(<< "event taken");
         return;
      }
   }
   
   try
   {
      InfoLog (<< "Got: " << msg->brief());
      DumDecrypted* decryptedMsg = dynamic_cast<DumDecrypted*>(msg.get());
      SipMessage* sipMsg = 0;
      if (decryptedMsg)
      {
         sipMsg = decryptedMsg->decrypted();
      }
      else
      {
         sipMsg = dynamic_cast<SipMessage*>(msg.get());
      }

      if (sipMsg)
      {
         //DebugLog ( << "DialogUsageManager::process: " << sipMsg->brief());
         if (sipMsg->isRequest())
         {
            // Validate Request URI
            if( !validateRequestURI(*sipMsg) )
            {
               DebugLog (<< "Failed RequestURI validation " << *sipMsg);
               return;
            }

            // Continue validation on all requests, except ACK and CANCEL
            if(sipMsg->header(h_RequestLine).method() != ACK &&
               sipMsg->header(h_RequestLine).method() != CANCEL)
            {
               if( !validateRequiredOptions(*sipMsg) )
               {
                  DebugLog (<< "Failed required options validation " << *sipMsg);
                  return;
               }
               if( getMasterProfile()->validateContentEnabled() && !validateContent(*sipMsg) )
               {
                  DebugLog (<< "Failed content validation " << *sipMsg);
                  return;
               }
               if( getMasterProfile()->validateAcceptEnabled() && !validateAccept(*sipMsg) )
               {
                  DebugLog (<< "Failed accept validation " << *sipMsg);
                  return;
               }
            }
            if (sipMsg->header(h_From).exists(p_tag))
            {
               if (mergeRequest(*sipMsg) )
               {
                  InfoLog (<< "Merged request: " << *sipMsg);
                  return;
               }
            }
            processRequest(*sipMsg);
         }
         else
         {
            processResponse(*sipMsg);
         }
      }
   }
   catch(BaseException& e)
   {
      //unparseable, bad 403 w/ 2543 trans it from FWD, etc
	  ErrLog(<<"Illegal message rejected: " << e.getMessage());
   }
}

// return true if there is more to do
bool 
DialogUsageManager::process()
{
   if (mFifo.messageAvailable())
   {
      internalProcess(std::auto_ptr<Message>(mFifo.getNext()));
   }
   return mFifo.messageAvailable();
}

bool 
DialogUsageManager::process(int timeoutMs)
{
   if(timeoutMs == -1)
   {
      internalProcess(std::auto_ptr<Message>(mFifo.getNext()));
   }
   else
   {
      std::auto_ptr<Message> msg(mFifo.getNext(timeoutMs));
      if (msg.get())
      {
         internalProcess(msg);
      }
   }
   return mFifo.messageAvailable();
}

bool
DialogUsageManager::validateRequestURI(const SipMessage& request)
{
   // RFC3261 - 8.2.1
   if (!getMasterProfile()->isMethodSupported(request.header(h_RequestLine).getMethod()))
   {
      InfoLog (<< "Received an unsupported method: " << request.brief());

      SipMessage failure;
      makeResponse(failure, request, 405);
      failure.header(h_Allows) = getMasterProfile()->getAllowedMethods();
      sendResponse(failure);

      return false;
   }

   // RFC3261 - 8.2.2
   if (!getMasterProfile()->isSchemeSupported(request.header(h_RequestLine).uri().scheme()))
   {
      InfoLog (<< "Received an unsupported scheme: " << request.brief());
      SipMessage failure;
      makeResponse(failure, request, 416);
      sendResponse(failure);

	  return false;
   }

   return true;
}


bool
DialogUsageManager::validateRequiredOptions(const SipMessage& request)
{
   // RFC 2162 - 8.2.2
   if(request.exists(h_Requires) &&                 // Don't check requires if method is ACK or CANCEL
      (request.header(h_RequestLine).getMethod() != ACK ||
       request.header(h_RequestLine).getMethod() != CANCEL))
   {
      Tokens unsupported = getMasterProfile()->getUnsupportedOptionsTags(request.header(h_Requires));
	  if (!unsupported.empty())
	  {
	     InfoLog (<< "Received an unsupported option tag(s): " << request.brief());

         SipMessage failure;
         makeResponse(failure, request, 420);
         failure.header(h_Unsupporteds) = unsupported;
         sendResponse(failure);

         return false;
	  }
   }

   return true;
}

bool
DialogUsageManager::validateContent(const SipMessage& request)
{
   // RFC3261 - 8.2.3
   // Don't need to validate content headers if they are specified as optional in the content-disposition
   if (!(request.exists(h_ContentDisposition) &&
         request.header(h_ContentDisposition).isWellFormed() &&
        request.header(h_ContentDisposition).exists(p_handling) &&
        isEqualNoCase(request.header(h_ContentDisposition).param(p_handling), Symbols::Optional)))
   {
     if (request.exists(h_ContentType) && !getMasterProfile()->isMimeTypeSupported(request.header(h_RequestLine).method(), request.header(h_ContentType)))
      {
         InfoLog (<< "Received an unsupported mime type: " << request.header(h_ContentType) << " for " << request.brief());

         SipMessage failure;
         makeResponse(failure, request, 415);
         failure.header(h_Accepts) = getMasterProfile()->getSupportedMimeTypes(request.header(h_RequestLine).method());
         sendResponse(failure);

         return false;
      }

     if (request.exists(h_ContentEncoding) && !getMasterProfile()->isContentEncodingSupported(request.header(h_ContentEncoding)))
      {
         InfoLog (<< "Received an unsupported mime type: " << request.header(h_ContentEncoding) << " for " << request.brief());
         SipMessage failure;
         makeResponse(failure, request, 415);
         failure.header(h_AcceptEncodings) = getMasterProfile()->getSupportedEncodings();
         sendResponse(failure);

         return false;
      }

      if (getMasterProfile()->validateContentLanguageEnabled() &&
          request.exists(h_ContentLanguages) && !getMasterProfile()->isLanguageSupported(request.header(h_ContentLanguages)))
      {
         InfoLog (<< "Received an unsupported language: " << request.header(h_ContentLanguages).front() << " for " << request.brief());

         SipMessage failure;
         makeResponse(failure, request, 415);
         failure.header(h_AcceptLanguages) = getMasterProfile()->getSupportedLanguages();
         sendResponse(failure);

         return false;
      }
   }

   return true;
}

bool
DialogUsageManager::validateAccept(const SipMessage& request)
{
   MethodTypes method = request.header(h_RequestLine).method();
   // checks for Accept to comply with SFTF test case 216
   if(request.exists(h_Accepts))
   {
      for (Mimes::const_iterator i = request.header(h_Accepts).begin();
           i != request.header(h_Accepts).end(); i++)
      {
         if (getMasterProfile()->isMimeTypeSupported(method, *i))
         {
            return true;  // Accept header passes validation if we support as least one of the mime types
         }
      }
   }
   // If no Accept header then application/sdp should be assumed for certain methods
   else if(method == INVITE ||
           method == OPTIONS ||
           method == PRACK ||
           method == UPDATE)
   {
      if (getMasterProfile()->isMimeTypeSupported(request.header(h_RequestLine).method(), Mime("application", "sdp")))
      {
         return true;
      }
   }
   else
   {
      // Other method without an Accept Header
      return true;
   }

   InfoLog (<< "Received unsupported mime types in accept header: " << request.brief());
   SipMessage failure;
   makeResponse(failure, request, 406);
   failure.header(h_Accepts) = getMasterProfile()->getSupportedMimeTypes(method);
   sendResponse(failure);
   return false;
}

bool
DialogUsageManager::mergeRequest(const SipMessage& request)
{
   assert(request.isRequest());
   assert(request.isExternal());

   if (!request.header(h_To).exists(p_tag))
   {
      if (mMergedRequests.count(MergedRequestKey(request, getMasterProfile()->checkReqUriInMergeDetectionEnabled())))
      {
         SipMessage failure;
         makeResponse(failure, request, 482, "Merged Request");
         failure.header(h_AcceptLanguages) = getMasterProfile()->getSupportedLanguages();
         sendResponse(failure);
         return true;
      }
   }

   return false;
}

void
DialogUsageManager::processRequest(const SipMessage& request)
{
   DebugLog ( << "DialogUsageManager::processRequest: " << request.brief());

   if (mShutdownState != Running && mShutdownState != ShutdownRequested)
   {
      WarningLog (<< "Ignoring a request since we are shutting down " << request.brief());

      SipMessage failure;
      makeResponse(failure, request, 480, "UAS is shutting down");
      sendResponse(failure);
      return;
   }

   if (request.header(h_RequestLine).method() == PUBLISH)
   {
      processPublish(request);
      return;
   }

   bool toTag = request.header(h_To).exists(p_tag);
   if(request.header(h_RequestLine).getMethod() == REGISTER && toTag && getMasterProfile()->allowBadRegistrationEnabled())
   {
       toTag = false;
   }

   assert(mAppDialogSetFactory.get());
   // !jf! note, the logic was reversed during ye great merge of March of Ought 5
   if (toTag ||
       findDialogSet(DialogSetId(request)))
   {
      switch (request.header(h_RequestLine).getMethod())
      {
         case REGISTER:
         {
            SipMessage failure;
            makeResponse(failure, request, 400, "Registration requests can't have To: tags.");
            failure.header(h_AcceptLanguages) = getMasterProfile()->getSupportedLanguages();
            sendResponse(failure);
            break;
         }

         default:
         {
            DialogSet* ds = findDialogSet(DialogSetId(request));
            if (ds == 0)
            {
               if (request.header(h_RequestLine).method() != ACK)
               {
                  SipMessage failure;
                  makeResponse(failure, request, 481);
                  failure.header(h_AcceptLanguages) = getMasterProfile()->getSupportedLanguages();
                  InfoLog (<< "Rejected request (which was in a dialog) " << request.brief());
                  sendResponse(failure);
               }
               else
               {
                  InfoLog (<< "ACK doesn't match any dialog" << request.brief());
               }
            }
            else
            {
               InfoLog (<< "Handling in-dialog request: " << request.brief());
               ds->dispatch(request);
            }
         }
      }
   }
   else
   {
      switch (request.header(h_RequestLine).getMethod())
      {
         case ACK:
            DebugLog (<< "Discarding request: " << request.brief());
            break;

         case PRACK:
         case BYE:
         case UPDATE:
         case INFO: // !rm! in an ideal world
         {
            SipMessage failure;
            makeResponse(failure, request, 481);
            failure.header(h_AcceptLanguages) = getMasterProfile()->getSupportedLanguages();
            sendResponse(failure);
            break;
         }
         case CANCEL:
         {
            // find the appropropriate ServerInvSession
            CancelMap::iterator i = mCancelMap.find(request.getTransactionId());
            if (i != mCancelMap.end())
            {
               i->second->dispatch(request);
            }
            else
            {
               InfoLog (<< "Received a CANCEL on a non-existent transaction ");
               SipMessage failure;
               makeResponse(failure, request, 481);
               sendResponse(failure);
            }
            break;
         }
         case PUBLISH:
            assert(false);
         case SUBSCRIBE:
            if (!checkEventPackage(request))
            {
               InfoLog (<< "Rejecting request (unsupported package) " 
                        << request.brief());
               return;
            }
         case NOTIFY : // handle unsolicited (illegal) NOTIFYs
         case INVITE:   // new INVITE
         case REFER:    // out-of-dialog REFER
            //case INFO :    // handle non-dialog (illegal) INFOs
         case OPTIONS : // handle non-dialog OPTIONS
         case MESSAGE :
         case REGISTER:
         {
            {
               DialogSetId id(request);
               //cryptographically dangerous
               if(mDialogSetMap.find(id) != mDialogSetMap.end()) 
               {
                  // this can only happen if someone sends us a request with the same callid and from tag as one 
                  // that is in the process of destroying - since this is bad endpoint behaviour - we will 
                  // reject the request with a 400 response
                  SipMessage badrequest;
                  makeResponse(badrequest, request, 400);
                  badrequest.header(h_AcceptLanguages) = getMasterProfile()->getSupportedLanguages();
                  sendResponse(badrequest);
                  return;
               }
            }
            if (mDumShutdownHandler)
            {
               SipMessage forbidden;
               makeResponse(forbidden, request, 480);
               forbidden.header(h_AcceptLanguages) = getMasterProfile()->getSupportedLanguages();
               sendResponse(forbidden);
               return;
            }
            try
            {
               DialogSet* dset =  new DialogSet(request, *this);

               StackLog ( << "*********** Calling AppDialogSetFactory *************"  );
               AppDialogSet* appDs = mAppDialogSetFactory->createAppDialogSet(*this, request);
               appDs->mDialogSet = dset;
               dset->setUserProfile(appDs->selectUASUserProfile(request));
               dset->mAppDialogSet = appDs;

               StackLog ( << "************* Adding DialogSet ***************" );
               StackLog ( << "Before: " << Inserter(mDialogSetMap) );
               mDialogSetMap[dset->getId()] = dset;
               StackLog ( << "After: Req" << Inserter(mDialogSetMap) );

               dset->dispatch(request);
            }
            catch (BaseException& e)
            {
               SipMessage failure;
               makeResponse(failure, request, 400, e.getMessage());
               failure.header(h_AcceptLanguages) = getMasterProfile()->getSupportedLanguages();
               sendResponse(failure);
            }

            break;
         }
         case RESPONSE:
         case SERVICE:
            assert(false);
            break;
         case UNKNOWN:
         case MAX_METHODS:
            assert(false);
            break;
      }
   }
}

void
DialogUsageManager::processResponse(const SipMessage& response)
{
   if (response.header(h_CSeq).method() != CANCEL)
   {
      DialogSet* ds = findDialogSet(DialogSetId(response));

      if (ds)
      {
         DebugLog ( << "DialogUsageManager::processResponse: " << std::endl << std::endl << response.brief());
         ds->dispatch(response);
      }
      else
      {
		 InfoLog (<< "Throwing away stray response: " << std::endl << std::endl << response.brief());
      }
   }
}

void
DialogUsageManager::processPublish(const SipMessage& request)
{
   if (!checkEventPackage(request))
   {
      InfoLog(<< "Rejecting request (unsupported package) " << request.brief());
      return;
   }

   if (request.exists(h_SIPIfMatch))
   {
      ServerPublications::iterator i = mServerPublications.find(request.header(h_SIPIfMatch).value());
      if (i != mServerPublications.end())
      {
         i->second->dispatch(request);
      }
      else
      {
         SharedPtr<SipMessage> response(new SipMessage);
         makeResponse(*response, request, 412);
         send(response);
      }
   }
   else
   {
      Data etag = Random::getCryptoRandom(8);
      while (mServerPublications.find(etag) != mServerPublications.end())
      {
         etag = Random::getCryptoRandom(8);
      }

      ServerPublication* sp = new ServerPublication(*this, etag, request);
      mServerPublications[etag] = sp;
      sp->dispatch(request);
   }
}

bool
DialogUsageManager::checkEventPackage(const SipMessage& request)
{
   int failureCode = 0;
   MethodTypes method = request.header(h_RequestLine).method();

//       || (method == NOTIFY && !request.exists(h_SubscriptionState)))

   if (!request.exists(h_Event))
   {
      InfoLog (<< "No Event header in " << request.header(h_RequestLine).unknownMethodName());
      failureCode = 400;
   }
   else
   {
      switch(method)
      {
         case SUBSCRIBE:
            if (!getServerSubscriptionHandler(request.header(h_Event).value()))
            {
               InfoLog (<< "No handler for event package for SUBSCRIBE: " 
                        << request.header(h_Event).value());
               failureCode = 489;
            }
            break;
         case NOTIFY:
            if (!getClientSubscriptionHandler(request.header(h_Event).value()))
            {
               InfoLog (<< "No handler for event package for NOTIFY: " 
                        << request.header(h_Event).value());
               failureCode = 489;
            }
            break;
         case PUBLISH:
            if (!getServerPublicationHandler(request.header(h_Event).value()))
            {
               InfoLog (<< "No handler for event package for PUBLISH: " 
                        << request.header(h_Event).value());
               failureCode = 489;
            }
            break;
         default:
            assert(0);
      }
   }

   if (failureCode > 0)
   {
      SharedPtr<SipMessage> response(new SipMessage);
      makeResponse(*response, request, failureCode);
      send(response);
      return false;
   }
   return true;
}

DialogSet*
DialogUsageManager::findDialogSet(const DialogSetId& id)
{
   StackLog ( << "Looking for dialogSet: " << id << " in map:" );
   StackLog ( << Inserter(mDialogSetMap) );
   DialogSetMap::const_iterator it = mDialogSetMap.find(id);

   if (it == mDialogSetMap.end())
   {
      return 0;
   }
   else
   {
      if(it->second->isDestroying())
      {
         return 0;
      }
      else
      {
         return it->second;
      }
   }
}

BaseCreator*
DialogUsageManager::findCreator(const DialogId& id)
{
   DialogSet* ds = findDialogSet(id.getDialogSetId());
   if (ds)
   {
      return ds->getCreator();
   }
   else
   {
      return 0;
   }
}

void
DialogUsageManager::removeDialogSet(const DialogSetId& dsId)
{
   StackLog ( << "************* Removing DialogSet ***************" );
   StackLog ( << "Before: " << Inserter(mDialogSetMap) );
   mDialogSetMap.erase(dsId);
   StackLog ( << "After: " << Inserter(mDialogSetMap) );
   if (mRedirectManager.get())
   {
      mRedirectManager->removeDialogSet(dsId);
   }
}

ClientSubscriptionHandler*
DialogUsageManager::getClientSubscriptionHandler(const Data& eventType)
{
   map<Data, ClientSubscriptionHandler*>::iterator res = mClientSubscriptionHandlers.find(eventType);
   if (res != mClientSubscriptionHandlers.end())
   {
      return res->second;
   }
   else
   {
      return 0;
   }
}

ServerSubscriptionHandler*
DialogUsageManager::getServerSubscriptionHandler(const Data& eventType)
{
   map<Data, ServerSubscriptionHandler*>::iterator res = mServerSubscriptionHandlers.find(eventType);
   if (res != mServerSubscriptionHandlers.end())
   {
      return res->second;
   }
   else
   {
      return 0;
   }
}

ClientPublicationHandler*
DialogUsageManager::getClientPublicationHandler(const Data& eventType)
{
   map<Data, ClientPublicationHandler*>::iterator res = mClientPublicationHandlers.find(eventType);
   if (res != mClientPublicationHandlers.end())
   {
      return res->second;
   }
   else
   {
      return 0;
   }
}

ServerPublicationHandler*
DialogUsageManager::getServerPublicationHandler(const Data& eventType)
{
   map<Data, ServerPublicationHandler*>::iterator res = mServerPublicationHandlers.find(eventType);
   if (res != mServerPublicationHandlers.end())
   {
      return res->second;
   }
   else
   {
      return 0;
   }
}

OutOfDialogHandler*
DialogUsageManager::getOutOfDialogHandler(const MethodTypes type)
{
   map<MethodTypes, OutOfDialogHandler*>::iterator res = mOutOfDialogHandlers.find(type);
   if (res != mOutOfDialogHandlers.end())
   {
      return res->second;
   }
   else
   {
      return 0;
   }
}

void 
DialogUsageManager::addIncomingFeature(resip::SharedPtr<DumFeature> feat)
{
   mIncomingFeatureList.push_back(feat);
}

void
DialogUsageManager::addOutgoingFeature(resip::SharedPtr<DumFeature> feat)
{
   // make sure EncryptionManager is the last feature in the list.
   mOutgoingFeatureList.insert(mOutgoingFeatureList.begin(), feat);
}

void
DialogUsageManager::setOutgoingMessageInterceptor(SharedPtr<DumFeature> feat)
{
   mOutgoingMessageInterceptor = feat;
}

void
DialogUsageManager::applyToAllServerSubscriptions(ServerSubscriptionFunctor* functor)
{
   assert(functor);
   for (DialogSetMap::iterator it = mDialogSetMap.begin(); it != mDialogSetMap.end(); ++it)
   {
      for (DialogSet::DialogMap::iterator i = it->second->mDialogs.begin(); i != it->second->mDialogs.end(); ++i)
      {
         std::vector<ServerSubscriptionHandle> serverSubs = i->second->getServerSubscriptions();
         for (std::vector<ServerSubscriptionHandle>::iterator iss = serverSubs.begin(); iss != serverSubs.end(); ++iss)
         {
            functor->apply(*iss);
         }
      }
   }
}

void
DialogUsageManager::applyToAllClientSubscriptions(ClientSubscriptionFunctor* functor)
{
   assert(functor);
   for (DialogSetMap::iterator it = mDialogSetMap.begin(); it != mDialogSetMap.end(); ++it)
   {
      for (DialogSet::DialogMap::iterator i = it->second->mDialogs.begin(); i != it->second->mDialogs.end(); ++i)
      {
         std::vector<ClientSubscriptionHandle> clientSubs = i->second->getClientSubscriptions();
         for (std::vector<ClientSubscriptionHandle>::iterator ics = clientSubs.begin(); ics != clientSubs.end(); ++ics)
         {
            functor->apply(*ics);
         }
      }
   }
}

void
DialogUsageManager::registerForConnectionTermination(Postable* listener)
{
   mConnectionTerminatedEventDispatcher.addListener(listener);
}

void
DialogUsageManager::unRegisterForConnectionTermination(Postable* listener)
{
   mConnectionTerminatedEventDispatcher.removeListener(listener);
}

void
DialogUsageManager::requestMergedRequestRemoval(const MergedRequestKey& key)
{
   DebugLog(<< "Got merged request removal request");
   MergedRequestRemovalCommand command(*this, key);
   mStack.postMS(command, Timer::TF, this);
}

void
DialogUsageManager::removeMergedRequest(const MergedRequestKey& key)
{
   DebugLog(<< "Merged request removed");
   mMergedRequests.erase(key);
}

TargetCommand::Target& 
DialogUsageManager::dumIncomingTarget() 
{
   return *mIncomingTarget;
}

TargetCommand::Target& 
DialogUsageManager::dumOutgoingTarget() 
{
   return *mOutgoingTarget;
}


/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * ====================================================================
 *
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
