// resip includes
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/Lock.hxx>
#include <rutil/Random.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/ClientInviteSession.hxx>
#include <resip/dum/ServerInviteSession.hxx>
#include <resip/dum/ClientSubscription.hxx>
#include <resip/dum/ServerOutOfDialogReq.hxx>
#include <resip/dum/ClientPagerMessage.hxx>
#include <resip/dum/ServerPagerMessage.hxx>

#include "ReconSubsystem.hxx"
#include "UserAgent.hxx"
#include "ConversationManager.hxx"
#include "ConversationManagerCmds.hxx"
#include "Conversation.hxx"
#include "Participant.hxx"
#include "BridgeMixer.hxx"
#include "DtmfEvent.hxx"
#include "RemoteParticipant.hxx"
#include "RemoteIMPagerParticipant.hxx"
#include "RemoteIMSessionParticipant.hxx"
#include "RemoteIMSessionParticipantDialogSet.hxx"
#include <rutil/WinLeakCheck.hxx>

#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4355 )
#endif

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

ConversationManager::ConversationManager(std::shared_ptr<MediaStackAdapter> mediaStackAdapter)
: mUserAgent(0),
  mMediaStackAdapter(mediaStackAdapter),
  mShuttingDown(false),
  mCurrentConversationHandle(1),
  mCurrentParticipantHandle(1),
  mBridgeMixer(0)
{
}

ConversationManager::~ConversationManager()
{
   resip_assert(mConversations.empty());
   resip_assert(mParticipants.empty());
   mBridgeMixer.reset();       // Make sure the mixer is destroyed before the media interface
}

void
ConversationManager::setUserAgent(UserAgent* userAgent)
{
   mUserAgent = userAgent;
   mMediaStackAdapter->setUserAgent(userAgent);
}

void
ConversationManager::shutdown()
{
   mShuttingDown = true;

   // Destroy each Conversation
   ConversationMap tempConvs = mConversations;  // Create copy for safety, since ending conversations can immediately remove themselves from map
   ConversationMap::iterator i;
   for(i = tempConvs.begin(); i != tempConvs.end(); i++)
   {
      InfoLog(<< "Destroying conversation: " << i->second->getHandle());
      i->second->destroy();
   }

   // End each Participant
   ParticipantMap tempParts = mParticipants;  
   ParticipantMap::iterator j;
   int j2=0;
   for(j = tempParts.begin(); j != tempParts.end(); j++, j2++)
   {
      InfoLog(<< "Destroying participant: " << j->second->getParticipantHandle());
      j->second->destroyParticipant();
   }
}

void
ConversationManager::process()
{
   mMediaStackAdapter->process();
}

ConversationHandle 
ConversationManager::createConversation(AutoHoldMode autoHoldMode)
{
   if (mShuttingDown) return 0;  // Don't allow new things to be created when we are shutting down
   ConversationHandle convHandle = getNewConversationHandle();

   CreateConversationCmd* cmd = new CreateConversationCmd(this, convHandle, autoHoldMode, 0);
   post(cmd);
   return convHandle;
}

void 
ConversationManager::destroyConversation(ConversationHandle convHandle)
{
   DestroyConversationCmd* cmd = new DestroyConversationCmd(this, convHandle);
   post(cmd);
}

void 
ConversationManager::joinConversation(ConversationHandle sourceConvHandle, ConversationHandle destConvHandle)
{
   JoinConversationCmd* cmd = new JoinConversationCmd(this, sourceConvHandle, destConvHandle);
   post(cmd);
}

ParticipantHandle
ConversationManager::createRemoteParticipant(ConversationHandle convHandle, const resip::NameAddr& destination, ParticipantForkSelectMode forkSelectMode, const std::shared_ptr<ConversationProfile>& conversationProfile, const std::multimap<resip::Data,resip::Data>& extraHeaders)
{
   if (mShuttingDown) return 0;  // Don't allow new things to be created when we are shutting down
   ParticipantHandle partHandle = getNewParticipantHandle();

   CreateRemoteParticipantCmd* cmd = new CreateRemoteParticipantCmd(this, partHandle, convHandle, destination, forkSelectMode, conversationProfile, extraHeaders);
   post(cmd);

   return partHandle;
}

