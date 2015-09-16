// sipX includes
#if (_MSC_VER >= 1600)
#include <stdint.h>       // Use Visual Studio's stdint.h
#define _MSC_STDINT_H_    // This define will ensure that stdint.h in sipXport tree is not used
#endif
#include <sdp/SdpCodec.h>
#include <os/OsConfigDb.h>
#include <mp/MpCodecFactory.h>
#include <mp/MprBridge.h>
#include <mp/MpResourceTopology.h>
#include <mi/CpMediaInterfaceFactoryFactory.h>
#include <mi/CpMediaInterface.h>

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

#include "ReconSubsystem.hxx"
#include "UserAgent.hxx"
#include "ConversationManager.hxx"
#include "ConversationManagerCmds.hxx"
#include "Conversation.hxx"
#include "Participant.hxx"
#include "BridgeMixer.hxx"
#include "DtmfEvent.hxx"
#include <rutil/WinLeakCheck.hxx>

#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4355 )
#endif

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

ConversationManager::ConversationManager(bool localAudioEnabled, MediaInterfaceMode mediaInterfaceMode) 
: mUserAgent(0),
  mCurrentConversationHandle(1),
  mCurrentParticipantHandle(1),
  mLocalAudioEnabled(localAudioEnabled),
  mMediaInterfaceMode(mediaInterfaceMode),
  mMediaFactory(0),
  mBridgeMixer(0),
  mSipXTOSValue(0)
{
   init();
}

ConversationManager::ConversationManager(bool localAudioEnabled, MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate)
: mUserAgent(0),
  mCurrentConversationHandle(1),
  mCurrentParticipantHandle(1),
  mLocalAudioEnabled(localAudioEnabled),
  mMediaInterfaceMode(mediaInterfaceMode),
  mMediaFactory(0),
  mBridgeMixer(0),
  mSipXTOSValue(0)
{
   init(defaultSampleRate, maxSampleRate);
}

void
ConversationManager::init(int defaultSampleRate, int maxSampleRate)
{
#ifdef _DEBUG
   UtlString codecPaths[] = {".", "../../../../sipXtapi/sipXmediaLib/bin"};
#else
   UtlString codecPaths[] = {"."};
#endif
   int codecPathsNum = sizeof(codecPaths)/sizeof(codecPaths[0]);
   OsStatus rc = CpMediaInterfaceFactory::addCodecPaths(codecPathsNum, codecPaths);
   resip_assert(OS_SUCCESS == rc);

   if(mMediaInterfaceMode == sipXConversationMediaInterfaceMode)
   {
      OsConfigDb sipXconfig;
      sipXconfig.set("PHONESET_MAX_ACTIVE_CALLS_ALLOWED",300);  // This controls the maximum number of flowgraphs allowed - default is 16
      mMediaFactory = sipXmediaFactoryFactory(&sipXconfig, 0, defaultSampleRate, maxSampleRate, mLocalAudioEnabled);
   }
   else
   {
      mMediaFactory = sipXmediaFactoryFactory(NULL, 0, defaultSampleRate, maxSampleRate, mLocalAudioEnabled);
   }

   // Create MediaInterface
   MpCodecFactory *pCodecFactory = MpCodecFactory::getMpCodecFactory();
   // For dynamic loading, we need to force the codecs to be loaded
   // somehow, this should be exposed through the API
   //pCodecFactory->loadAllDynCodecs("/tmp/codecs", "");
   unsigned int count = 0;
   const MppCodecInfoV1_1 **codecInfoArray;
   pCodecFactory->getCodecInfoArray(count, codecInfoArray);

   if(count == 0)
   {
      // Try to load plugin modules, if possible...
      InfoLog(<<"No statically linked codecs, trying to load codec plugin modules with dlopen()");
      pCodecFactory->loadAllDynCodecs(NULL, "");
      pCodecFactory->getCodecInfoArray(count, codecInfoArray);
      if(count == 0)
      {
         ErrLog( << "No codec plugins found.  Cannot start.");
         exit(-1);
      }
   }

   InfoLog( << "Loaded codecs are:");
   for(unsigned int i =0; i < count; i++)
   {
      InfoLog( << "  " << codecInfoArray[i]->codecName 
               << "(" << codecInfoArray[i]->codecManufacturer << ") " 
               << codecInfoArray[i]->codecVersion 
               << " MimeSubtype: " << codecInfoArray[i]->mimeSubtype 
               << " Rate: " << codecInfoArray[i]->sampleRate
               << " Channels: " << codecInfoArray[i]->numChannels);
   }

   if(mMediaInterfaceMode == sipXGlobalMediaInterfaceMode)
   {
      createMediaInterfaceAndMixer(mLocalAudioEnabled /* giveFocus?*/,    // This is the one and only media interface - give it focus
                                   0,
                                   mMediaInterface, 
                                   &mBridgeMixer);      
   }
}

