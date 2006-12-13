#include "resiprocate/Security.hxx"
#include "resiprocate/SecurityAttributes.hxx"
#include "resiprocate/ShutdownMessage.hxx"
#include "resiprocate/SipFrag.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/TransactionUserMessage.hxx"
#include "resiprocate/dum/AppDialog.hxx"
#include "resiprocate/dum/AppDialogSet.hxx"
#include "resiprocate/dum/AppDialogSetFactory.hxx"
#include "resiprocate/dum/BaseUsage.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"
#include "resiprocate/dum/ClientOutOfDialogReq.hxx"
#include "resiprocate/dum/ClientPagerMessage.hxx"
#include "resiprocate/dum/ClientPublication.hxx"
#include "resiprocate/dum/ClientRegistration.hxx"
#include "resiprocate/dum/ClientSubscription.hxx"
#include "resiprocate/dum/DefaultServerReferHandler.hxx"
#include "resiprocate/dum/DestroyUsage.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/DumException.hxx"
#include "resiprocate/dum/DumShutdownHandler.hxx"
#include "resiprocate/dum/ExternalMessageHandler.hxx"
#include "resiprocate/dum/ExternalMessageBase.hxx"
#include "resiprocate/dum/InviteSessionCreator.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/KeepAliveManager.hxx"
#include "resiprocate/dum/KeepAliveTimeout.hxx"
#include "resiprocate/dum/MasterProfile.hxx"
#include "resiprocate/dum/OutOfDialogReqCreator.hxx"
#include "resiprocate/dum/PagerMessageCreator.hxx"
#include "resiprocate/dum/PublicationCreator.hxx"
#include "resiprocate/dum/RedirectManager.hxx"
#include "resiprocate/dum/RegistrationCreator.hxx"
#include "resiprocate/dum/ServerAuthManager.hxx"
#include "resiprocate/dum/ServerInviteSession.hxx"
#include "resiprocate/dum/ServerPublication.hxx"
#include "resiprocate/dum/ServerSubscription.hxx"
#include "resiprocate/dum/SubscriptionCreator.hxx"
#include "resiprocate/dum/SubscriptionHandler.hxx"
#include "resiprocate/dum/UserAuthInfo.hxx"
#include "resiprocate/dum/InternalDumAsyncMessageBase.hxx"
#include "resiprocate/dum/InternalDumCancelOutgoingMessage.hxx"
#include "resiprocate/os/Inserter.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Random.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"
#include "resiprocate/os/RWMutex.hxx"
#include "resiprocate/external/HttpProvider.hxx"
#include "resiprocate/external/HttpGetMessage.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

DialogUsageManager::DialogUsageManager(SipStack& stack) :
   mMasterProfile(0),
   mRedirectManager(new RedirectManager()),
   mInviteSessionHandler(0),
   mClientRegistrationHandler(0),
   mServerRegistrationHandler(0),
   mRedirectHandler(0),
   mDialogSetHandler(0),
   mRegistrationPersistenceManager(0),
   mClientPagerMessageHandler(0),
   mServerPagerMessageHandler(0),
   mAppDialogSetFactory(new AppDialogSetFactory()),
   mStack(stack),
   mDumShutdownHandler(0),
   mShutdownState(Running)
{
   mStack.registerTransactionUser(*this);
   addServerSubscriptionHandler("refer", DefaultServerReferHandler::Instance());
}