ParticipantHandle 
ConversationManager::createRemoteIMPagerParticipant(const resip::NameAddr& destination, const std::shared_ptr<ConversationProfile>& conversationProfile)
{
   if (mShuttingDown) return 0;  // Don't allow new things to be created when we are shutting down
   ParticipantHandle partHandle = getNewParticipantHandle();

   CreateRemoteIMPagerParticipantCmd* cmd = new CreateRemoteIMPagerParticipantCmd(this, partHandle, destination, conversationProfile);
   post(cmd);

   return partHandle;
}

ParticipantHandle 
ConversationManager::createRemoteIMSessionParticipant(const resip::NameAddr& destination, ParticipantForkSelectMode forkSelectMode, const std::shared_ptr<ConversationProfile>& conversationProfile, const std::multimap<resip::Data, resip::Data>& extraHeaders)
{
   if (mShuttingDown) return 0;  // Don't allow new things to be created when we are shutting down
   ParticipantHandle partHandle = getNewParticipantHandle();

   CreateRemoteIMSessionParticipantCmd* cmd = new CreateRemoteIMSessionParticipantCmd(this, partHandle, destination, forkSelectMode, conversationProfile, extraHeaders);
   post(cmd);

   return partHandle;
}

ParticipantHandle 
ConversationManager::createMediaResourceParticipant(ConversationHandle convHandle, const Uri& mediaUrl)
{
   if (mShuttingDown) return 0;  // Don't allow new things to be created when we are shutting down
   ParticipantHandle partHandle = getNewParticipantHandle();

   CreateMediaResourceParticipantCmd* cmd = new CreateMediaResourceParticipantCmd(this, partHandle, convHandle, mediaUrl);
   post(cmd);

   return partHandle;
}

ParticipantHandle 
ConversationManager::createLocalParticipant()
{
   if (mShuttingDown) return 0;  // Don't allow new things to be created when we are shutting down
   ParticipantHandle partHandle = 0;
   if (getMediaStackAdapter().supportsLocalAudio())
   {
      partHandle = getNewParticipantHandle();

      CreateLocalParticipantCmd* cmd = new CreateLocalParticipantCmd(this, partHandle);
      post(cmd);
   }
   else
   {
      WarningLog(<< "createLocalParticipant called when local audio support is disabled.");
   }

   return partHandle;
}

void 
ConversationManager::destroyParticipant(ParticipantHandle partHandle)
{
   DestroyParticipantCmd* cmd = new DestroyParticipantCmd(this, partHandle);
   post(cmd);
}

void 
ConversationManager::addParticipant(ConversationHandle convHandle, ParticipantHandle partHandle)
{
   AddParticipantCmd* cmd = new AddParticipantCmd(this, convHandle, partHandle);
   post(cmd);
}

void 
ConversationManager::removeParticipant(ConversationHandle convHandle, ParticipantHandle partHandle)
{
   RemoveParticipantCmd* cmd = new RemoveParticipantCmd(this, convHandle, partHandle);
   post(cmd);
}

void
ConversationManager::moveParticipant(ParticipantHandle partHandle, ConversationHandle sourceConvHandle, ConversationHandle destConvHandle)
{
   MoveParticipantCmd* cmd = new MoveParticipantCmd(this, partHandle, sourceConvHandle, destConvHandle);
   post(cmd);
}

void 
ConversationManager::modifyParticipantContribution(ConversationHandle convHandle, ParticipantHandle partHandle, unsigned int inputGain, unsigned int outputGain)
{
   ModifyParticipantContributionCmd* cmd = new ModifyParticipantContributionCmd(this, convHandle, partHandle, inputGain, outputGain);
   post(cmd);
}

void 
ConversationManager::outputBridgeMatrix(ConversationHandle convHandle)
{
   OutputBridgeMixWeightsCmd* cmd = new OutputBridgeMixWeightsCmd(this, convHandle);
   post(cmd);
}

void 
ConversationManager::alertParticipant(ParticipantHandle partHandle, bool earlyFlag)
{
   AlertParticipantCmd* cmd = new AlertParticipantCmd(this, partHandle, earlyFlag);
   post(cmd);
}

void 
ConversationManager::answerParticipant(ParticipantHandle partHandle)
{
   AnswerParticipantCmd* cmd = new AnswerParticipantCmd(this, partHandle);
   post(cmd);
}

void 
ConversationManager::rejectParticipant(ParticipantHandle partHandle, unsigned int rejectCode)
{
   RejectParticipantCmd* cmd = new RejectParticipantCmd(this, partHandle, rejectCode);
   post(cmd);
}

