#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "UserAgent.hxx"
#include "UserAgentDialogSetFactory.hxx"
#include "UserAgentCmds.hxx"
#include "UserAgentServerAuthManager.hxx"
#include "UserAgentClientSubscription.hxx"
#include "UserAgentClientPublication.hxx"
#include "UserAgentRegistration.hxx"
#include "ReconSubsystem.hxx"

#include "reflow/FlowManagerSubsystem.hxx"

#include <reTurn/ReTurnSubsystem.hxx>

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/stack/ConnectionTerminated.hxx>
#include <resip/stack/Pidf.hxx>
#include <resip/stack/GenericPidfContents.hxx>
#include <resip/dum/ClientAuthManager.hxx>
#include <resip/dum/ClientSubscription.hxx>
#include <resip/dum/ServerSubscription.hxx>
#include <resip/dum/ClientPublication.hxx>
#include <resip/dum/ServerPublication.hxx>
#include <resip/dum/ClientRegistration.hxx>
#include <resip/dum/ClientPagerMessage.hxx>
#include <resip/dum/ServerPagerMessage.hxx>
#include <resip/stack/PlainContents.hxx>
#include <resip/dum/KeepAliveManager.hxx>
#include <resip/dum/AppDialogSet.hxx>
#if defined(USE_SSL)
#include <resip/stack/ssl/Security.hxx>
#endif
#include <rutil/WinLeakCheck.hxx>
#include <rutil/Random.hxx>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

UserAgent::UserAgent(ConversationManager* conversationManager, SharedPtr<UserAgentMasterProfile> profile, AfterSocketCreationFuncPtr socketFunc, SharedPtr<InstantMessage> instantMessage) : 
   mCurrentSubscriptionHandle(1),
   mCurrentConversationProfileHandle(1),
   mDefaultOutgoingConversationProfileHandle(0),
   mConversationManager(conversationManager),
   mProfile(profile),
   mInstantMessage(instantMessage),
#if defined(USE_SSL)
   mSecurity(new Security(profile->certPath())),
#else
   mSecurity(0),
#endif
   mStack(mSecurity, profile->getAdditionalDnsServers(), &mSelectInterruptor, false /* stateless */, socketFunc),
   mDum(mStack),
   mStackThread(mStack, mSelectInterruptor),
   mDumShutdown(false)
{
#if defined(USE_SSL)
   const std::vector<Data>& rootCertDirectories = mProfile->rootCertDirectories();
   std::vector<Data>::const_iterator ci = rootCertDirectories.begin();
   for(;ci != rootCertDirectories.end();ci++)
   {
      mSecurity->loadCADirectory(*ci);
   }
   const std::vector<Data>& rootCertBundles = mProfile->rootCertBundles();
   ci = rootCertBundles.begin();
   for(;ci != rootCertBundles.end();ci++)
   {
      mSecurity->loadCAFile(*ci);
   }
#endif
   resip_assert(mConversationManager);
   mConversationManager->setUserAgent(this);

   mStack.setTransportSipMessageLoggingHandler(profile->getTransportSipMessageLoggingHandler());
   mConversationManager->getFlowManager().setRTCPEventLoggingHandler(profile->getRTCPEventLoggingHandler());

   addTransports();

   // Set Enum Suffixes
   mStack.setEnumSuffixes(profile->getEnumSuffixes());

   // Enable/Disable Statistics Manager
   mStack.statisticsManagerEnabled() = profile->statisticsManagerEnabled();
   
   // Install Handlers
   mDum.setMasterProfile(mProfile);
   mDum.setClientRegistrationHandler(this);
   mDum.registerForConnectionTermination(this);
   mDum.setClientAuthManager(std::auto_ptr<ClientAuthManager>(new ClientAuthManager));
   mDum.setKeepAliveManager(std::auto_ptr<KeepAliveManager>(new KeepAliveManager));
   mDum.setRedirectHandler(mConversationManager);
   mDum.setInviteSessionHandler(mConversationManager); 
   mDum.setDialogSetHandler(mConversationManager);
   mDum.addOutOfDialogHandler(OPTIONS, mConversationManager);
   mDum.addOutOfDialogHandler(REFER, mConversationManager);
   mDum.addClientSubscriptionHandler("refer", mConversationManager);
   mDum.addServerSubscriptionHandler("refer", mConversationManager);

   // If application didn't create an IM object, we use a default one
   if(!mInstantMessage)
   {
      mInstantMessage = SharedPtr<InstantMessage>(new InstantMessage());
   }
   mDum.setServerPagerMessageHandler(mInstantMessage.get());
   mDum.setClientPagerMessageHandler(mInstantMessage.get());

   //mDum.addClientSubscriptionHandler(Symbols::Presence, this);
   //mDum.addClientPublicationHandler(Symbols::Presence, this);
   //mDum.addOutOfDialogHandler(NOTIFY, this);
   //mDum.addServerSubscriptionHandler("message-summary", this);

   // Set AppDialogSetFactory
   auto_ptr<AppDialogSetFactory> dsf(new UserAgentDialogSetFactory(*mConversationManager));
	mDum.setAppDialogSetFactory(dsf);

   // Set UserAgentServerAuthManager
   SharedPtr<ServerAuthManager> uasAuth( new UserAgentServerAuthManager(*this));
   mDum.setServerAuthManager(uasAuth);
}