DialogUsageManager::~DialogUsageManager()
{
   mShutdownState = Destroying;
   //InfoLog ( << "~DialogUsageManager" );

   if(!mDialogSetMap.empty())
   {
      InfoLog(<< "DialogUsageManager::mDialogSetMap has " << mDialogSetMap.size() << " DialogSets");
      DialogSetMap::const_iterator ds = mDialogSetMap.begin();
      for(; ds != mDialogSetMap.end(); ++ds)
      {
         InfoLog(<< "DialgSetId:" << ds->first);
         DialogSet::DialogMap::const_iterator   d = ds->second->mDialogs.begin();
         for(; d != ds->second->mDialogs.end(); ++d)
         {
            //const Dialog* p = &(d->second);
            InfoLog(<<"DialogId:" << d->first << ", " << *d->second);
         }
      }
   }

   while(!mDialogSetMap.empty())
   {
      DialogSet*  ds = mDialogSetMap.begin()->second;
      delete ds;
   }
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
   mStack.addTransport(protocol, port, version, ipInterface,
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
   if (mHandleMap.size() > 0)
   {
      WarningLog (<< "force shutdown ");
      dumpHandles();
      
      mDumShutdownHandler = h;
      //HandleManager::shutdown();  // clear out usages
      mShutdownState = ShutdownRequested;
      DialogUsageManager::onAllHandlesDestroyed();
   }
}

void DialogUsageManager::setAppDialogSetFactory(std::auto_ptr<AppDialogSetFactory> factory)
{
   mAppDialogSetFactory = factory;
}

MasterProfile*
DialogUsageManager::getMasterProfile()
{
   assert(mMasterProfile);
   return mMasterProfile;
}

void DialogUsageManager::setMasterProfile(MasterProfile* masterProfile)
{
   assert(!mMasterProfile);
   mMasterProfile = masterProfile;
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
DialogUsageManager::setServerAuthManager(std::auto_ptr<ServerAuthManager> manager)
{
   mServerAuthManager = manager;
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
DialogUsageManager::addTimer(DumTimeout::Type type, unsigned long duration,
                             BaseUsageHandle target, int cseq, int rseq)
{
   DumTimeout t(type, duration, target, cseq, rseq);
   mStack.post(t, duration, this);
}

void
DialogUsageManager::addTimerMs(DumTimeout::Type type, unsigned long duration,
                               BaseUsageHandle target, int cseq, int rseq)
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
   if (eventType == "refer")
   {
      delete mServerSubscriptionHandlers[eventType];
      mServerSubscriptionHandlers[eventType] = NULL;
   }
   else
   {
      assert(mServerSubscriptionHandlers.count(eventType) == 0);
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

void
DialogUsageManager::addExternalMessageHandler(ExternalMessageHandler* handler)
{
   std::vector<ExternalMessageHandler*>::iterator found = std::find(mExternalMessageHandlers.begin(), mExternalMessageHandlers.end(), handler);
   if (found == mExternalMessageHandlers.end())
   {
      mExternalMessageHandlers.push_back(handler);
   }
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

   DebugLog ( << "************* Adding DialogSet ***************" );
   DebugLog ( << "Before: " << Inserter(mDialogSetMap) );
   mDialogSetMap[ds->getId()] = ds;
   DebugLog ( << "After: " << Inserter(mDialogSetMap) );
   return ds;
}

SipMessage&
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
DialogUsageManager::sendResponse(SipMessage& response)
{
   assert(response.isResponse());
   mStack.send(response, this);
}

SipMessage&
DialogUsageManager::makeInviteSession(const NameAddr& target, UserProfile& userProfile, const SdpContents* initialOffer, AppDialogSet* appDs)
{
   SipMessage& inv = makeNewSession(new InviteSessionCreator(*this, target, userProfile, initialOffer), appDs);
   return inv;
}

SipMessage&
DialogUsageManager::makeInviteSession(const NameAddr& target, const SdpContents* initialOffer, AppDialogSet* appDs)
{
   SipMessage& inv = makeNewSession(new InviteSessionCreator(*this, target, *getMasterProfile(), initialOffer), appDs);
   return inv;
}

SipMessage&
DialogUsageManager::makeInviteSessionFromRefer(const SipMessage& refer,
                                               ServerSubscriptionHandle serverSub,
                                               const SdpContents* initialOffer,
                                               AppDialogSet* appDs)
{
   //generate and send 100
   SipFrag contents;
   contents.message().header(h_StatusLine).statusCode() = 100;
   contents.message().header(h_StatusLine).reason() = "Trying";
   //will be cloned...ServerSub may not have the most efficient API possible
   serverSub->setSubscriptionState(Active);
   SipMessage& notify = serverSub->update(&contents);
//   mInviteSessionHandler->onReadyToSend(InviteSessionHandle::NotValid(), notify);
   serverSub->sendAsync(notify);

   //19.1.5
   NameAddr target = refer.header(h_ReferTo);
   target.uri().removeEmbedded();
   target.uri().remove(p_method);

   // !jf! this code assumes you have a UserProfile
   SipMessage& inv = makeNewSession(new InviteSessionCreator(*this,
                                                             target,
                                                             *serverSub->mDialog.mDialogSet.getUserProfile(),
                                                             initialOffer, serverSub), appDs);

   //could pass dummy target, then apply merge rules from 19.1.5...or
   //makeNewSession would use rules from 19.1.5
   if (refer.exists(h_ReferredBy))
   {
      inv.header(h_ReferredBy) = refer.header(h_ReferredBy);
   }

   const Uri& referTo = refer.header(h_ReferTo).uri();
   //19.1.5
   if (referTo.hasEmbedded() && referTo.embedded().exists(h_Replaces))
   {
      inv.header(h_Replaces) = referTo.embedded().header(h_Replaces);
   }
   return inv;
}


SipMessage&
DialogUsageManager::makeSubscription(const NameAddr& target, UserProfile& userProfile, const Data& eventType, AppDialogSet* appDs)
{
   return makeNewSession(new SubscriptionCreator(*this, target, userProfile, eventType, userProfile.getDefaultSubscriptionTime()), appDs);
}

SipMessage&
DialogUsageManager::makeSubscription(const NameAddr& target, UserProfile& userProfile, const Data& eventType,
                                     int subscriptionTime, AppDialogSet* appDs)
{
   return makeNewSession(new SubscriptionCreator(*this, target, userProfile, eventType, subscriptionTime), appDs);
}

SipMessage&
DialogUsageManager::makeSubscription(const NameAddr& target, UserProfile& userProfile, const Data& eventType,
                                     int subscriptionTime, int refreshInterval, AppDialogSet* appDs)
{
   return makeNewSession(new SubscriptionCreator(*this, target, userProfile, eventType, subscriptionTime, refreshInterval), appDs);
}

SipMessage&
DialogUsageManager::makeSubscription(const NameAddr& target, const Data& eventType, AppDialogSet* appDs)
{
   return makeNewSession(new SubscriptionCreator(*this, target, *getMasterProfile(), eventType, getMasterProfile()->getDefaultSubscriptionTime()), appDs);
}

SipMessage&
DialogUsageManager::makeSubscription(const NameAddr& target, const Data& eventType,
                                     int subscriptionTime, AppDialogSet* appDs)
{
   return makeNewSession(new SubscriptionCreator(*this, target, *getMasterProfile(), eventType, subscriptionTime), appDs);
}

SipMessage&
DialogUsageManager::makeSubscription(const NameAddr& target, const Data& eventType,
                                     int subscriptionTime, int refreshInterval, AppDialogSet* appDs)
{
   return makeNewSession(new SubscriptionCreator(*this, target, *getMasterProfile(), eventType, subscriptionTime, refreshInterval), appDs);
}

SipMessage&
DialogUsageManager::makeRegistration(const NameAddr& target, UserProfile& userProfile, AppDialogSet* appDs)
{
   return makeNewSession(new RegistrationCreator(*this, target, userProfile, userProfile.getDefaultRegistrationTime()), appDs);
}

SipMessage&
DialogUsageManager::makeRegistration(const NameAddr& target, UserProfile& userProfile, int registrationTime, AppDialogSet* appDs)
{
   return makeNewSession(new RegistrationCreator(*this, target, userProfile, registrationTime), appDs);
}

SipMessage&
DialogUsageManager::makeRegistration(const NameAddr& target, AppDialogSet* appDs)
{
   return makeNewSession(new RegistrationCreator(*this, target, *getMasterProfile(), getMasterProfile()->getDefaultRegistrationTime()), appDs);
}

SipMessage&
DialogUsageManager::makeRegistration(const NameAddr& target, int registrationTime, AppDialogSet* appDs)
{
   return makeNewSession(new RegistrationCreator(*this, target, *getMasterProfile(), registrationTime), appDs);
}

SipMessage&
DialogUsageManager::makePublication(const NameAddr& targetDocument,
                                    UserProfile& userProfile,
                                    const Contents& body,
                                    const Data& eventType,
                                    unsigned expiresSeconds,
                                    AppDialogSet* appDs)
{
   return makeNewSession(new PublicationCreator(*this, targetDocument, userProfile, body, eventType, expiresSeconds), appDs);
}

SipMessage&
DialogUsageManager::makePublication(const NameAddr& targetDocument,
                                    const Contents& body,
                                    const Data& eventType,
                                    unsigned expiresSeconds,
                                    AppDialogSet* appDs)
{
   return makeNewSession(new PublicationCreator(*this, targetDocument, *getMasterProfile(), body, eventType, expiresSeconds), appDs);
}

SipMessage&
DialogUsageManager::makeOutOfDialogRequest(const NameAddr& target, UserProfile& userProfile, const MethodTypes meth, AppDialogSet* appDs)
{
   return makeNewSession(new OutOfDialogReqCreator(*this, meth, target, userProfile), appDs);
}

SipMessage&
DialogUsageManager::makeOutOfDialogRequest(const NameAddr& target, const MethodTypes meth, AppDialogSet* appDs)
{
   return makeNewSession(new OutOfDialogReqCreator(*this, meth, target, *getMasterProfile()), appDs);
}

ClientPagerMessageHandle
DialogUsageManager::makePagerMessage(const NameAddr& target, UserProfile& userProfile, AppDialogSet* appDs)
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
   return makePagerMessage(target, *getMasterProfile(), appDs);
}

void
DialogUsageManager::send(SipMessage& msg)
{
   // !slg! There is probably a more efficient way to get the userProfile here (pass it in?)
   DialogSet* ds = findDialogSet(DialogSetId(msg));
   UserProfile* userProfile;
   if (ds == 0)
   {
      userProfile = getMasterProfile();
   }
   else
   {
      userProfile = ds->getUserProfile();
   }

   if (userProfile->hasUserAgent())
   {
      msg.header(h_UserAgent).value() = userProfile->getUserAgent();
   }

   DebugLog (<< "SEND:\n" << msg);
   if (msg.isRequest())
   {
      // We may not need to call reset() if makeRequest is always used.
      if (msg.header(h_RequestLine).method() != CANCEL &&
          msg.header(h_RequestLine).method() != ACK &&
          msg.exists(h_Vias))
      {
         msg.header(h_Vias).front().param(p_branch).reset();
      }

      if (msg.exists(h_Vias))
      {
         if(!userProfile->getRportEnabled())
         {
            msg.header(h_Vias).front().remove(p_rport);
         }
         int fixedTransportPort = userProfile->getFixedTransportPort();
         if(fixedTransportPort != 0)
         {
            msg.header(h_Vias).front().sentPort() = fixedTransportPort;
         }
      }

      if (mClientAuthManager.get() && msg.header(h_RequestLine).method() != ACK)
      {
         mClientAuthManager->addAuthentication(msg);
      }

      //copy message if processStrictRoute will modify it
      if (msg.exists(h_Routes) &&
          !msg.header(h_Routes).empty() &&
          !msg.header(h_Routes).front().uri().exists(p_lr))
      {
         SipMessage copyOfMessage(msg);
         Helper::processStrictRoute(copyOfMessage);
         sendUsingOutboundIfAppropriate(*userProfile, copyOfMessage);
      }
      else
      {
         sendUsingOutboundIfAppropriate(*userProfile, msg);
      }
   }
   else
   {
      sendResponse(msg);
   }
}

void
DialogUsageManager::sendUsingOutboundIfAppropriate(UserProfile& userProfile, SipMessage& msg)
{
   //a little inefficient, branch parameter might be better
   DialogId id(msg);
   if (userProfile.hasOutboundProxy() && !findDialog(id))
   {
      DebugLog ( << "Using outbound proxy");
      mStack.sendTo(msg, userProfile.getOutboundProxy().uri(), this);
   }
   else
   {
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
DialogUsageManager::cancelInvite(const DialogSetId& invSessionId)
{
   InfoLog(<< "Cancel outgoing INVITE (sync)");
   end(invSessionId);
}

void
DialogUsageManager::cancelInviteAsync(const DialogSetId& invSessionId)
{
   InfoLog(<< "Post cancel outgoing message (async)");
   post(new InternalDumCancelOutgoingMessage(invSessionId, *this));
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
   // !slg! Logic is here for now.
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

// !jf! When should this function return false?
void
DialogUsageManager::internalProcess(std::auto_ptr<Message> msg)
{
   // After a Stack ShutdownMessage has been received, don't do anything else in dum
   if (mShutdownState == Shutdown)
   {
      return;
   }

   bool authorized = false;
   if (mServerAuthManager.get())
   {
      UserAuthInfo* userAuth = dynamic_cast<UserAuthInfo*>(msg.get());
      if (userAuth)
      {
         Message* result = mServerAuthManager->handleUserAuthInfo(userAuth);
         if (result)
         {
            authorized = true;
            msg = std::auto_ptr<Message>(result);
         }
         else
         {
            InfoLog(<< "ServerAuth rejected request " << *userAuth);
            return;
         }
      }
   }

   try
   {
      InfoLog (<< "Got: " << msg->brief());

      //InfoLog(<< "Test if sip message" );
      SipMessage* sipMsg = dynamic_cast<SipMessage*>(msg.get());
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

            if (mServerAuthManager.get() && !authorized)
            {
               switch ( mServerAuthManager->handle(*sipMsg) )
               {
                  case ServerAuthManager::Challenged:
                     InfoLog(<< "ServerAuth challenged request " << sipMsg->brief());
                     return;
                  case ServerAuthManager::RequestedCredentials:
                     InfoLog(<< "ServerAuth requested credentials " << sipMsg->brief());
                     return;
                  case ServerAuthManager::Rejected:
                     InfoLog(<< "ServerAuth rejected request " << sipMsg->brief());
                     return;
                  default:
                     break;
               }
            }
      
            if (queueForIdentityCheck(sipMsg))
            {
               msg.release();
            }
            else
            {
               processRequest(*sipMsg);
            }
         }
         else
         {
            processResponse(*sipMsg);
         }
         return;
      }

      HttpGetMessage* httpMsg = dynamic_cast<HttpGetMessage*>(msg.get());
      if (httpMsg)
      {
         processIdentityCheckResponse(*httpMsg);         
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
         DebugLog(<< "Destroying usage" << *destroyUsage);
         destroyUsage->destroy();
         return;
      }

      DumTimeout* dumMsg = dynamic_cast<DumTimeout*>(msg.get());
      if (dumMsg)
      {
         InfoLog(<< "Timeout Message" );
         if (!dumMsg->getBaseUsage().isValid())
         {
            DebugLog(<< "BaseUsage Invalid, discard Timeout Message");
            return;
         }

         dumMsg->getBaseUsage()->dispatch(*dumMsg);
         return;
      }

      KeepAliveTimeout* keepAliveMsg = dynamic_cast<KeepAliveTimeout*>(msg.get());
      if (keepAliveMsg)
      {
         InfoLog(<< "Keep Alive Message" );
         if (mKeepAliveManager.get())
         {
            mKeepAliveManager->process(*keepAliveMsg);
         }
      }

      // !polo! internal messages for thread synchronization.
      InternalDumAsyncMessageBase* asyncMsgBase = dynamic_cast<InternalDumAsyncMessageBase*>(msg.get());
      if (asyncMsgBase)
      {
         asyncMsgBase->execute();
      }

      ExternalMessageBase* externalMessage = NULL;
      if (mExternalMessageHandlers.size() && (externalMessage = dynamic_cast<ExternalMessageBase*>(msg.get())))
      {
         processExternalMessage(externalMessage);
      }
   }
   catch(BaseException& e)
   {
      //unparseable, bad 403 w/ 2543 trans it from FWD, etc
	  ErrLog(<<"Illegal message rejected: " << e.getMessage());
   }
}

void
DialogUsageManager::processExternalMessage(ExternalMessageBase* externalMessage)
{
   bool handled = false;
   for(std::vector<ExternalMessageHandler*>::iterator i = mExternalMessageHandlers.begin(); 
      i != mExternalMessageHandlers.end(); ++i)
   {
      (*i)->onMessage(externalMessage, handled);
      if (handled)
      {
         break;
      }
   }
}

void
DialogUsageManager::processIdentityCheckResponse(const HttpGetMessage& msg)
{
#if defined(USE_SSL)
   InfoLog(<< "DialogUsageManager::processIdentityCheckResponse: " << msg.brief());   
   RequiresCerts::iterator it = mRequiresCerts.find(msg.tid());
   if (it != mRequiresCerts.end())
   {
      getSecurity()->checkAndSetIdentity( *it->second, msg.getBodyData() );
      InfoLog(<< "Passing msg onto processRequest: " << msg.brief());      
      processRequest(*it->second);
      delete it->second;
      mRequiresCerts.erase(it);
   }
#endif
}

bool
DialogUsageManager::queueForIdentityCheck(SipMessage* sipMsg)
{
#if defined(USE_SSL)
   if (sipMsg->exists(h_Identity) &&
       sipMsg->exists(h_IdentityInfo) &&
       sipMsg->exists(h_Date))
   {
      if (getSecurity()->hasDomainCert(sipMsg->header(h_From).uri().host()))
      {
         getSecurity()->checkAndSetIdentity(*sipMsg);
         return false;
      }
      else
      {
         if (!HttpProvider::instance())
         {
            return false;
         }
         
         try
         {
            mRequiresCerts[sipMsg->getTransactionId()] = sipMsg;
            InfoLog( << "Dum::queueForIdentityCheck, sending http request to: " 
                     << sipMsg->header(h_IdentityInfo));
            
            HttpProvider::instance()->get(sipMsg->header(h_IdentityInfo), 
                                          sipMsg->getTransactionId(),
                                          *this);
            return true;
         }
         catch (BaseException&)
         {
         }
      }
   }
#endif

   std::auto_ptr<SecurityAttributes> sec(new SecurityAttributes);
   sec->setIdentity(sipMsg->header(h_From).uri().getAor());
   sec->setIdentityStrength(SecurityAttributes::From);
   sipMsg->setSecurityAttributes(sec);
   return false;
}

bool
DialogUsageManager::hasEvents() const
{
   return mFifo.messageAvailable();
}

// return false if there is nothing to do at the moment
bool 
DialogUsageManager::process(bool block, resip::RWMutex* mutex)   // !polo! blocking process() so dum can be put to thread.
{
   bool hasNext = false;
   std::auto_ptr<Message> msg;
   do 
   {
      if(block)
      {
         msg.reset(mFifo.getNext());
         hasNext = (mFifo.size() != 0);
      }
      else
      {
         msg.reset(mFifo.getNext((bool&)hasNext));
      }
      if (msg.get())
      {
         if (mutex)
         {
            resip::Lock lock(*mutex); (void)lock;
            internalProcess(msg);
         }
         else
         {
            internalProcess(msg);
         }
      }
   } while (block && hasNext);
   return hasNext;
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
      if (mMergedRequests.count(MergedRequestKey(request)))
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

   assert(mAppDialogSetFactory.get());
   // !jf! note, the logic was reversed during ye great merge of March of Ought 5
   if (request.header(h_To).exists(p_tag) ||
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
         case NOTIFY : // handle unsolicited (illegal) NOTIFYs
            //.dcm. Illegal NOTIFY messages aren't handler through a
            //ClientSubscriptionHandler so this check will always fail unless
            //the corresponding event packacke is also installed
            //in-dialog. Taking it out for now.
//             if (!checkEventPackage(request))
//             {
//                InfoLog (<< "Rejecting request (unsupported package) " 
//                         << request.brief());
//                return;
//             }
            // no break
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
               assert(mDialogSetMap.find(id) == mDialogSetMap.end());
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

               DebugLog ( << "*********** Calling AppDialogSetFactory *************"  );
               AppDialogSet* appDs = mAppDialogSetFactory->createAppDialogSet(*this, request);
               assert(dset);
               appDs->mDialogSet = dset;
               dset->setUserProfile(appDs->selectUASUserProfile(request));
               assert(appDs);
               dset->mAppDialogSet = appDs;

               DebugLog ( << "************* Adding DialogSet ***************" );
               DebugLog ( << "Before: " << Inserter(mDialogSetMap) );
               mDialogSetMap[dset->getId()] = dset;
               DebugLog ( << "After: Req" << Inserter(mDialogSetMap) );

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
   DebugLog ( << "DialogUsageManager::processResponse:\n" << response);

   // !slg! if we do this, then stack may not shutdown if we are waiting for responses that are to tear down usages
   //if (mShutdownState != Running)
   //{
   //   InfoLog (<< "Ignoring a response since we are shutting down " << response.brief());
   //   return;
   //}

   if (response.header(h_CSeq).method() != CANCEL)
   {
      DialogSet* ds = findDialogSet(DialogSetId(response));

      if (ds)
      {
         DebugLog ( << "DialogUsageManager::processResponse: " << response.brief());
         ds->dispatch(response);
      }
      else
      {
         InfoLog (<< "Throwing away stray response: " << response.brief());
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
         SipMessage response;
         makeResponse(response, request, 412);
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
      failureCode = 400;
   }
   else
   {
      switch(method)
      {
         case SUBSCRIBE:
            if (!getServerSubscriptionHandler(request.header(h_Event).value()))
            {
               failureCode = 489;
            }
            break;
         case NOTIFY:
            if (!getClientSubscriptionHandler(request.header(h_Event).value()))
            {
               failureCode = 489;
            }
            break;
         case PUBLISH:
            if (!getServerPublicationHandler(request.header(h_Event).value()))
            {
               failureCode = 489;
            }
            break;
         default:
            assert(0);
      }
   }

   if (failureCode > 0)
   {
      SipMessage response;
      makeResponse(response, request, failureCode);
      send(response);
      return false;
   }
   return true;
}

DialogSet*
DialogUsageManager::findDialogSet(const DialogSetId& id)
{
   DebugLog ( << "Looking for dialogSet: " << id << " in map:" );
   DebugLog ( << Inserter(mDialogSetMap) );
   DialogSetMap::const_iterator it = mDialogSetMap.find(id);

   if (it == mDialogSetMap.end())
   {
      return 0;
   }
   else
   {
      return it->second;
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
   DebugLog ( << "************* Removing DialogSet ***************" );
   DialogSetMap::iterator found = mDialogSetMap.find(dsId);
   if (found != mDialogSetMap.end())
   {
      DebugLog ( << "Before: " << Inserter(mDialogSetMap) );
      mDialogSetMap.erase(dsId);
      DebugLog ( << "After: " << Inserter(mDialogSetMap) );
   }
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