void 
ConversationManager::redirectParticipant(ParticipantHandle partHandle, const NameAddr& destination, unsigned int redirectCode, RedirectSuccessCondition successCondition)
{
   RedirectParticipantCmd* cmd = new RedirectParticipantCmd(this, partHandle, destination, redirectCode, successCondition);
   post(cmd);
}

void 
ConversationManager::redirectToParticipant(ParticipantHandle partHandle, ParticipantHandle destPartHandle, RedirectSuccessCondition successCondition)
{
   RedirectToParticipantCmd* cmd = new RedirectToParticipantCmd(this, partHandle, destPartHandle, successCondition);
   post(cmd);
}

void
ConversationManager::holdParticipant(ParticipantHandle partHandle, bool hold)
{
   HoldParticipantCmd* cmd = new HoldParticipantCmd(this, partHandle, hold);
   post(cmd);
}

void 
ConversationManager::sendIMToParticipant(ParticipantHandle partHandle, std::unique_ptr<Contents> contents)
{
   SendIMToParticipantCmd* cmd = new SendIMToParticipantCmd(this, partHandle, std::move(contents));
   post(cmd);
}

void 
ConversationManager::startApplicationTimer(unsigned int timerId, unsigned int timerData1, unsigned int timerData2, unsigned int durationMs)
{
   ApplicationTimerCmd cmd(this, timerId, timerData1, timerData2);
   post(cmd, durationMs);
}

ConversationHandle 
ConversationManager::getNewConversationHandle()
{
   Lock lock(mConversationHandleMutex);
   return mCurrentConversationHandle++; 
}

void 
ConversationManager::registerConversation(Conversation *conversation)
{
   mConversations[conversation->getHandle()] = conversation;
}

void 
ConversationManager::unregisterConversation(Conversation *conversation)
{
   mConversations.erase(conversation->getHandle());
}

ParticipantHandle 
ConversationManager::getNewParticipantHandle()
{
   Lock lock(mParticipantHandleMutex);
   return mCurrentParticipantHandle++; 
}

void 
ConversationManager::registerParticipant(Participant *participant)
{
   mParticipants[participant->getParticipantHandle()] = participant;
}

void 
ConversationManager::unregisterParticipant(Participant *participant)
{
   InfoLog(<< "participant unregistered, handle=" << participant->getParticipantHandle());
   mParticipants.erase(participant->getParticipantHandle());
}

void 
ConversationManager::post(resip::Message *msg)
{
   if (mUserAgent)
   {
      mUserAgent->getDialogUsageManager().post(msg);
   }
}

void 
ConversationManager::post(resip::ApplicationMessage& message, unsigned int ms)
{
   if (mUserAgent)
   {
      mUserAgent->post(message, ms);
   }
}

void 
ConversationManager::buildSdpOffer(ConversationProfile* profile, SdpContents& offer)
{
   // copy over session capabilities
   offer = profile->sessionCaps();

   // Set sessionid and version for this offer
   UInt64 currentTime = Timer::getTimeMicroSec();
   offer.session().origin().getSessionId() = currentTime;
   offer.session().origin().getVersion() = currentTime;  

   // Set local port in offer
   // for now we only allow 1 audio media
   resip_assert(offer.session().media().size() == 1);
   resip_assert(offer.session().media().front().name() == "audio");
}

Participant* 
ConversationManager::getParticipant(ParticipantHandle partHandle)
{
   ParticipantMap::iterator i = mParticipants.find(partHandle);
   if(i != mParticipants.end())
   {
      return i->second;
   }
   else
   {
      return 0;
   }
}

Conversation* 
ConversationManager::getConversation(ConversationHandle convHandle)
{
   ConversationMap::iterator i = mConversations.find(convHandle);
   if(i != mConversations.end())
   {
      return i->second;
   }
   else
   {
      return 0;
   }
}

std::set<ConversationHandle>
ConversationManager::getConversations() const
{
   set<ConversationHandle> conversations;
   ConversationMap::const_iterator it;
   for(it = mConversations.begin(); it != mConversations.end(); it++)
   {
      conversations.insert(it->first);
   }
   return conversations;
}

void 
ConversationManager::addBufferToMediaResourceCache(const resip::Data& name, const resip::Data& buffer, int type)
{
   mMediaResourceCache.addToCache(name, buffer, type);
}

bool 
ConversationManager::getBufferFromMediaResourceCache(const resip::Data& name, resip::Data** buffer, int* type)
{
   return mMediaResourceCache.getFromCache(name, buffer, type);
}