UserAgent::~UserAgent()
{
   shutdown();
}

void
UserAgent::startup()
{
   mStackThread.run(); 
}

SubscriptionHandle 
UserAgent::getNewSubscriptionHandle()
{
   Lock lock(mSubscriptionHandleMutex);
   return mCurrentSubscriptionHandle++; 
}

void 
UserAgent::registerSubscription(UserAgentClientSubscription *subscription)
{
   mSubscriptions[subscription->getSubscriptionHandle()] = subscription;
}

void 
UserAgent::unregisterSubscription(UserAgentClientSubscription *subscription)
{
   mSubscriptions.erase(subscription->getSubscriptionHandle());
}

PublicationHandle
UserAgent::getNewPublicationHandle()
{
   Lock lock(mPublicationHandleMutex);
   return mCurrentPublicationHandle++;
}

void
UserAgent::registerPublication(UserAgentClientPublication *publication)
{
   mPublications[publication->getPublicationHandle()] = publication;
}

void
UserAgent::unregisterPublication(UserAgentClientPublication *publication)
{
   mPublications.erase(publication->getPublicationHandle());
}

ConversationProfileHandle 
UserAgent::getNewConversationProfileHandle()
{
   Lock lock(mConversationProfileHandleMutex);
   return mCurrentConversationProfileHandle++; 
}

void 
UserAgent::registerRegistration(UserAgentRegistration *registration)
{
   mRegistrations[registration->getConversationProfileHandle()] = registration;
}

void 
UserAgent::unregisterRegistration(UserAgentRegistration *registration)
{
   mRegistrations.erase(registration->getConversationProfileHandle());
}

void 
UserAgent::process(int timeoutMs)
{
   mDum.process(timeoutMs);
}

void
UserAgent::shutdown()
{
   UserAgentShutdownCmd* cmd = new UserAgentShutdownCmd(this);
   mDum.post(cmd);

   // Wait for Dum to shutdown
   while(!mDumShutdown) 
   {
      process(100);
   }

   mStackThread.shutdown();
   mStackThread.join();
}

void
UserAgent::logDnsCache()
{
   mStack.logDnsCache();
}

void 
UserAgent::clearDnsCache()
{
   mStack.clearDnsCache();
}

void 
UserAgent::post(ApplicationMessage& message, unsigned int ms)
{
   if(ms > 0)
   {
      mStack.postMS(message, ms, &mDum);
   }
   else
   {
      mDum.post(&message);
   }
}

void 
UserAgent::setLogLevel(Log::Level level, LoggingSubsystem subsystem)
{
   switch(subsystem)
   {
   case SubsystemAll:
      Log::setLevel(level);
      break;
   case SubsystemContents:
      Log::setLevel(level, Subsystem::CONTENTS);
      break;
   case SubsystemDns:
      Log::setLevel(level, Subsystem::DNS);
      break;
   case SubsystemDum:
      Log::setLevel(level, Subsystem::DUM);
      break;
   case SubsystemSdp:
      Log::setLevel(level, Subsystem::SDP);
      break;
   case SubsystemSip:
      Log::setLevel(level, Subsystem::SIP);
      break;
   case SubsystemTransaction:
      Log::setLevel(level, Subsystem::TRANSACTION);
      break;
   case SubsystemTransport:
      Log::setLevel(level, Subsystem::TRANSPORT);
      break;
   case SubsystemStats:
      Log::setLevel(level, Subsystem::STATS);
      break;
   case SubsystemRecon:
      Log::setLevel(level, ReconSubsystem::RECON);
      break;
   case SubsystemFlowManager:
      Log::setLevel(level, FlowManagerSubsystem::FLOWMANAGER);
      break;
   case SubsystemReTurn:
      Log::setLevel(level, ReTurnSubsystem::RETURN);
      break;
   }
}