ConversationManager::~ConversationManager()
{
   resip_assert(mConversations.empty());
   resip_assert(mParticipants.empty());
   delete mBridgeMixer;
   if(mMediaInterface) mMediaInterface.reset();  // Make sure inteface is destroyed before factory
   sipxDestroyMediaFactoryFactory();
}

void
ConversationManager::setUserAgent(UserAgent* userAgent)
{
   mUserAgent = userAgent;

   // Note: This is not really required, since we are managing the port allocation - but no harm done
   // Really not needed now - since FlowManager integration
   //mMediaFactory->getFactoryImplementation()->setRtpPortRange(mUserAgent->getUserAgentMasterProfile()->rtpPortRangeMin(), 
   //                                                           mUserAgent->getUserAgentMasterProfile()->rtpPortRangeMax());

   initRTPPortFreeList();
}

void
ConversationManager::shutdown()
{
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

ConversationHandle 
ConversationManager::createConversation(bool broadcastOnly)
{
   ConversationHandle convHandle = getNewConversationHandle();

   CreateConversationCmd* cmd = new CreateConversationCmd(this, convHandle, broadcastOnly);
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
ConversationManager::createRemoteParticipant(ConversationHandle convHandle, const NameAddr& destination, ParticipantForkSelectMode forkSelectMode)
{
   SharedPtr<UserProfile> profile;
   return createRemoteParticipant(convHandle, destination, forkSelectMode, profile, std::multimap<resip::Data,resip::Data>());
}

ParticipantHandle
ConversationManager::createRemoteParticipant(ConversationHandle convHandle, const resip::NameAddr& destination, ParticipantForkSelectMode forkSelectMode, SharedPtr<UserProfile>& callerProfile, const std::multimap<resip::Data,resip::Data>& extraHeaders)
{
   ParticipantHandle partHandle = getNewParticipantHandle();

   CreateRemoteParticipantCmd* cmd = new CreateRemoteParticipantCmd(this, partHandle, convHandle, destination, forkSelectMode, callerProfile, extraHeaders);
   post(cmd);

   return partHandle;
}

ParticipantHandle 
ConversationManager::createMediaResourceParticipant(ConversationHandle convHandle, const Uri& mediaUrl)
{
   ParticipantHandle partHandle = getNewParticipantHandle();

   CreateMediaResourceParticipantCmd* cmd = new CreateMediaResourceParticipantCmd(this, partHandle, convHandle, mediaUrl);
   post(cmd);

   return partHandle;
}

ParticipantHandle 
ConversationManager::createLocalParticipant()
{
   ParticipantHandle partHandle = 0;
   if(mLocalAudioEnabled)
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
ConversationManager::outputBridgeMatrix()
{
   if(mMediaInterfaceMode == sipXGlobalMediaInterfaceMode)
   {
      OutputBridgeMixWeightsCmd* cmd = new OutputBridgeMixWeightsCmd(this);
      post(cmd);
   }
   else
   {
      WarningLog(<< "ConversationManager::outputBridgeMatrix not supported in current Media Interface Mode");
   }
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
ConversationManager::redirectParticipant(ParticipantHandle partHandle, const NameAddr& destination)
{
   RedirectParticipantCmd* cmd = new RedirectParticipantCmd(this, partHandle, destination);
   post(cmd);
}

void 
ConversationManager::redirectToParticipant(ParticipantHandle partHandle, ParticipantHandle destPartHandle)
{
   RedirectToParticipantCmd* cmd = new RedirectToParticipantCmd(this, partHandle, destPartHandle);
   post(cmd);
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
   mUserAgent->getDialogUsageManager().post(msg);
}

void 
ConversationManager::post(resip::ApplicationMessage& message, unsigned int ms)
{
    mUserAgent->post(message, ms);
}

void 
ConversationManager::initRTPPortFreeList()
{
   mRTPPortFreeList.clear();
   for(unsigned int i = mUserAgent->getUserAgentMasterProfile()->rtpPortRangeMin(); i <= mUserAgent->getUserAgentMasterProfile()->rtpPortRangeMax();)
   {
      mRTPPortFreeList.push_back(i);
      i=i+2;  // only add even ports - note we are assuming rtpPortRangeMin is even
   }
}
 
unsigned int 
ConversationManager::allocateRTPPort()
{
   unsigned int port = 0;
   if(!mRTPPortFreeList.empty())
   {
      port = mRTPPortFreeList.front();
      mRTPPortFreeList.pop_front();
   }
   return port;
}
 
void
ConversationManager::freeRTPPort(unsigned int port)
{
   resip_assert(port >= mUserAgent->getUserAgentMasterProfile()->rtpPortRangeMin() && port <= mUserAgent->getUserAgentMasterProfile()->rtpPortRangeMax());
   mRTPPortFreeList.push_back(port);
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

void
ConversationManager::setSpeakerVolume(int volume)
{
   OsStatus status =  mMediaFactory->getFactoryImplementation()->setSpeakerVolume(volume);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "setSpeakerVolume failed: status=" << status);
   }
}

void 
ConversationManager::setMicrophoneGain(int gain)
{
   OsStatus status =  mMediaFactory->getFactoryImplementation()->setMicrophoneGain(gain);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "setMicrophoneGain failed: status=" << status);
   }
}

void 
ConversationManager::muteMicrophone(bool mute)
{
   OsStatus status =  mMediaFactory->getFactoryImplementation()->muteMicrophone(mute? TRUE : FALSE);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "muteMicrophone failed: status=" << status);
   }
}
 