void 
ConversationManager::notifyMediaEvent(ParticipantHandle partHandle, MediaEvent::MediaEventType eventType, MediaEvent::MediaDirection direction)
{
   Participant* participant = getParticipant(partHandle);
   if (participant)
   {
      if (eventType == MediaEvent::RESOURCE_DONE || eventType == MediaEvent::RESOURCE_FAILED)
      {
         MediaResourceParticipant* mrPart = dynamic_cast<MediaResourceParticipant*>(participant);
         if (mrPart)
         {
            if (eventType == MediaEvent::RESOURCE_FAILED)
            {
               onMediaResourceParticipantFailed(partHandle);
            }
            mrPart->resourceDone();
         }
      }
      else if (eventType == MediaEvent::VOICE_STARTED || eventType == MediaEvent::VOICE_STOPPED)
      {
         onParticipantVoiceActivity(partHandle, eventType == MediaEvent::VOICE_STARTED ? true : false, direction == MediaEvent::DIRECTION_IN);
      }
   }
}

void 
ConversationManager::notifyDtmfEvent(ParticipantHandle partHandle, int dtmf, int duration, bool up)
{
   // Call virtual method that applications can override
   onDtmfEvent(partHandle, dtmf, duration, up);
}

RemoteParticipant* 
ConversationManager::createAppropriateRemoteParticipantInstance(DialogUsageManager& dum, RemoteParticipantDialogSet& rpds)
{
   if (dynamic_cast<RemoteIMSessionParticipantDialogSet*>(&rpds) != nullptr)
   {
      return new RemoteIMSessionParticipant(*this, dum, rpds);
   }
   else
   {
      return getMediaStackAdapter().createRemoteParticipantInstance(dum, rpds);
   }
}

RemoteParticipant* 
ConversationManager::createAppropriateRemoteParticipantInstance(ParticipantHandle partHandle, DialogUsageManager& dum, RemoteParticipantDialogSet& rpds)
{
   if (dynamic_cast<RemoteIMSessionParticipantDialogSet*>(&rpds) != nullptr)
   {
      return new RemoteIMSessionParticipant(partHandle, *this, dum, rpds);
   }
   else
   {
      return getMediaStackAdapter().createRemoteParticipantInstance(partHandle, dum, rpds);
   }
}

RemoteParticipantDialogSet* 
ConversationManager::createRemoteIMSessionParticipantDialogSetInstance(ParticipantForkSelectMode forkSelectMode, std::shared_ptr<ConversationProfile> conversationProfile)
{
   return new RemoteIMSessionParticipantDialogSet(*this, forkSelectMode, conversationProfile);
}

void
ConversationManager::setMediaStackAdapter(std::shared_ptr<MediaStackAdapter> mediaStackAdapter)
{
   mMediaStackAdapter = mediaStackAdapter;
   if(mediaStackAdapter)
   {
      mediaStackAdapter->conversationManagerReady(this);
   }
}

void
ConversationManager::onNewSession(ClientInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onNewSession(h, oat, msg);
}

void
ConversationManager::onNewSession(ServerInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onNewSession(h, oat, msg);
}

void
ConversationManager::onFailure(ClientInviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onFailure(h, msg);
}
      
void
ConversationManager::onEarlyMedia(ClientInviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onEarlyMedia(h, msg, sdp);
}

void
ConversationManager::onProvisional(ClientInviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onProvisional(h, msg);
}

void
ConversationManager::onConnected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onConnected(h, msg);
}

void
ConversationManager::onConnected(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onConnected(h, msg);
}

void
ConversationManager::onConnectedConfirmed(InviteSessionHandle h, const SipMessage &msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onConnectedConfirmed(h, msg);
}

void
ConversationManager::onStaleCallTimeout(ClientInviteSessionHandle h)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onStaleCallTimeout(h);
}

void
ConversationManager::onTerminated(InviteSessionHandle h, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onTerminated(h, reason, msg);
}

void
ConversationManager::onRedirected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onRedirected(h, msg);
}

void
ConversationManager::onAnswer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onAnswer(h, msg, sdp);
}

void
ConversationManager::onOffer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{         
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onOffer(h, msg, sdp);
}

void
ConversationManager::onOfferRequired(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onOfferRequired(h, msg);
}

void
ConversationManager::onOfferRejected(InviteSessionHandle h, const SipMessage* msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onOfferRejected(h, msg);
}

