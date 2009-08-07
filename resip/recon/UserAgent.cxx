#include "UserAgent.hxx"
#include "UserAgentCmds.hxx"
#include "UserAgentServerAuthManager.hxx"
#include "UserAgentClientSubscription.hxx"
#include "UserAgentRegistration.hxx"
#include "ConversationManager.hxx"
#include "ReconSubsystem.hxx"
#include "FlowManagerSubsystem.hxx"

#include <ReTurnSubsystem.hxx>

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/dum/ClientAuthManager.hxx>
#include <resip/dum/ClientSubscription.hxx>
#include <resip/dum/ServerSubscription.hxx>
#include <resip/dum/ClientRegistration.hxx>
#include <resip/dum/KeepAliveManager.hxx>
#include <resip/dum/AppDialogSet.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

UserAgent::UserAgent(SharedPtr<UserAgentMasterProfile> profile) : 
   mCurrentSubscriptionHandle(1),
   mRegManager(new RegistrationManager()),
   mProfile(profile),
#if defined(USE_SSL)
   mSecurity(new Security(profile->certPath())),
#else
   mSecurity(0),
#endif
   mStack(mSecurity, profile->getAdditionalDnsServers(), &mSelectInterruptor),
   mDum(new DialogUsageManager(mStack)),
   mStackThread(mStack, mSelectInterruptor),
   mDumShutdown(false)
{
   addTransports();

   // Set Enum Suffixes
   mStack.setEnumSuffixes(profile->getEnumSuffixes());

   // Enable/Disable Statistics Manager
   mStack.statisticsManagerEnabled() = profile->statisticsManagerEnabled();
   
   // Install Handlers
   mDum->setMasterProfile(mProfile);
   mDum->setClientRegistrationHandler(this);
   mDum->setClientAuthManager(std::auto_ptr<ClientAuthManager>(new ClientAuthManager));
   mDum->setKeepAliveManager(std::auto_ptr<KeepAliveManager>(new KeepAliveManager));

   // See Conversation Manager for remainder of (invite-related) dum initialization

   //mDum->addClientSubscriptionHandler(Symbols::Presence, this);
   //mDum->addClientPublicationHandler(Symbols::Presence, this);
   //mDum->addOutOfDialogHandler(NOTIFY, this);
   //mDum->addServerSubscriptionHandler("message-summary", this);

   // Initialize the application timers object with the dum
   mApplicationTimers = SharedPtr<ApplicationTimers>(new ApplicationTimers(mDum));
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

void 
UserAgent::process(int timeoutMs)
{
   mDum->process(timeoutMs);
}

void
UserAgent::shutdown()
{
   UserAgentShutdownCmd* cmd = new UserAgentShutdownCmd(this);
   mDum->post(cmd);

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

SharedPtr<ApplicationTimers>
UserAgent::getTimers()
{
   return mApplicationTimers;
}

SubscriptionHandle 
UserAgent::createSubscription(const Data& eventType, const NameAddr& target, unsigned int subscriptionTime, const Mime& mimeType)
{
   SubscriptionHandle handle = getNewSubscriptionHandle();
   CreateSubscriptionCmd* cmd = new CreateSubscriptionCmd(this, handle, eventType, target, subscriptionTime, mimeType);
   mDum->post(cmd);
   return handle;
}

void 
UserAgent::destroySubscription(SubscriptionHandle handle)
{
   DestroySubscriptionCmd* cmd = new DestroySubscriptionCmd(this, handle);
   mDum->post(cmd);
}

SharedPtr<UserAgentMasterProfile> 
UserAgent::getUserAgentMasterProfile()
{
   return mProfile;
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
UserAgent::addTransports()
{
   const std::vector<UserAgentMasterProfile::TransportInfo>& transports = mProfile->getTransports();
   std::vector<UserAgentMasterProfile::TransportInfo>::const_iterator i;
   for(i = transports.begin(); i != transports.end(); i++)
   {
      try
      {
         switch((*i).mProtocol)
         {
#ifdef USE_SSL
         case TLS:
#ifdef USE_DTLS
         case DTLS:
#endif
            mDum->addTransport((*i).mProtocol, (*i).mPort, (*i).mIPVersion, (*i).mIPInterface, (*i).mSipDomainname, Data::Empty, (*i).mSslType);
            break;
#endif
         case UDP:
         case TCP:
            mDum->addTransport((*i).mProtocol, (*i).mPort, (*i).mIPVersion, (*i).mIPInterface);
            break;
         default:
            WarningLog (<< "Failed to add " << Tuple::toData((*i).mProtocol) << " transport - unsupported type");
         }
      }
      catch (BaseException& e)
      {
         WarningLog (<< "Caught: " << e);
         WarningLog (<< "Failed to add " << Tuple::toData((*i).mProtocol) << " transport on " << (*i).mPort);
      }
   }
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

void 
UserAgent::shutdownImpl()
{
   mDum->shutdown(this);

   // End all subscriptions
   // Destroy each Conversation
   SubscriptionMap tempSubs = mSubscriptions;  // Create copy for safety, since ending Subscriptions can immediately remove themselves from map
   SubscriptionMap::iterator i;
   for(i = tempSubs.begin(); i != tempSubs.end(); i++)
   {
      i->second->end();
   }

   // Unregister all registrations
   mRegManager->unregisterAll();

   mConversationManager->shutdown();
}

void 
UserAgent::createSubscriptionImpl(SubscriptionHandle handle, const Data& eventType, const NameAddr& target, unsigned int subscriptionTime, const Mime& mimeType)
{
   // Ensure we have a client subscription handler for this event type
   if(!mDum->getClientSubscriptionHandler(eventType))
   {
      mDum->addClientSubscriptionHandler(eventType, this);
   }
   // Ensure that the request Mime type is supported in the dum profile
   if(!mProfile->isMimeTypeSupported(SUBSCRIBE, mimeType))
   {
      mProfile->addSupportedMimeType(SUBSCRIBE, mimeType);  
   }

   UserAgentClientSubscription *subscription = new UserAgentClientSubscription(*this, *mDum, handle);
   mDum->send(mDum->makeSubscription(target, mConversationManager->getDefaultOutgoingConversationProfile(), eventType, subscriptionTime, subscription));
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


/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
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