ConversationProfileHandle 
UserAgent::addConversationProfile(SharedPtr<ConversationProfile> conversationProfile, bool defaultOutgoing)
{
   ConversationProfileHandle handle = getNewConversationProfileHandle();
   AddConversationProfileCmd* cmd = new AddConversationProfileCmd(this, handle, conversationProfile, defaultOutgoing);
   mDum.post(cmd);
   return handle;
}

void
UserAgent::setDefaultOutgoingConversationProfile(ConversationProfileHandle handle)
{
   SetDefaultOutgoingConversationProfileCmd* cmd = new SetDefaultOutgoingConversationProfileCmd(this, handle);
   mDum.post(cmd);
}

void 
UserAgent::destroyConversationProfile(ConversationProfileHandle handle)
{
   DestroyConversationProfileCmd* cmd = new DestroyConversationProfileCmd(this, handle);
   mDum.post(cmd);
}

SubscriptionHandle 
UserAgent::createSubscription(const Data& eventType, const NameAddr& target, unsigned int subscriptionTime, const Mime& mimeType)
{
   SubscriptionHandle handle = getNewSubscriptionHandle();
   CreateSubscriptionCmd* cmd = new CreateSubscriptionCmd(this, handle, eventType, target, subscriptionTime, mimeType);
   mDum.post(cmd);
   return handle;
}

void 
UserAgent::destroySubscription(SubscriptionHandle handle)
{
   DestroySubscriptionCmd* cmd = new DestroySubscriptionCmd(this, handle);
   mDum.post(cmd);
}

PublicationHandle
UserAgent::createPublication(const Data& eventType, const NameAddr& target, const Data& status, unsigned int publicationTime, const Mime& mimeType)
{
   PublicationHandle handle = getNewPublicationHandle();
   CreatePublicationCmd* cmd = new CreatePublicationCmd(this, handle, status, eventType, target, publicationTime, mimeType);
   mDum.post(cmd);

   return handle;   
}

void 
UserAgent::destroyPublication(PublicationHandle handle)
{
   DestroyPublicationCmd* cmd = new DestroyPublicationCmd(this, handle);
   mDum.post(cmd);
}

SharedPtr<ConversationProfile> 
UserAgent::getDefaultOutgoingConversationProfile()
{
   if(mDefaultOutgoingConversationProfileHandle != 0)
   {
      return mConversationProfiles[mDefaultOutgoingConversationProfileHandle];
   }
   else
   {
      resip_assert(false);
      ErrLog( << "getDefaultOutgoingConversationProfile: something is wrong - no profiles to return");
      return SharedPtr<ConversationProfile>((ConversationProfile*)0);
   }
}

SharedPtr<ConversationProfile>
UserAgent::getConversationProfileByMediaAddress(const resip::Data& mediaAddress)
{
   resip_assert(!mediaAddress.empty());

   ConversationProfileMap::iterator conIt;
   for(conIt = mConversationProfiles.begin(); conIt != mConversationProfiles.end(); conIt++)
   {
      SharedPtr<ConversationProfile> cp = conIt->second;
      if(cp->sessionCaps().session().origin().getAddress() == mediaAddress)
      {
         return cp;
      }
   }
   return SharedPtr<ConversationProfile>();
}