void
ConversationManager::onOfferRequestRejected(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onOfferRequestRejected(h, msg);
}

void
ConversationManager::onRemoteSdpChanged(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onRemoteSdpChanged(h, msg, sdp);
}

void
ConversationManager::onInfo(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onInfo(h, msg);
}

void
ConversationManager::onInfoSuccess(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onInfoSuccess(h, msg);
}

void
ConversationManager::onInfoFailure(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onInfoFailure(h, msg);
}

void
ConversationManager::onRefer(InviteSessionHandle h, ServerSubscriptionHandle ssh, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onRefer(h, ssh, msg);
}

void
ConversationManager::onReferAccepted(InviteSessionHandle h, ClientSubscriptionHandle csh, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onReferAccepted(h, csh, msg);
}

void
ConversationManager::onReferRejected(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onReferRejected(h, msg);
}

void
ConversationManager::onReferNoSub(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onReferNoSub(h, msg);
}

void
ConversationManager::onMessage(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onMessage(h, msg);
}

void
ConversationManager::onMessageSuccess(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onMessageSuccess(h, msg);
}

void
ConversationManager::onMessageFailure(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onMessageFailure(h, msg);
}

void
ConversationManager::onForkDestroyed(ClientInviteSessionHandle h)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onForkDestroyed(h);
}

////////////////////////////////////////////////////////////////////////////////
// DialogSetHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
ConversationManager::onTrying(AppDialogSetHandle h, const SipMessage& msg)
{
   RemoteParticipantDialogSet *remoteParticipantDialogSet = dynamic_cast<RemoteParticipantDialogSet *>(h.get());
   if(remoteParticipantDialogSet)
   {
      remoteParticipantDialogSet->onTrying(h, msg);
   }
   else
   {
      InfoLog(<< "onTrying(AppDialogSetHandle): " << msg.brief());
   }
}

void 
ConversationManager::onNonDialogCreatingProvisional(AppDialogSetHandle h, const SipMessage& msg)
{
   RemoteParticipantDialogSet *remoteParticipantDialogSet = dynamic_cast<RemoteParticipantDialogSet *>(h.get());
   if(remoteParticipantDialogSet)
   {
      remoteParticipantDialogSet->onNonDialogCreatingProvisional(h, msg);
   }
   else
   {
      InfoLog(<< "onNonDialogCreatingProvisional(AppDialogSetHandle): " << msg.brief());
   }
}

////////////////////////////////////////////////////////////////////////////////
// ClientSubscriptionHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConversationManager::onUpdatePending(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onUpdatePending(h, msg, outOfOrder);
}

void
ConversationManager::onUpdateActive(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onUpdateActive(h, msg, outOfOrder);
}

void
ConversationManager::onUpdateExtension(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onUpdateExtension(h, msg, outOfOrder);
}

void
ConversationManager::onTerminated(ClientSubscriptionHandle h, const SipMessage* msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onTerminated(h, msg);
}

void
ConversationManager::onNewSubscription(ClientSubscriptionHandle h, const SipMessage& msg)
{
   dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onNewSubscription(h, msg);
}

int 
ConversationManager::onRequestRetry(ClientSubscriptionHandle h, int retryMinimum, const SipMessage& msg)
{
   return dynamic_cast<RemoteParticipant *>(h->getAppDialog().get())->onRequestRetry(h, retryMinimum, msg);
}

////////////////////////////////////////////////////////////////////////////////
// ServerSubscriptionHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
ConversationManager::onNewSubscription(ServerSubscriptionHandle, const SipMessage& msg)
{
   InfoLog(<< "onNewSubscription(ServerSubscriptionHandle): " << msg.brief());
}