void 
ConversationManager::enableEchoCancel(bool enable)
{
   OsStatus status =  mMediaFactory->getFactoryImplementation()->setAudioAECMode(enable ? MEDIA_AEC_CANCEL : MEDIA_AEC_DISABLED);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "enableEchoCancel failed: status=" << status);
   }
   if(mMediaInterfaceMode == sipXGlobalMediaInterfaceMode)  // Note for sipXConversationMediaInterfaceMode - setting will apply on next conversation given focus
   {
      mMediaInterface->getInterface()->defocus();   // required to apply changes
      mMediaInterface->getInterface()->giveFocus();
   }
}

void 
ConversationManager::enableAutoGainControl(bool enable)
{
   OsStatus status =  mMediaFactory->getFactoryImplementation()->enableAGC(enable ? TRUE : FALSE);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "enableAutoGainControl failed: status=" << status);
   }
   if(mMediaInterfaceMode == sipXGlobalMediaInterfaceMode)  // Note for sipXConversationMediaInterfaceMode - setting will apply on next conversation given focus
   {
      mMediaInterface->getInterface()->defocus();   // required to apply changes
      mMediaInterface->getInterface()->giveFocus();
   }
}
 
void 
ConversationManager::enableNoiseReduction(bool enable)
{
   OsStatus status =  mMediaFactory->getFactoryImplementation()->setAudioNoiseReductionMode(enable ? MEDIA_NOISE_REDUCTION_MEDIUM /* arbitrary */ : MEDIA_NOISE_REDUCTION_DISABLED);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "enableAutoGainControl failed: status=" << status);
   }
   if(mMediaInterfaceMode == sipXGlobalMediaInterfaceMode)  // Note for sipXConversationMediaInterfaceMode - setting will apply on next conversation given focus
   {
      mMediaInterface->getInterface()->defocus();   // required to apply changes
      mMediaInterface->getInterface()->giveFocus();
   }
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

void 
ConversationManager::addBufferToMediaResourceCache(const resip::Data& name, const resip::Data& buffer, int type)
{
   mMediaResourceCache.addToCache(name, buffer, type);
}