SharedPtr<ConversationProfile> 
UserAgent::getIncomingConversationProfile(const SipMessage& msg)
{
   resip_assert(msg.isRequest());

   // Examine the sip message, and select the most appropriate conversation profile

   // Check if request uri matches registration contact
   const Uri& requestUri = msg.header(h_RequestLine).uri();
   RegistrationMap::iterator regIt;
   for(regIt = mRegistrations.begin(); regIt != mRegistrations.end(); regIt++)
   {
      const NameAddrs& contacts = regIt->second->getContactAddresses();
      NameAddrs::const_iterator naIt;
      for(naIt = contacts.begin(); naIt != contacts.end(); naIt++)
      {
         InfoLog( << "getIncomingConversationProfile: comparing requestUri=" << requestUri << " to contactUri=" << (*naIt).uri());
         if ((*naIt).uri() == requestUri &&             // uri's match 
             (!requestUri.exists(p_rinstance) ||        // and request Uri doesn't have rinstance parameter OR 
              ((*naIt).uri().exists(p_rinstance) && requestUri.param(p_rinstance) == (*naIt).uri().param(p_rinstance))))  // rinstance parameter matches
         {
            ConversationProfileMap::iterator conIt = mConversationProfiles.find(regIt->first);
            if(conIt != mConversationProfiles.end())
            {
               return conIt->second;
            }
         }
      }
   }

   // Check if To header matches default from
   Data toAor = msg.header(h_To).uri().getAor();
   ConversationProfileMap::iterator conIt;
   for(conIt = mConversationProfiles.begin(); conIt != mConversationProfiles.end(); conIt++)
   {
      InfoLog( << "getIncomingConversationProfile: comparing toAor=" << toAor << " to defaultFromAor=" << conIt->second->getDefaultFrom().uri().getAor());
      if(isEqualNoCase(toAor, conIt->second->getDefaultFrom().uri().getAor()))
      {
         return conIt->second;
      }
   }

   // If can't find any matches, then return the default outgoing profile
   InfoLog( << "getIncomingConversationProfile: no matching profile found, falling back to default outgoing profile");
   return getDefaultOutgoingConversationProfile();
}

SharedPtr<ConversationProfile>
UserAgent::getConversationProfileForRefer(const SipMessage& msg)
{
   // default behaviour (before this API method was added) was to simply
   // use the default outgoing conversation profile.  A more precise method
   // for profile selection may need to developed, it can also be
   // implemented in a subclass.
   return getDefaultOutgoingConversationProfile();
}

SharedPtr<UserAgentMasterProfile> 
UserAgent::getUserAgentMasterProfile()
{
   return mProfile;
}

DialogUsageManager& 
UserAgent::getDialogUsageManager()
{
   return mDum;
}

ConversationManager*
UserAgent::getConversationManager()
{
   return mConversationManager;
}

void 
UserAgent::onDumCanBeDeleted()
{
   mDumShutdown = true;
}

void 
UserAgent::post(resip::Message* pMsg)
{
    // This get's posted to from the Dum thread - so we have no threading concerns
    ConnectionTerminated* pTerminated = dynamic_cast<ConnectionTerminated*>(pMsg);
    if (pTerminated)
    {
        InfoLog(<< "ConnectionTerminated seen for " << pTerminated->getFlow()  << " refreshing registrations");

        // Iterate through registrations and see if any match this connection.  If so, then force an immediate
        // refresh so that we can detect server failure and re-register sooner.
        RegistrationMap::iterator regIt;
        for (regIt = mRegistrations.begin(); regIt != mRegistrations.end(); regIt++)
        {
            if (regIt->second->getLastServerTuple().getFlowKey() == pTerminated->getFlow().getFlowKey())
            {
                regIt->second->forceRefresh();
            }
        }
    }
    delete pMsg;
}

void
UserAgent::addTransports()
{
   const std::vector<UserAgentMasterProfile::TransportInfo>& transports = mProfile->getTransports();
   std::vector<UserAgentMasterProfile::TransportInfo>::const_iterator i;
   for(i = transports.begin(); i != transports.end(); i++)
   {
      const UserAgentMasterProfile::TransportInfo& ti = *i;
      try
      {
#ifdef USE_SSL
         // Make sure certificate material available before trying to instantiate Transport
         if(isSecure(ti.mProtocol))
         {
            // FIXME: see comments about repro / CertificatePath
            if(!ti.mTlsCertificate.empty())
            {
               mSecurity->addDomainCertPEM(ti.mSipDomainname, Data::fromFile(ti.mTlsCertificate));
            }
            if(!ti.mTlsPrivateKey.empty())
            {
               mSecurity->addDomainPrivateKeyPEM(ti.mSipDomainname, Data::fromFile(ti.mTlsPrivateKey), ti.mTlsPrivateKeyPassPhrase);
            }
         }
#endif
         Transport *t = mStack.addTransport(ti.mProtocol,
                                 ti.mPort,
                                 ti.mIPVersion,
                                 StunEnabled,
                                 ti.mIPInterface,       // interface to bind to
                                 ti.mSipDomainname,
                                 ti.mTlsPrivateKeyPassPhrase,  // private key passphrase
                                 ti.mSslType, // sslType
                                 0,            // transport flags
                                 ti.mTlsCertificate, ti.mTlsPrivateKey,
                                 ti.mCvm,          // tls client verification mode
                                 ti.mUseEmailAsSIP);

         if (t)
         {
            int rcvBufLen = ti.mRcvBufLen;
            if (rcvBufLen >0 )
            {
#if defined(RESIP_SIPSTACK_HAVE_FDPOLL)
               // this new method is part of the epoll changeset,
               // which isn't commited yet.
               t->setRcvBufLen(rcvBufLen);
#else
               resip_assert(0);
#endif
            }
         }
      }
      catch (BaseException& e)
      {
         WarningLog (<< "Caught: " << e);
         WarningLog (<< "Failed to add " << Tuple::toData(ti.mProtocol) << " transport on " << ti.mPort);
      }
   }
}

