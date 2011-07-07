// sipX includes
//include <sdp/SdpCodec.h>
//include <mp/MpCodecFactory.h>
//include <mp/MprBridge.h>
//include <mp/MpResourceTopology.h>
//include <mi/MiNotification.h>
//include <mi/MiDtmfNotf.h>
//include <mi/MiRtpStreamActivityNotf.h>
//include <mi/MiIntNotf.h>

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
#include "UserAgentDialogSetFactory.hxx" // !ds! should probably be moved in future
#include "UserAgentServerAuthManager.hxx"
#include "ConversationManager.hxx"
#include "ConversationManagerCmds.hxx"
#include "Conversation.hxx"
#include "Participant.hxx"
#include "BridgeMixer.hxx"
#include "DtmfEvent.hxx"
#include "UserAgentRegistration.hxx"
#include "media/Mixer.hxx"
#include <rutil/WinLeakCheck.hxx>

#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4355 )
#endif

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

ConversationManager::ConversationManager(UserAgent& ua) 
: mDefaultOutgoingConversationProfileHandle(0),
  mCurrentConversationProfileHandle(1),
  mCurrentConversationHandle(1),
  mCurrentParticipantHandle(1),
  mLocalAudioEnabled(true),
  mMediaStack(ua.getMediaStack()),
  mBridgeMixer(*this),
  mCodecFactory(ua.getCodecFactory()),
  mRTPAllocator(NULL),
  mDum(NULL)
{
   sdpcontainer::SdpMediaLine::CodecList codecs;
   mCodecFactory->getCodecs(true, true, codecs);
   
   mDum = ua.getDialogUsageManager();
   mDum->setRedirectHandler(this);
   mDum->setInviteSessionHandler(this); 
   mDum->setDialogSetHandler(this);
   mDum->addOutOfDialogHandler(OPTIONS, this);
   mDum->addOutOfDialogHandler(REFER, this);
   mDum->addClientSubscriptionHandler("refer", this);
   mDum->addServerSubscriptionHandler("refer", this);

   // Set AppDialogSetFactory
   // .jjg. this could interfere with apps that don't use recon for things other than INVITE dialogs...
   //auto_ptr<AppDialogSetFactory> dsf(new UserAgentDialogSetFactory(*this));
   //mDum->setAppDialogSetFactory(dsf);

   // Set UserAgentServerAuthManager
   // !jjg! interferes with apps that don't use recon for things other than INVITEs...
   // also is problematic for interop, since it means that we would send a 403 if we
   // get an Auth header with a nonce that doesn't match the format we are expecting,
   // whereas if we do not set a ServerAuthManager, we would never send the 403...
   //SharedPtr<ServerAuthManager> uasAuth( new UserAgentServerAuthManager(*this));
   //mDum->setServerAuthManager(uasAuth);

   mRegManager = ua.getRegistrationManager();

   unsigned int minPort = 0, maxPort = 0;
   ua.getPortRange(minPort, maxPort);
   mRTPAllocator = new RTPPortAllocator(mFlowManager.getIOService(), minPort, maxPort);

   if (codecs.size() == 0)
   {
      ErrLog( << "No codec plugins found. Cannot start.");
   }

   InfoLog( << "Loaded codecs are:");
   sdpcontainer::SdpMediaLine::CodecList::const_iterator codecIter = codecs.begin();
   for (; codecIter != codecs.end(); ++codecIter)
   {
      InfoLog( << "  " << codecIter->getMimeSubtype()
               << " MediaType: " << codecIter->getMimeType()
               << " SampleRate: " << codecIter->getRate()
               << " PayloadType: " << codecIter->getPayloadType()
               << " PacketLength: " << codecIter->getPacketTime());
   }
}

ConversationManager::~ConversationManager()
{
   assert(mConversations.empty());
   assert(mParticipants.empty());
   delete mRTPAllocator; mRTPAllocator = NULL;
}