void 
ConversationManager::buildSessionCapabilities(const resip::Data& ipaddress, unsigned int numCodecIds, 
                                              unsigned int codecIds[], resip::SdpContents& sessionCaps)
{
   sessionCaps = SdpContents::Empty;  // clear out passed in SdpContents

   // Check if ipaddress is V4 or V6
   bool v6 = false;
   if(!ipaddress.empty())
   {
      Tuple testTuple(ipaddress, 0, UDP);
      if(testTuple.ipVersion() == V6)
      {
         v6 = true;
      }
   }

   // Create Session Capabilities 
   // Note:  port, sessionId and version will be replaced in actual offer/answer
   // Build s=, o=, t=, and c= lines
   SdpContents::Session::Origin origin("-", 0 /* sessionId */, 0 /* version */, v6 ? SdpContents::IP6 : SdpContents::IP4, ipaddress.empty() ? "0.0.0.0" : ipaddress);   // o=   
   SdpContents::Session session(0, origin, "-" /* s= */);
   session.connection() = SdpContents::Session::Connection(v6 ? SdpContents::IP6 : SdpContents::IP4, ipaddress.empty() ? "0.0.0.0" : ipaddress);  // c=
   session.addTime(SdpContents::Session::Time(0, 0));

   MpCodecFactory *pCodecFactory = MpCodecFactory::getMpCodecFactory();
   SdpCodecList codecList;
   pCodecFactory->addCodecsToList(codecList);
   codecList.bindPayloadTypes();

   //UtlString output;
   //codecList.toString(output);
   //InfoLog( << "Codec List: " << output.data());

   // Auto-Create Session Codec Capabilities
   // Note:  port, and potentially payloadid will be replaced in actual offer/answer

   // Build Codecs and media offering
   SdpContents::Session::Medium medium("audio", 0, 1, "RTP/AVP");

   bool firstCodecAdded = false;
   for(unsigned int idIter = 0; idIter < numCodecIds; idIter++)
   {
      const SdpCodec* sdpcodec = codecList.getCodec((SdpCodec::SdpCodecTypes)codecIds[idIter]);
      if(sdpcodec)
      {
         UtlString mediaType;
         sdpcodec->getMediaType(mediaType);
         // Ensure this codec is an audio codec
         if(mediaType.compareTo("audio", UtlString::ignoreCase) == 0)
         {
            UtlString mimeSubType;
            sdpcodec->getEncodingName(mimeSubType);
            //mimeSubType.toUpper();
            
            SdpContents::Session::Codec codec(mimeSubType.data(), sdpcodec->getCodecPayloadFormat(), sdpcodec->getSampleRate());
            if(sdpcodec->getNumChannels() > 1)
            {
               codec.encodingParameters() = Data(sdpcodec->getNumChannels());
            }

            // Check for telephone-event and add fmtp manually
            if(mimeSubType.compareTo("telephone-event", UtlString::ignoreCase) == 0)
            {
               codec.parameters() = Data("0-15");
            }
            else
            {
               UtlString fmtpField;
               sdpcodec->getSdpFmtpField(fmtpField);
               if(fmtpField.length() != 0)
               {
                  codec.parameters() = Data(fmtpField.data());
               }
            }

            DebugLog(<< "Added codec to session capabilites: id=" << codecIds[idIter] 
                    << " type=" << mimeSubType.data()
                    << " rate=" << sdpcodec->getSampleRate()
                    << " plen=" << sdpcodec->getPacketLength()
                    << " payloadid=" << sdpcodec->getCodecPayloadFormat()
                    << " fmtp=" << codec.parameters());

            medium.addCodec(codec);
            if(!firstCodecAdded)
            {
               firstCodecAdded = true;

               // 20 ms of speech per frame (note G711 has 10ms samples, so this is 2 samples per frame)
               // Note:  There are known problems with SDP and the ptime attribute.  For now we will choose an
               // appropriate ptime from the first codec
               medium.addAttribute("ptime", Data(sdpcodec->getPacketLength() / 1000));  
            }
         }
      }
   }

   session.addMedium(medium);
   sessionCaps.session() = session;
}