void 
UserAgent::startApplicationTimer(unsigned int timerId, unsigned int durationMs, unsigned int seqNumber)
{
   UserAgentTimeout t(*this, timerId, durationMs, seqNumber);
   post(t, durationMs);
}

void 
UserAgent::onApplicationTimer(unsigned int timerId, unsigned int durationMs, unsigned int seqNumber)
{
   // Default implementation is to do nothing - application should override this
}

void 
UserAgent::onSubscriptionTerminated(SubscriptionHandle handle, unsigned int statusCode)
{
   // Default implementation is to do nothing - application should override this
}

void 
UserAgent::onSubscriptionNotify(SubscriptionHandle handle, const Data& notifyData)
{
   // Default implementation is to do nothing - application should override this
}

const char*
UserAgent::sendMessage(const NameAddr& destination, const Data& msg, const Mime& mimeType)
{
   if(!mDum.getMasterProfile()->isMethodSupported(MESSAGE))
   {
      WarningLog (<< "MESSAGE method not detected in list of supported methods, adding it belatedly" );
      mDum.getMasterProfile()->addSupportedMethod(MESSAGE);
   }
   
   ClientPagerMessageHandle cpmh = mDum.makePagerMessage(destination);
   auto_ptr<Contents> msgContent(new PlainContents(msg, mimeType));
   cpmh.get()->page(msgContent);
   SharedPtr<SipMessage> sipMessage = cpmh.get()->getMessageRequestSharedPtr();
   mDum.send(sipMessage);

   return sipMessage->header(h_CallId).value().c_str();
}

void 
UserAgent::shutdownImpl()
{
   mDum.shutdown(this);

   // End all subscriptions
   SubscriptionMap tempSubs = mSubscriptions;  // Create copy for safety, since ending Subscriptions can immediately remove themselves from map
   SubscriptionMap::iterator i;
   for(i = tempSubs.begin(); i != tempSubs.end(); i++)
   {
      i->second->end();
   }

   // Unregister all registrations
   RegistrationMap tempRegs = mRegistrations;  // Create copy for safety, since ending can immediately remove themselves from map
   RegistrationMap::iterator j;
   for(j = tempRegs.begin(); j != tempRegs.end(); j++)
   {
      j->second->end();
   }

   mConversationManager->shutdown();
}

void 
UserAgent::addConversationProfileImpl(ConversationProfileHandle handle, SharedPtr<ConversationProfile> conversationProfile, bool defaultOutgoing)
{
   // Store new profile
   mConversationProfiles[handle] = conversationProfile;
   conversationProfile->setHandle(handle);

#ifdef USE_SSL
   // If this is the first profile ever set - then use the aor defined in it as the aor used in 
   // the DTLS certificate for the DtlsFactory - TODO - improve this sometime so that we can change the aor in 
   // the cert at runtime to equal the aor in the default conversation profile
   if(!mDefaultOutgoingConversationProfileHandle)
   {
      mConversationManager->getFlowManager().initializeDtlsFactory(conversationProfile->getDefaultFrom().uri().getAor().c_str());
   }
#endif

   // Set the default outgoing if requested to do so, or we don't have one yet
   if(defaultOutgoing || mDefaultOutgoingConversationProfileHandle == 0)
   {
      setDefaultOutgoingConversationProfileImpl(handle);
   }

   // Register new profile
   if(conversationProfile->getDefaultRegistrationTime() != 0)
   {
      UserAgentRegistration *registration = new UserAgentRegistration(*this, mDum, handle);
      mDum.send(mDum.makeRegistration(conversationProfile->getDefaultFrom(), conversationProfile, registration));
   }
}