void 
ConversationManager::onNewSubscriptionFromRefer(ServerSubscriptionHandle ss, const SipMessage& msg)
{
   InfoLog(<< "onNewSubscriptionFromRefer(ServerSubscriptionHandle): " << msg.brief());
   // Received an out-of-dialog refer request with implicit subscription
   try
   {
      if(msg.exists(h_ReferTo))
      {
         // Check if TargetDialog header is present
         if(msg.exists(h_TargetDialog))
         {
            pair<InviteSessionHandle, int> presult;
            presult = mUserAgent->getDialogUsageManager().findInviteSession(msg.header(h_TargetDialog));
            if(!(presult.first == InviteSessionHandle::NotValid())) 
            {         
               RemoteParticipant* participantToRefer = (RemoteParticipant*)presult.first->getAppDialog().get();

               participantToRefer->onRefer(presult.first, ss, msg);
               return;
            }
         }

         // Create new Participant
         RemoteParticipantDialogSet *participantDialogSet = getMediaStackAdapter().createRemoteParticipantDialogSetInstance();
         RemoteParticipant *participant = participantDialogSet->createUACOriginalRemoteParticipant(getNewParticipantHandle());  

         // Set pending OOD info in Participant - causes accept or reject to be called later
         participant->setPendingOODReferInfo(ss, msg);

         // Notify application
         ConversationProfile* profile = dynamic_cast<ConversationProfile*>(ss->getUserProfile().get());
         if(profile)
         {
            onRequestOutgoingParticipant(participant->getParticipantHandle(), msg, *profile);
         }
         else
         {
            // FIXME - could we do something else here?
            WarningLog(<<"not an instance of ConversationProfile, not calling onRequestOutgoingParticipant");
         }
      }
      else
      {
         WarningLog (<< "Received refer w/out a Refer-To: " << msg.brief());
         ss->send(ss->reject(400));
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "onNewSubscriptionFromRefer exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "onNewSubscriptionFromRefer unknown exception");
   }
}

void 
ConversationManager::onRefresh(ServerSubscriptionHandle, const SipMessage& msg)
{
   InfoLog(<< "onRefresh(ServerSubscriptionHandle): " << msg.brief());
}

void 
ConversationManager::onTerminated(ServerSubscriptionHandle)
{
   InfoLog(<< "onTerminated(ServerSubscriptionHandle)");
}

void 
ConversationManager::onReadyToSend(ServerSubscriptionHandle, SipMessage&)
{
}

void 
ConversationManager::onNotifyRejected(ServerSubscriptionHandle, const SipMessage& msg)
{
   WarningLog(<< "onNotifyRejected(ServerSubscriptionHandle): " << msg.brief());
}

void 
ConversationManager::onError(ServerSubscriptionHandle, const SipMessage& msg)
{
   WarningLog(<< "onError(ServerSubscriptionHandle): " << msg.brief());
}

void 
ConversationManager::onExpiredByClient(ServerSubscriptionHandle, const SipMessage& sub, SipMessage& notify)
{
   InfoLog(<< "onExpiredByClient(ServerSubscriptionHandle): " << notify.brief());
}

void 
ConversationManager::onExpired(ServerSubscriptionHandle, SipMessage& msg)
{
   InfoLog(<< "onExpired(ServerSubscriptionHandle): " << msg.brief());
}

bool 
ConversationManager::hasDefaultExpires() const
{
   return true;
}

UInt32 
ConversationManager::getDefaultExpires() const
{
   return 60;
}

////////////////////////////////////////////////////////////////////////////////
// OutOfDialogHandler //////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
ConversationManager::onSuccess(ClientOutOfDialogReqHandle, const SipMessage& msg)
{
   InfoLog(<< "onSuccess(ClientOutOfDialogReqHandle): " << msg.brief());
}

void 
ConversationManager::onFailure(ClientOutOfDialogReqHandle, const SipMessage& msg)
{
   InfoLog(<< "onFailure(ClientOutOfDialogReqHandle): " << msg.brief());
}

void 
ConversationManager::onReceivedRequest(ServerOutOfDialogReqHandle ood, const SipMessage& msg)
{
   InfoLog(<< "onReceivedRequest(ServerOutOfDialogReqHandle): " << msg.brief());

   switch(msg.method())
   {
   case OPTIONS:
   {
      auto optionsAnswer = ood->answerOptions();

      // Attach an offer to the options request
      SdpContents sdp;
      buildSdpOffer(mUserAgent->getIncomingConversationProfile(msg).get(), sdp);
      optionsAnswer->setContents(&sdp);
      ood->send(std::move(optionsAnswer));
      break;
   }
   case REFER:
   {
      // Received an OOD refer request with no refer subscription
      try
      {
         if(msg.exists(h_ReferTo))
         {
            // Check if TargetDialog header is present
            if(msg.exists(h_TargetDialog))
            {
               pair<InviteSessionHandle, int> presult;
               presult = mUserAgent->getDialogUsageManager().findInviteSession(msg.header(h_TargetDialog));
               if(!(presult.first == InviteSessionHandle::NotValid())) 
               {         
                  RemoteParticipant* participantToRefer = (RemoteParticipant*)presult.first->getAppDialog().get();

                  // Accept the Refer
                  ood->send(ood->accept(202 /* Refer Accepted */));

                  participantToRefer->doReferNoSub(msg);
                  return;
               }
            }

            // Create new Participant 
            RemoteParticipantDialogSet *participantDialogSet = getMediaStackAdapter().createRemoteParticipantDialogSetInstance();
            RemoteParticipant *participant = participantDialogSet->createUACOriginalRemoteParticipant(getNewParticipantHandle());  

            // Set pending OOD info in Participant - causes accept or reject to be called later
            participant->setPendingOODReferInfo(ood, msg);

            // Notify application
            ConversationProfile* profile = dynamic_cast<ConversationProfile*>(ood->getUserProfile().get());
            resip_assert(profile);
            if(profile)
            {
               onRequestOutgoingParticipant(participant->getParticipantHandle(), msg, *profile);
            }
            else
            {
               // FIXME - could we do something else here?
               WarningLog(<<"not an instance of ConversationProfile, not calling onRequestOutgoingParticipant");
            }
         }
         else
         {
            WarningLog (<< "onReceivedRequest(ServerOutOfDialogReqHandle): Received refer w/out a Refer-To: " << msg.brief());
            ood->send(ood->reject(400));
         }
      }
      catch(BaseException &e)
      {
         WarningLog(<< "onReceivedRequest(ServerOutOfDialogReqHandle): exception " << e);
      }
      catch(...)
      {
         WarningLog(<< "onReceivedRequest(ServerOutOfDialogReqHandle): unknown exception");
      }

      break;
   }
   default:
      break;
   }
}

////////////////////////////////////////////////////////////////////////////////
// RedirectHandler /////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
ConversationManager::onRedirectReceived(AppDialogSetHandle, const SipMessage& msg)
{
   InfoLog(<< "onRedirectReceived(AppDialogSetHandle): " << msg.brief());
}

bool 
ConversationManager::onTryingNextTarget(AppDialogSetHandle, const SipMessage& msg)
{
   InfoLog(<< "onTryingNextTarget(AppDialogSetHandle): " << msg.brief());
   // Always allow redirection for now
   return true;
}


////////////////////////////////////////////////////////////////////////////////
// ClientPagerMessageHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
ConversationManager::onSuccess(ClientPagerMessageHandle h, const SipMessage& status)
{
   return dynamic_cast<RemoteIMPagerParticipant*>(h->getAppDialogSet().get())->onSuccess(h, status);
}

void 
ConversationManager::onFailure(ClientPagerMessageHandle h, const SipMessage& status, std::unique_ptr<Contents> contents)
{
   return dynamic_cast<RemoteIMPagerParticipant*>(h->getAppDialogSet().get())->onFailure(h, status, std::move(contents));
}


////////////////////////////////////////////////////////////////////////////////
// ServerPagerMessageHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
ConversationManager::onMessageArrived(ServerPagerMessageHandle h, const SipMessage& message)
{
   RemoteIMPagerParticipant* remoteIMPagerParticipant = nullptr;

   // First see if we already have a RemoteIMPagerParticipant for this CallId yet or not
   for (ParticipantMap::iterator i = mParticipants.begin(); i != mParticipants.end(); i++)
   {
      remoteIMPagerParticipant = dynamic_cast<RemoteIMPagerParticipant*>(i->second);
      if (remoteIMPagerParticipant != nullptr && remoteIMPagerParticipant->doesMessageMatch(message))
      {
         // Found existing remoteIMPagerParticipant, break out
         break;
      }
      remoteIMPagerParticipant = nullptr;
   }

   if (remoteIMPagerParticipant == nullptr && mUserAgent != nullptr)
   {
      // If we make it here, we didn't find an existing RemoteIMPagerParticipant via the callId,
      // for From header matching create a new one now.
      remoteIMPagerParticipant = new RemoteIMPagerParticipant(getNewParticipantHandle(), *this);
   }

   if (remoteIMPagerParticipant)
   {
      remoteIMPagerParticipant->onMessageArrived(h, message);
   }
   else
   {
      h->send(h->reject(500)); // Reject with internal server error
   }
   
}


/* ====================================================================

 Copyright (c) 2021-2022, SIP Spectrum, Inc. www.sipspectrum.com
 Copyright (c) 2021, Daniel Pocock https://danielpocock.com
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