void 
ConversationManager::notifyMediaEvent(ConversationHandle conversationHandle, int mediaConnectionId, MediaEvent::MediaEventType eventType)
{
   resip_assert(eventType == MediaEvent::PLAY_FINISHED);

   if(conversationHandle == 0) // sipXGlobalMediaInterfaceMode
   {
      if(eventType == MediaEvent::PLAY_FINISHED)
      {
         // Using sipXGlobalMediaInterfaceMode it is only possible to have one active media participant
         // actually playing a file (or from cache) at a time, so for now it is sufficient to have
         // this event indicate that any active media participants (playing a file/cache) should be destroyed.
         ParticipantMap::iterator it;
         for(it = mParticipants.begin(); it != mParticipants.end();)
         {
            MediaResourceParticipant* mrPart = dynamic_cast<MediaResourceParticipant*>(it->second);
            it++;  // increment iterator here, since destroy may end up calling unregisterParticipant
            if(mrPart)
            {
               if(mrPart->getResourceType() == MediaResourceParticipant::File ||
                  mrPart->getResourceType() == MediaResourceParticipant::Cache)
               {
                  mrPart->destroyParticipant();
               }
            }
         }
      }
   }
   else
   {
      Conversation* conversation = getConversation(conversationHandle);
      if(conversation)
      {
         conversation->notifyMediaEvent(mediaConnectionId, eventType);
      }
   }
}

void 
ConversationManager::notifyDtmfEvent(ConversationHandle conversationHandle, int mediaConnectionId, int dtmf, int duration, bool up)
{
   if(conversationHandle == 0) // sipXGlobalMediaInterfaceMode
   {
      ParticipantMap::iterator i = mParticipants.begin();
      for(; i != mParticipants.end(); i++)
      {
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(i->second);
         if(remoteParticipant)
         {
            if(remoteParticipant->getMediaConnectionId() == mediaConnectionId)
            {
               onDtmfEvent(remoteParticipant->getParticipantHandle(), dtmf, duration, up);
            }
         }
      }
   }
   else
   {
      Conversation* conversation = getConversation(conversationHandle);
      if(conversation)
      {
         conversation->notifyDtmfEvent(mediaConnectionId, dtmf, duration, up);
      }
   }
}

void 
ConversationManager::createMediaInterfaceAndMixer(bool giveFocus, 
                                                  ConversationHandle ownerConversationHandle,
                                                  SharedPtr<MediaInterface>& mediaInterface, 
                                                  BridgeMixer** bridgeMixer)
{
   UtlString localRtpInterfaceAddress("127.0.0.1");  // Will be overridden in RemoteParticipantDialogSet, when connection is created anyway

   // Note:  STUN and TURN capabilities of the sipX media stack are not used - the FlowManager is responsible for STUN/TURN
   mediaInterface = SharedPtr<MediaInterface>(new MediaInterface(*this, ownerConversationHandle, mMediaFactory->createMediaInterface(NULL, 
            localRtpInterfaceAddress, 
            0,     /* numCodecs - not required at this point */
            0,     /* codecArray - not required at this point */ 
            NULL,  /* local */
            mSipXTOSValue,  /* TOS Options */
            NULL,  /* STUN Server Address */
            0,     /* STUN Options */
            25,    /* STUN Keepalive period (seconds) */
            NULL,  /* TURN Server Address */
            0,     /* TURN Port */
            NULL,  /* TURN User */
            NULL,  /* TURN Password */
            25,    /* TURN Keepalive period (seconds) */
            false))); /* enable ICE? */

   // Register the NotificationDispatcher class (derived from OsMsgDispatcher)
   // as the sipX notification dispatcher
   mediaInterface->getInterface()->setNotificationDispatcher(mediaInterface.get());

   // Turn on notifications for all resources...
   mediaInterface->getInterface()->setNotificationsEnabled(true);

   if(giveFocus)
   {
      mediaInterface->getInterface()->giveFocus();
   }

   *bridgeMixer = new BridgeMixer(*(mediaInterface->getInterface()));
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
         RemoteParticipantDialogSet *participantDialogSet = new RemoteParticipantDialogSet(*this);
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
      SharedPtr<SipMessage> optionsAnswer = ood->answerOptions();

      // Attach an offer to the options request
      SdpContents sdp;
      buildSdpOffer(mUserAgent->getIncomingConversationProfile(msg).get(), sdp);
      optionsAnswer->setContents(&sdp);
      ood->send(optionsAnswer);
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
            RemoteParticipantDialogSet *participantDialogSet = new RemoteParticipantDialogSet(*this);
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