void 
UserAgent::setDefaultOutgoingConversationProfileImpl(ConversationProfileHandle handle)
{
   mDefaultOutgoingConversationProfileHandle = handle;
}

void 
UserAgent::destroyConversationProfileImpl(ConversationProfileHandle handle)
{
   // Remove matching registration if found
   RegistrationMap::iterator it = mRegistrations.find(handle);
   if(it != mRegistrations.end())
   {
      it->second->end();
   }

   // Remove from ConversationProfile map
   mConversationProfiles.erase(handle);

   // If this Conversation Profile was the default - select the first item in the map as the new default
   if(handle == mDefaultOutgoingConversationProfileHandle)
   {
      ConversationProfileMap::iterator it = mConversationProfiles.begin();
      if(it != mConversationProfiles.end())
      {
         setDefaultOutgoingConversationProfileImpl(it->first);
      }
      else
      {
         setDefaultOutgoingConversationProfileImpl(0);
      }
   }
}

void 
UserAgent::createSubscriptionImpl(SubscriptionHandle handle, const Data& eventType, const NameAddr& target, unsigned int subscriptionTime, const Mime& mimeType)
{
   // Ensure we have a client subscription handler for this event type
   if(!mDum.getClientSubscriptionHandler(eventType))
   {
      mDum.addClientSubscriptionHandler(eventType, this);
   }
   // Ensure that the request Mime type is supported in the dum profile
   if(!mProfile->isMimeTypeSupported(NOTIFY, mimeType))
   {
      mProfile->addSupportedMimeType(NOTIFY, mimeType);  
   }

   UserAgentClientSubscription *subscription = new UserAgentClientSubscription(*this, mDum, handle);
   mDum.send(mDum.makeSubscription(target, getDefaultOutgoingConversationProfile(), eventType, subscriptionTime, subscription));
}

void 
UserAgent::destroySubscriptionImpl(SubscriptionHandle handle)
{
   SubscriptionMap::iterator it = mSubscriptions.find(handle);
   if(it != mSubscriptions.end())
   {
      it->second->end();
   }
}

void 
UserAgent::createPublicationImpl(PublicationHandle handle, const Data& status, const Data& eventType, const NameAddr& target, unsigned int publicationTime, const Mime& mimeType)
{
   // Ensure we have a client publication handler for this event type
   if(!mDum.getClientPublicationHandler(eventType))
   {
      mDum.addClientPublicationHandler(eventType, this);
   }

   Data note;
   if(status == "dnd")
   {
      note = Data("Busy (DND)");
   }
   else if(status == "available")
   {
      note = Data("Online");
   }
   else if(status == "away")
   {
      note = Data("Away");
   }

   // Adding rpid defined at RFC 4480 https://tools.ietf.org/html/rfc4480#page-7
   resip::GenericPidfContents gPidf;
   gPidf.setSimplePresenceTupleNode(Data(string("ID-") + Random::getRandomHex(3).c_str()), true, Data::Empty, note, target.uri().getAorNoReally(), Data(1.0));

   resip::GenericPidfContents::Node* dm = new resip::GenericPidfContents::Node();
   dm->mNamespacePrefix = Data("dm:");
   dm->mTag = "person";
   dm->mAttributes["id"] = Data(string("ID-") + Random::getRandomHex(3).c_str());

   resip::GenericPidfContents::Node* rpid = new resip::GenericPidfContents::Node();
   rpid->mNamespacePrefix = Data("rpid:");
   rpid->mTag = "activities";
   dm->mChildren.push_back(rpid);   

   if(status != "available")
   {
      resip::GenericPidfContents::Node* st = new resip::GenericPidfContents::Node();
      st->mNamespacePrefix = Data("rpid:");
      st->mTag = status;
      rpid->mChildren.push_back(st);
   }
   
   resip::GenericPidfContents::NodeList rootNodes = gPidf.getRootNodes();
   rootNodes.push_back(dm);
   gPidf.setRootNodes(rootNodes);

   gPidf.setEntity(target.uri());
   gPidf.addNamespace(Data("urn:ietf:params:xml:ns:pidf:data-model"), Data("dm"));
   gPidf.addNamespace(Data("urn:ietf:params:xml:ns:pidf:rpid"), Data("rpid"));

   DebugLog( << "generated final gPidf : " << endl << gPidf );
   
   UserAgentClientPublication *publication = new UserAgentClientPublication(*this, mDum, handle);
   mDum.send(mDum.makePublication(target, getDefaultOutgoingConversationProfile(), gPidf, eventType, publicationTime, publication));
}