void
ConversationManager::shutdown()
{
   // Destroy each Conversation
   ConversationMap tempConvs = mConversations;  // Create copy for safety, since ending conversations can immediately remove themselves from map
   ConversationMap::iterator i;
   for(i = tempConvs.begin(); i != tempConvs.end(); ++i)
   {
      InfoLog(<< "Destroying conversation: " << i->second->getHandle());
      i->second->destroy();
   }

   // End each Participant
   ParticipantMap tempParts = mParticipants;  
   ParticipantMap::iterator j;
   int j2=0;
   for(j = tempParts.begin(); j != tempParts.end(); ++j, ++j2)
   {
      InfoLog(<< "Destroying participant: " << j->second->getParticipantHandle());
      j->second->destroyParticipant();
   }
}

resip::AppDialogSetFactory* ConversationManager::getAppDialogSetFactory()
{
	return new UserAgentDialogSetFactory(*this);
}

ConversationProfileHandle 
ConversationManager::getNewConversationProfileHandle()
{
   Lock lock(mConversationProfileHandleMutex);
   return mCurrentConversationProfileHandle++; 
}

ConversationProfileHandle 
ConversationManager::addConversationProfile(SharedPtr<ConversationProfile> conversationProfile, bool defaultOutgoing)
{
   ConversationProfileHandle handle = getNewConversationProfileHandle();
   mDum->post(new AddConversationProfileCmd(this, handle, conversationProfile, defaultOutgoing));
   return handle;
}

void
ConversationManager::setDefaultOutgoingConversationProfile(ConversationProfileHandle handle)
{
   mDum->post(new SetDefaultOutgoingConversationProfileCmd(this, handle));
}

void 
ConversationManager::destroyConversationProfile(ConversationProfileHandle handle)
{
   mDum->post(new DestroyConversationProfileCmd(this, handle));
}

ConversationProfileHandle ConversationManager::cloneConversationProfile( ConversationProfileHandle handle )
{
   ConversationProfileHandle newHandle = getNewConversationProfileHandle();
   class CloneConversationCmd : public DumCommandStub
   {
   public:
      CloneConversationCmd( ConversationManager* cmgr, ConversationProfileHandle oldHandle, ConversationProfileHandle newHandle )
         : ConMgr( cmgr ), OldHandle( oldHandle ), NewHandle( newHandle ) {}
      virtual void executeCommand()
      {
         SharedPtr<ConversationProfile> profile = ConMgr->getConversationProfile( OldHandle );
         assert( profile.get() != NULL );

         SharedPtr<ConversationProfile> clone( new ConversationProfile( *profile ));
         assert( clone.get() != NULL );

         ConMgr->addConversationProfileImpl(NewHandle, clone, false);
      }
   private:
      ConversationManager *ConMgr;
      ConversationProfileHandle OldHandle, NewHandle;
   };
   mDum->post(new CloneConversationCmd( this, handle, newHandle ));
   return newHandle;
}

void 
ConversationManager::addConversationProfileImpl(ConversationProfileHandle handle, SharedPtr<ConversationProfile> conversationProfile, bool defaultOutgoing)
{
   // Store new profile
   mConversationProfiles[handle] = conversationProfile;
   conversationProfile->handle() = handle;

#ifdef USE_DTLS
   // If this is the first profile ever set - then use the aor defined in it as
   // the aor used in the DTLS certificate for the DtlsFactory - TODO - improve
   // this sometime so that we can change the aor in the cert at runtime to
   // equal the aor in the default conversation profile
   if(!mDefaultOutgoingConversationProfileHandle)
   {
      getFlowManager().initializeDtlsFactory(conversationProfile->getDefaultFrom().uri().getAor().c_str());
   }
#endif

   // Set the default outgoing if requested to do so, or we don't have one yet
   if(defaultOutgoing || mDefaultOutgoingConversationProfileHandle == 0)
   {
      setDefaultOutgoingConversationProfileImpl(handle);
   }

   //// Register new profile
   //if(mRegManager && mRegManager->manageRegistrations() && ( conversationProfile->getDefaultRegistrationTime() != 0))
   //{
   //   UserAgentRegistration *registration = new UserAgentRegistration(*mRegManager, *mDum, handle);
   //   mDum->send(mDum->makeRegistration(conversationProfile->getDefaultFrom(), conversationProfile, registration));
   //}
}