void 
UserAgent::destroyPublicationImpl(PublicationHandle handle)
{
   PublicationMap::iterator it = mPublications.find(handle);
   if(it != mPublications.end())
   {
      it->second->end();
   }
}

////////////////////////////////////////////////////////////////////////////////
// Registration Handler ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
UserAgent::onSuccess(ClientRegistrationHandle h, const SipMessage& msg)
{
   dynamic_cast<UserAgentRegistration *>(h->getAppDialogSet().get())->onSuccess(h, msg);
}

void
UserAgent::onFailure(ClientRegistrationHandle h, const SipMessage& msg)
{
   dynamic_cast<UserAgentRegistration *>(h->getAppDialogSet().get())->onFailure(h, msg);
}

void
UserAgent::onRemoved(ClientRegistrationHandle h, const SipMessage&msg)
{
   dynamic_cast<UserAgentRegistration *>(h->getAppDialogSet().get())->onRemoved(h, msg);
}

int 
UserAgent::onRequestRetry(ClientRegistrationHandle h, int retryMinimum, const SipMessage& msg)
{
   return dynamic_cast<UserAgentRegistration *>(h->getAppDialogSet().get())->onRequestRetry(h, retryMinimum, msg);
}

////////////////////////////////////////////////////////////////////////////////
// ClientSubscriptionHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
UserAgent::onUpdatePending(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   dynamic_cast<UserAgentClientSubscription *>(h->getAppDialogSet().get())->onUpdatePending(h, msg, outOfOrder);
}

void
UserAgent::onUpdateActive(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   dynamic_cast<UserAgentClientSubscription *>(h->getAppDialogSet().get())->onUpdateActive(h, msg, outOfOrder);
}

void
UserAgent::onUpdateExtension(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   dynamic_cast<UserAgentClientSubscription *>(h->getAppDialogSet().get())->onUpdateExtension(h, msg, outOfOrder);
}

void
UserAgent::onTerminated(ClientSubscriptionHandle h, const SipMessage* msg)
{
   dynamic_cast<UserAgentClientSubscription *>(h->getAppDialogSet().get())->onTerminated(h, msg);
}

void
UserAgent::onNewSubscription(ClientSubscriptionHandle h, const SipMessage& msg)
{
   dynamic_cast<UserAgentClientSubscription *>(h->getAppDialogSet().get())->onNewSubscription(h, msg);
}

int 
UserAgent::onRequestRetry(ClientSubscriptionHandle h, int retryMinimum, const SipMessage& msg)
{
   return dynamic_cast<UserAgentClientSubscription *>(h->getAppDialogSet().get())->onRequestRetry(h, retryMinimum, msg);
}

////////////////////////////////////////////////////////////////////////////////
// ClientPublicationHandler ////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
UserAgent::onSuccess(ClientPublicationHandle h, const SipMessage& status)
{
   dynamic_cast<UserAgentClientPublication *>(h->getAppDialogSet().get())->onSuccess(h, status);
}

void
UserAgent::onRemove(ClientPublicationHandle h, const SipMessage& status)
{
   dynamic_cast<UserAgentClientPublication *>(h->getAppDialogSet().get())->onRemove(h, status);
}

int
UserAgent::onRequestRetry(ClientPublicationHandle h, int retrySeconds, const SipMessage& status)
{
   return dynamic_cast<UserAgentClientPublication *>(h->getAppDialogSet().get())->onRequestRetry(h, retrySeconds, status);
}

void
UserAgent::onFailure(ClientPublicationHandle h, const SipMessage& status)
{
   dynamic_cast<UserAgentClientPublication *>(h->getAppDialogSet().get())->onFailure(h, status);
}

/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 Copyright (c) 2016, SIP Spectrum, Inc.

 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