void 
ConversationManager::setDefaultOutgoingConversationProfileImpl(ConversationProfileHandle handle)
{
   mDefaultOutgoingConversationProfileHandle = handle;
}

void 
ConversationManager::destroyConversationProfileImpl(ConversationProfileHandle handle)
{
   //if (mRegManager)
   //{
   //   // Remove matching registration if found
   //   mRegManager->removeRegistration( handle );
   //}

   // Remove from ConversationProfile map
   mConversationProfiles.erase(handle);

   // If this Conversation Profile was the default - select the first item
   // in the map as the new default
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

SharedPtr<ConversationProfile>
ConversationManager::getConversationProfile( ConversationProfileHandle cpHandle )
{
   return mConversationProfiles[ cpHandle ];
}

SharedPtr<ConversationProfile> 
ConversationManager::getDefaultOutgoingConversationProfile()
{
   if(mDefaultOutgoingConversationProfileHandle != 0)
   {
      return mConversationProfiles[mDefaultOutgoingConversationProfileHandle];
   }
   else
   {
      assert(false);
      return SharedPtr<ConversationProfile>((ConversationProfile*)0);
   }
}

SharedPtr<ConversationProfile> 
ConversationManager::getIncomingConversationProfile(const SipMessage& msg)
{
   assert(msg.isRequest());

   // Examine the sip message, and select the most appropriate conversation profile
   // Check if request uri matches registration contact
   const Uri& requestUri = msg.header(h_RequestLine).uri();
   UriToConversationProfileMap::iterator cpIt = mMapDefaultIncomingConvProfile.find(requestUri);
   if (cpIt != mMapDefaultIncomingConvProfile.end())
   {
      ConversationProfileMap::iterator conIt = mConversationProfiles.find(cpIt->second);
      if(conIt != mConversationProfiles.end())
      {
         return conIt->second;
      }
   }

   // Check if To header matches default from
   Data toAor = msg.header(h_To).uri().getAor();
   ConversationProfileMap::iterator conIt;
   for(conIt = mConversationProfiles.begin(); conIt != mConversationProfiles.end(); ++conIt)
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

ConversationHandle 
ConversationManager::createConversation()
{
   ConversationHandle convHandle = getNewConversationHandle();
   mDum->post(new CreateConversationCmd(this, convHandle, 0));
   return convHandle;
}

ConversationHandle 
ConversationManager::createConversation(ConversationProfileHandle cpHandle)
{
   ConversationHandle convHandle = getNewConversationHandle();
   mDum->post(new CreateConversationCmd(this, convHandle, cpHandle));
   return convHandle;
}

void 
ConversationManager::updateMedia(ParticipantHandle partHandle, const ConversationManager::MediaAttributes& mediaAttribs, bool sendOffer)
{
  class UpdateMediaCmd : public DumCommandStub
   {
   public:
      UpdateMediaCmd(ConversationManager* convMan, ParticipantHandle partHandle, const ConversationManager::MediaAttributes& mediaAttribs, bool sendOffer)
         : mConvMan(convMan), mMediaAttribs(mediaAttribs), mPartHandle(partHandle), mSendOffer(sendOffer) {}
      virtual void executeCommand()
      {
         RemoteParticipant* participant = dynamic_cast<RemoteParticipant*>(mConvMan->getParticipant(mPartHandle));
         if (participant)
         {
            participant->updateMedia(mMediaAttribs, mSendOffer);
         }
      }
   private:
      ConversationManager*    mConvMan;
      ConversationManager::MediaAttributes mMediaAttribs;
      ParticipantHandle       mPartHandle;
      bool                    mSendOffer;
   };
   mDum->post(new UpdateMediaCmd(this, partHandle, mediaAttribs, sendOffer));
}

void 
ConversationManager::destroyConversation(ConversationHandle convHandle)
{
   mDum->post(new DestroyConversationCmd(this, convHandle));
}

void 
ConversationManager::joinConversation(ConversationHandle sourceConvHandle, ConversationHandle destConvHandle)
{
   mDum->post(new JoinConversationCmd(this, sourceConvHandle, destConvHandle));
}

ParticipantHandle 
ConversationManager::createRemoteParticipant(ConversationHandle convHandle, NameAddr& destination, const MediaAttributes& mediaAttributes, const CallAttributes& callAttributes)
{
   ParticipantHandle partHandle = getNewParticipantHandle();
   mDum->post(new CreateRemoteParticipantCmd(this, partHandle, convHandle, destination, mediaAttributes, callAttributes));
   return partHandle;
}

ParticipantHandle 
ConversationManager::createMediaResourceParticipant(ConversationHandle convHandle, Uri& mediaUrl)
{
   ParticipantHandle partHandle = getNewParticipantHandle();
   mDum->post(new CreateMediaResourceParticipantCmd(this, partHandle, convHandle, mediaUrl));
   return partHandle;
}

ParticipantHandle 
ConversationManager::createLocalParticipant()
{
   ParticipantHandle partHandle = 0;
   if(mLocalAudioEnabled)
   {
      partHandle = getNewParticipantHandle();
      mDum->post(new CreateLocalParticipantCmd(this, partHandle));
   }
   else
   {
      WarningLog(<< "createLocalParticipant called when local audio support is disabled.");
   }

   return partHandle;
}

void 
ConversationManager::destroyParticipant(ParticipantHandle partHandle, const resip::Data& appDefinedReason)
{
   mDum->post(new DestroyParticipantCmd(this, partHandle, appDefinedReason));
}

void 
ConversationManager::addParticipant(ConversationHandle convHandle, ParticipantHandle partHandle)
{
   mDum->post(new AddParticipantCmd(this, convHandle, partHandle));
}

void 
ConversationManager::removeParticipant(ConversationHandle convHandle, ParticipantHandle partHandle)
{
   mDum->post(new RemoveParticipantCmd(this, convHandle, partHandle));
}

void
ConversationManager::moveParticipant(ParticipantHandle partHandle, ConversationHandle sourceConvHandle, ConversationHandle destConvHandle, bool bTriggerHold )
{
   mDum->post(new MoveParticipantCmd(this, partHandle, sourceConvHandle, destConvHandle, bTriggerHold ));
}

void 
ConversationManager::modifyParticipantContribution(ConversationHandle convHandle, ParticipantHandle partHandle, unsigned int inputGain, unsigned int outputGain)
{
   mDum->post(new ModifyParticipantContributionCmd(this, convHandle, partHandle, inputGain, outputGain));
}

void 
ConversationManager::outputBridgeMatrix()
{
   mDum->post(new OutputBridgeMixWeightsCmd(this));
}

void 
ConversationManager::alertParticipant(ParticipantHandle partHandle, bool earlyFlag)
{
   mDum->post(new AlertParticipantCmd(this, partHandle, earlyFlag));
}

void 
ConversationManager::answerParticipant(ParticipantHandle partHandle, MediaAttributes mediaAttributes)
{
   mDum->post(new AnswerParticipantCmd(this, partHandle, mediaAttributes));
}

void 
ConversationManager::rejectParticipant(ParticipantHandle partHandle, unsigned int rejectCode)
{
   mDum->post(new RejectParticipantCmd(this, partHandle, rejectCode));
}

void 
ConversationManager::redirectParticipant(ParticipantHandle partHandle, NameAddr& destination)
{
   mDum->post(new RedirectParticipantCmd(this, partHandle, destination));
}

void 
ConversationManager::redirectToParticipant(ParticipantHandle partHandle, ParticipantHandle destPartHandle)
{
   mDum->post(new RedirectToParticipantCmd(this, partHandle, destPartHandle));
}

ConversationHandle 
ConversationManager::getNewConversationHandle()
{
   Lock lock(mConversationHandleMutex);
   return mCurrentConversationHandle++; 
}

void 
ConversationManager::registerConversation(Conversation* conversation)
{
   if ( conversation == NULL )
      return;

   mConversations[conversation->getHandle()] = conversation;
}

void 
ConversationManager::unregisterConversation(Conversation* conversation)
{
   if ( conversation == NULL )
      return;

   mConversations.erase(conversation->getHandle());
}

BridgeMixer& 
ConversationManager::getBridgeMixer()
{
   return mBridgeMixer;
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
ConversationManager::buildSdpOffer(ConversationProfile* profile, SdpContents& offer)
{
   // copy over session capabilities
   offer = profile->sessionCaps();

   // Set sessionid and version for this offer
   UInt64 currentTime = Timer::getTimeMicroSec();
   offer.session().origin().getSessionId() = currentTime;
   offer.session().origin().getVersion() = 1;
}

void
ConversationManager::setSpeakerVolume(int volume)
{
   int status = mMediaStack->setSpeakerVolume(volume);
   if (status != 0)
   {
      WarningLog(<< "setSpeakerVolume failed: status=" << status);
   }
}

void 
ConversationManager::setMicrophoneGain(int gain)
{
   int status =  mMediaStack->setMicrophoneGain(gain);
   if(status != 0)
   {
      WarningLog(<< "setMicrophoneGain failed: status=" << status);
   }
}

void 
ConversationManager::muteMicrophone(bool mute)
{
   int status =  mMediaStack->muteMicrophone(mute);
   if(status != 0)
   {
      WarningLog(<< "muteMicrophone failed: status=" << status);
   }
}
 
void 
ConversationManager::enableEchoCancel(bool enable)
{
   int status =  mMediaStack->setEchoCancellation(enable);
   if(status != 0)
   {
      WarningLog(<< "enableEchoCancel failed: status=" << status);
   }
}

void 
ConversationManager::enableAutoGainControl(bool enable)
{
   int status =  mMediaStack->setAutomaticGainControl(enable);
   if(status != 0)
   {
      WarningLog(<< "enableAutoGainControl failed: status=" << status);
   }
}
 
void 
ConversationManager::enableNoiseReduction(bool enable)
{
   int status =  mMediaStack->setNoiseReduction(enable);
   if(status != 0)
   {
      WarningLog(<< "enableAutoGainControl failed: status=" << status);
   }
}

void
ConversationManager::startRecording(
      const ConversationHandle&  convHandle, const std::wstring& filePath,
      unsigned long width, unsigned long height,
      int videoFrameRate )
{
   class StartRecordCmd : public DumCommandStub
   {
   public:
      StartRecordCmd( ConversationManager* cmgr, ConversationHandle convHandle,
         const std::wstring& filePath, unsigned long width, unsigned long height, int videoFrameRate )
         : ConMgr( cmgr ), mConvHandle(convHandle), mFilePath(filePath), mWidth(width), mHeight(height), mVideoFrameRate(videoFrameRate) {}
      virtual void executeCommand()
      {
         // Find the conversation
         Conversation* pConv = ConMgr->getConversation(mConvHandle);
         if( pConv == NULL )
            return;

         // Get the associated mixer
         boost::shared_ptr<Mixer> pMixer = pConv->getMixer();
         if( pMixer.get() == NULL )
            return;

         pMixer->startRecording(mFilePath, mWidth, mHeight, mVideoFrameRate);
      }
   private:
      ConversationManager *ConMgr;
      ConversationHandle  mConvHandle;
      const std::wstring  mFilePath;
      unsigned long       mWidth;
      unsigned long       mHeight;
      int                 mVideoFrameRate;
   };
   mDum->post(new StartRecordCmd( this, convHandle, filePath, width, height, videoFrameRate ));
}

void
ConversationManager::stopRecording(const ConversationHandle& convHandle)
{
   class StopRecordCmd : public DumCommandStub
   {
   public:
      StopRecordCmd( ConversationManager* cmgr, ConversationHandle convHandle )
         : ConMgr( cmgr ), mConvHandle(convHandle) {}
      virtual void executeCommand()
      {
         // Find the conversation
         Conversation* pConv = ConMgr->getConversation(mConvHandle);
         if( pConv == NULL )
            return;

         // Get the associated mixer
         boost::shared_ptr<Mixer> pMixer = pConv->getMixer();
         if( pMixer.get() == NULL )
            return;

         pMixer->stopRecording();
      }
   private:
      ConversationManager *ConMgr;
      ConversationHandle  mConvHandle;
   };
   mDum->post(new StopRecordCmd( this, convHandle ));
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

RemoteParticipant* 
ConversationManager::getRemoteParticipantFromMediaConnectionId(int mediaConnectionId)
{
   ParticipantMap::iterator i = mParticipants.begin();
   for(; i != mParticipants.end(); i++)
   {
      RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(i->second);
      if(remoteParticipant)
      {
         if(remoteParticipant->getMediaConnectionId() == mediaConnectionId)
         {
            return remoteParticipant;
         }
      }
   }
   return 0;
}

boost::shared_ptr<Mixer>
ConversationManager::getConversationMixer( ConversationHandle convHandle )
{
   boost::shared_ptr<Mixer> result;

   ConversationMap::iterator i = mConversations.find(convHandle);
   if(i != mConversations.end())
   {
      if ( i->second != NULL )
         result = i->second->getMixer();
   }

   return result;
}

Conversation*
ConversationManager::getConversation(ConversationHandle convHandle)
{
   Conversation* result = NULL;

   ConversationMap::iterator i = mConversations.find(convHandle);
   if(i != mConversations.end())
      result = i->second;

   return result;
}

void 
ConversationManager::addBufferToMediaResourceCache(resip::Data& name, resip::Data& buffer, int type)
{
   mMediaResourceCache.addToCache(name, buffer, type);
}

void 
ConversationManager::buildSessionCapabilities(bool includeAudio, bool includeVideo, resip::Data& ipaddress, resip::SdpContents& sessionCaps, const resip::Data& sessionName)
{
   if (ipaddress == resip::Data::Empty)
   {
      // just use the IP address that was previously specified;
      // this is intended for the case where we want to rebuild the codec list only
      // at the start of each conversation
      ipaddress = sessionCaps.session().origin().getAddress();
   }

   sessionCaps = SdpContents::Empty;  // clear out passed in SdpContents

   // Create Session Capabilities 
   // Note:  port, sessionId and version will be replaced in actual offer/answer
   // Build s=, o=, t=, and c= lines
   SdpContents::AddrType addrType = (ipaddress.find(":") == Data::npos ? SdpContents::IP4 : SdpContents::IP6);
   SdpContents::Session::Origin origin("-", 0 /* sessionId */, 0 /* version */, addrType, ipaddress);   // o=   
   SdpContents::Session session(0, origin, sessionName /* s= */);
   session.connection() = SdpContents::Session::Connection(addrType, ipaddress);  // c=
   session.addTime(SdpContents::Session::Time(0, 0));

   sdpcontainer::SdpMediaLine::CodecList codecs;
   mCodecFactory->getCodecs(includeAudio, includeVideo, codecs);

   sdpcontainer::SdpMediaLine::CodecList::const_iterator codecIter;

   // Auto-Create Session Codec Capabilities
   // Note:  port, and potentially payload id will be replaced in actual
   // offer/answer

   // Build Codecs and media offering
   SdpContents::Session::Medium audioMedium("audio", 0, 1, "RTP/AVP");
   audioMedium.encodeAttribsForStaticPLs() = false; // !jjg! todo: make a profile setting for this

   bool firstCodecAdded = false;
   bool anyCodecAdded = false;
   for(codecIter = codecs.begin() ; codecIter != codecs.end(); ++codecIter)
   {
      const sdpcontainer::SdpCodec& codec = (*codecIter);
      
      // Ensure this codec is an audio codec
      if (! isEqualNoCase( codec.getMimeType(), "audio" ))
         continue;

      SdpContents::Session::Codec sdpCodec(codec.getMimeSubtype(), codec.getPayloadType(), codec.getRate());

      if (Data(codec.getMimeSubtype()).lowercase().find("telephone-event") != Data::npos)
         sdpCodec.parameters() = Data("0-15");
      else
         sdpCodec.parameters() = codec.getFormatParameters();

      InfoLog(<< "Added audio codec to session capabilites:"
              << " id=" << codec.getMimeSubtype() 
              << " type=" << codec.getMimeType()
              << " rate=" << codec.getRate()
              << " plen=" << codec.getPacketTime()
              << " payloadid=" << codec.getPayloadType()
              << " fmtp=" << codec.getFormatParameters());

      audioMedium.addCodec(sdpCodec);
      anyCodecAdded = true;
      if(!firstCodecAdded)
      {
         firstCodecAdded = true;

         // 20 ms of speech per frame (note G711 has 10ms samples, so this is 2 samples per frame)
         // Note:  There are known problems with SDP and the ptime attribute.  For now we will choose an
         // appropriate ptime from the first codec
         //audioMedium.addAttribute("ptime", Data(codec.getFormatParameters()));  
      }
   }

   // Add the medium only if at least one codec was added
   if ( anyCodecAdded )
      session.addMedium(audioMedium);

   anyCodecAdded = false;
   SdpContents::Session::Medium videoMedium("video", 0, 1, "RTP/AVP");
   videoMedium.encodeAttribsForStaticPLs() = false; // !jjg! todo: make a profile setting for this

   for(codecIter = codecs.begin() ; codecIter != codecs.end(); ++codecIter)
   {
      const sdpcontainer::SdpCodec& codec = (*codecIter);

      // Ensure this codec is a video codec
      if (! isEqualNoCase( codec.getMimeType(), "video" ))
         continue;

      SdpContents::Session::Codec sdpCodec(codec.getMimeSubtype(), codec.getPayloadType(), codec.getRate());
      sdpCodec.parameters() = codec.getFormatParameters();

      InfoLog(<< "Added video codec to session capabilites:"
              << " id=" << codec.getMimeSubtype() 
              << " type=" << codec.getMimeType()
              << " rate=" << codec.getRate()
              << " payloadid=" << codec.getPayloadType()
              << " fmtp=" << codec.getFormatParameters());

      videoMedium.addCodec(sdpCodec);
      anyCodecAdded = true;
   }

   if ( anyCodecAdded )
      session.addMedium(videoMedium);

   sessionCaps.session() = session;
}

void 
ConversationManager::onMediaEvent(MediaEvent::MediaEventType eventType)
{
   assert(eventType == MediaEvent::PLAY_FINISHED);

   if(eventType == MediaEvent::PLAY_FINISHED)
   {
      // Using current sipX architecture is it only possible to have one active media participant
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
            presult = mDum->findInviteSession(msg.header(h_TargetDialog));
            if(!(presult.first == InviteSessionHandle::NotValid())) 
            {         
               RemoteParticipant* participantToRefer = (RemoteParticipant*)presult.first->getAppDialog().get();

               participantToRefer->onRefer(presult.first, ss, msg);
               return;
            }
         }

         // application level policy
         if (isOutOfDialogReferSupported())
         {
            ss->send(ss->accept(202));

            // Create new Participant
            RemoteParticipantDialogSet *participantDialogSet = new RemoteParticipantDialogSet(*this);
            RemoteParticipant *participant = participantDialogSet->createUACOriginalRemoteParticipant(getNewParticipantHandle());  

            // Set pending OOD info in Participant - call to be triggered by app
            participant->setPendingOODReferInfo(ss, msg);

            // Notify application
            onRequestOutgoingParticipant(participant->getParticipantHandle(), msg);
         }
         else
         {
            ss->send(ss->reject(403));
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
ConversationManager::onRefresh(ServerSubscriptionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onRefresh(ServerSubscriptionHandle): " << msg.brief());
   if (resip::isEqualNoCase(h->getEventType(), "refer"))
   {
      // .jjg. always accept for 'refer' subscriptions
      h->send(h->accept(200));
      h->send(h->neutralNotify());
   }
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

      if (msg.exists(h_Accepts))
      {
         bool acceptsSdp = false;
         const Mimes& mimes = msg.header(h_Accepts);
         Mimes::const_iterator mit = mimes.begin();
         for (; mit != mimes.end(); ++mit)
         {
            if (*mit == Mime("application","sdp"))
            {
               acceptsSdp = true;
               break;
            }
         }
         if (acceptsSdp)
         {
            // Attach an offer to the options request
            SdpContents sdp;
            buildSdpOffer(getIncomingConversationProfile(msg).get(), sdp);
            optionsAnswer->setContents(&sdp);
         }
      }
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
               presult = mDum->findInviteSession(msg.header(h_TargetDialog));
               if(!(presult.first == InviteSessionHandle::NotValid())) 
               {         
                  RemoteParticipant* participantToRefer = (RemoteParticipant*)presult.first->getAppDialog().get();

                  // Accept the Refer
                  ood->send(ood->accept(202 /* Refer Accepted */));

                  participantToRefer->doReferNoSub(msg);
                  return;
               }
            }

            // application level policy
            if (isOutOfDialogReferSupported())
            {
               ood->send(ood->accept(202));

               // Create new Participant 
               RemoteParticipantDialogSet *participantDialogSet = new RemoteParticipantDialogSet(*this);
               RemoteParticipant *participant = participantDialogSet->createUACOriginalRemoteParticipant(getNewParticipantHandle());  

               // Set pending OOD info in Participant - application can choose to continue with accept or reject
               participant->setPendingOODReferInfo(ood, msg);

               // Notify application
               onRequestOutgoingParticipant(participant->getParticipantHandle(), msg);
            }
            else
            {
               ood->send(ood->reject(403));
            }
         }
         else
         {
            WarningLog (<< "Received refer w/out a Refer-To: " << msg.brief());
            ood->send(ood->reject(400));
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
ConversationManager::onRedirectReceived(AppDialogSetHandle h, const SipMessage& msg)
{
   InfoLog(<< "onRedirectReceived(AppDialogSetHandle): " << msg.brief());
   RemoteParticipantDialogSet *remoteParticipantDialogSet = dynamic_cast<RemoteParticipantDialogSet *>(h.get());
   if(remoteParticipantDialogSet)
   {
      if(remoteParticipantDialogSet->getNumDialogs() == 0)
      {
         remoteParticipantDialogSet->onRedirectReceived(h,msg);
      }
   }
}

bool 
ConversationManager::onTryingNextTarget(AppDialogSetHandle, const SipMessage& msg)
{
   InfoLog(<< "onTryingNextTarget(AppDialogSetHandle): " << msg.brief());
   // Always allow redirection for now
   return true;
}


bool
ConversationManager::isOutOfDialogReferSupported() const
{
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
