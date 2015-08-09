#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// sipX includes
#if (_MSC_VER >= 1600)
#include <stdint.h>       // Use Visual Studio's stdint.h
#define _MSC_STDINT_H_    // This define will ensure that stdint.h in sipXport tree is not used
#endif
#include <CpTopologyGraphInterface.h>

#include "ConversationManager.hxx"
#include "ReconSubsystem.hxx"
#include "RemoteParticipantDialogSet.hxx"
#include "RemoteParticipant.hxx"
#include "Conversation.hxx"
#include "UserAgent.hxx"
#include "MediaStreamEvent.hxx"
#include "FlowManagerSipXSocket.hxx"

// Flowmanager Includes
#include "reflow/FlowManager.hxx"
#include "reflow/Flow.hxx"
#include "reflow/MediaStream.hxx"

#include "sdp/SdpHelperResip.hxx"
#include "sdp/Sdp.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/Random.hxx>
#include <rutil/DnsUtil.hxx>
#include <resip/stack/SipFrag.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/ServerInviteSession.hxx>

//#define DISABLE_FLOWMANAGER_IF_NO_NAT_TRAVERSAL
#include <rutil/WinLeakCheck.hxx>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

RemoteParticipantDialogSet::RemoteParticipantDialogSet(ConversationManager& conversationManager,        
                                                       ConversationManager::ParticipantForkSelectMode forkSelectMode) :
   AppDialogSet(conversationManager.getUserAgent()->getDialogUsageManager()),
   mConversationManager(conversationManager),
   mUACOriginalRemoteParticipant(0),
   mNumDialogs(0),
   mPeerExpectsSAVPF(false),
   mLocalRTPPort(0),
   mAllocateLocalRTPPortFailed(false),
   mForkSelectMode(forkSelectMode),
   mUACConnectedDialogId(Data::Empty, Data::Empty, Data::Empty),
   mActiveRemoteParticipantHandle(0),
   mNatTraversalMode(flowmanager::MediaStream::NoNatTraversal),
   mMediaStream(0),
   mRtpSocket(0),
   mRtcpSocket(0),
   mProposedSdp(0),
   mSecureMediaMode(ConversationProfile::NoSecureMedia),
   mSecureMediaRequired(false),
   mMediaConnectionId(0),
   mConnectionPortOnBridge(-1)
{

   InfoLog(<< "RemoteParticipantDialogSet created.");
}

RemoteParticipantDialogSet::~RemoteParticipantDialogSet() 
{
   freeMediaResources();

   // If we have no dialogs and mUACOriginalRemoteParticipant is set, then we have not passed 
   // ownership of mUACOriginalRemoteParticipant to DUM - so we need to delete the participant
   if(mNumDialogs == 0 && mUACOriginalRemoteParticipant)
   {
      delete mUACOriginalRemoteParticipant;
   }

   // Delete Sdp memory
   if(mProposedSdp) delete mProposedSdp;

   InfoLog(<< "RemoteParticipantDialogSet destroyed.  mActiveRemoteParticipantHandle=" << mActiveRemoteParticipantHandle);
}

SharedPtr<UserProfile> 
RemoteParticipantDialogSet::selectUASUserProfile(const SipMessage& msg)
{
   return mConversationManager.getUserAgent()->getIncomingConversationProfile(msg);
}

unsigned int 
RemoteParticipantDialogSet::getLocalRTPPort()
{
   if(mLocalRTPPort == 0 && !mAllocateLocalRTPPortFailed)
   {
      bool isUAC = false;
      mLocalRTPPort = mConversationManager.allocateRTPPort();
      if(mLocalRTPPort == 0)
      {
         WarningLog(<< "Could not allocate a free RTP port for RemoteParticipantDialogSet!");
         mAllocateLocalRTPPortFailed = true;
         return 0;
      }
      else
      {
         InfoLog(<< "Port allocated: " << mLocalRTPPort);
      }

      // UAS Dialogs should have a user profile at this point - for UAC to get default outgoing
      ConversationProfile* profile = dynamic_cast<ConversationProfile*>(getUserProfile().get());
      if(!profile)
      {
         profile = mConversationManager.getUserAgent()->getDefaultOutgoingConversationProfile().get();
         isUAC = true;
      }

      OsStatus ret;
      // Create localBinding Tuple - note:  transport may be changed depending on NAT traversal mode
      StunTuple localBinding(StunTuple::UDP, asio::ip::address::from_string(profile->sessionCaps().session().connection().getAddress().c_str()), mLocalRTPPort); 

      switch(profile->natTraversalMode())
      {
      case ConversationProfile::StunBindDiscovery:
         // Use straight UDP with Stun Binding discovery
         mNatTraversalMode = MediaStream::StunBindDiscovery;
         break;
      case ConversationProfile::TurnUdpAllocation:
         // Use Udp turn media relay
         mNatTraversalMode = MediaStream::TurnAllocation;
         break;
      case ConversationProfile::TurnTcpAllocation:
         // Use Tcp turn media relay
         localBinding.setTransportType(StunTuple::TCP);
         mNatTraversalMode = MediaStream::TurnAllocation;
         break;
#ifdef USE_SSL
      case ConversationProfile::TurnTlsAllocation:
         // Use TLS turn media relay
         localBinding.setTransportType(StunTuple::TLS);
         mNatTraversalMode = MediaStream::TurnAllocation;
         break;
#endif
      case ConversationProfile::NoNatTraversal:
      default:
         // Use straight UDP
         mNatTraversalMode = MediaStream::NoNatTraversal;
         break;
      }

#ifdef USE_SSL
      if(profile->secureMediaMode() == ConversationProfile::SrtpDtls &&
         mNatTraversalMode == MediaStream::TurnAllocation)
      {
         WarningLog(<< "You cannot use SrtpDtls and a Turn allocation at the same time - disabling SrtpDtls!");
         mSecureMediaMode = ConversationProfile::NoSecureMedia;
      }
      else
#endif
      {
         mSecureMediaMode = profile->secureMediaMode();
         mSecureMediaRequired = profile->secureMediaRequired();
      }      

      // Set other Srtp properties
      mLocalSrtpSessionKey = Random::getCryptoRandom(SRTP_MASTER_KEY_LEN);
      mSecureMediaRequired = profile->secureMediaRequired();
      
      switch(profile->secureMediaDefaultCryptoSuite())
      {
      case ConversationProfile::SRTP_AES_CM_128_HMAC_SHA1_32:
         mSrtpCryptoSuite = flowmanager::MediaStream::SRTP_AES_CM_128_HMAC_SHA1_32;
         break;
      default:
         mSrtpCryptoSuite = flowmanager::MediaStream::SRTP_AES_CM_128_HMAC_SHA1_80;
         break;
      }
 
#ifdef DISABLE_FLOWMANAGER_IF_NO_NAT_TRAVERSAL
      if(mNatTraversalMode != MediaStream::NoNatTraversal)
      {
#endif
         mMediaStream = mConversationManager.getFlowManager().createMediaStream(
                     *this, 
                     localBinding, 
                     true /* rtcp? */, 
                     mNatTraversalMode, 
                     profile->natTraversalServerHostname().c_str(), 
                     profile->natTraversalServerPort(), 
                     profile->stunUsername().c_str(), 
                     profile->stunPassword().c_str()); 

         // New Remote Participant - create media Interface connection
         mRtpSocket = new FlowManagerSipXSocket(mMediaStream->getRtpFlow(), mConversationManager.mSipXTOSValue);
         mRtcpSocket = new FlowManagerSipXSocket(mMediaStream->getRtcpFlow(), mConversationManager.mSipXTOSValue);

         ret = ((CpTopologyGraphInterface*)getMediaInterface()->getInterface())->createConnection(mMediaConnectionId,mRtpSocket,mRtcpSocket,false);
#ifdef DISABLE_FLOWMANAGER_IF_NO_NAT_TRAVERSAL
      }
      else
      {
         ret = getMediaInterface()->getInterface()->createConnection(mMediaConnectionId,
                                                     profile->sessionCaps().session().connection().getAddress().c_str(),
                                                     mLocalRTPPort);
         mRtpTuple = localBinding;  // Just treat media stream as immediately ready using the localBinding in the SDP
      }
#endif

      if(ret == OS_SUCCESS)
      {
         // Get the capabilities so that we can ensure there are codecs loaded
         UtlString rtpHostAddress;
         int rtpAudioPort;
         int rtcpAudioPort;
         int rtpVideoPort;
         int rtcpVideoPort;
         SdpCodecList supportedCodecs;
         SdpSrtpParameters srtpParameters;
         int bandWidth = 0;
         int videoBandwidth;
         int videoFramerate;

         ret = getMediaInterface()->getInterface()->getCapabilities(
            mMediaConnectionId, 
            rtpHostAddress, 
            rtpAudioPort,
            rtcpAudioPort,
            rtpVideoPort,
            rtcpVideoPort,
            supportedCodecs,
            srtpParameters,
            bandWidth,
            videoBandwidth,
            videoFramerate);

         if(ret == OS_SUCCESS)
         {
            if(supportedCodecs.getCodecCount() == 0)
            {
               ErrLog( << "No supported codecs!!!!!");
            }
         }
         else
         {
            ErrLog( << "Error getting connection capabilities, ret=" << ret);
         }
      }
      else
      {
         ErrLog( << "Error creating connection, ret=" << ret);
      }

      //InfoLog(<< "About to get Connection Port on Bridge for MediaConnectionId: " << mMediaConnectionId);
      ret = ((CpTopologyGraphInterface*)getMediaInterface()->getInterface())->getConnectionPortOnBridge(mMediaConnectionId, 0, mConnectionPortOnBridge);
      InfoLog( << "RTP Port allocated=" << mLocalRTPPort << " (sipXmediaConnectionId=" << mMediaConnectionId << ", BridgePort=" << mConnectionPortOnBridge << ", ret=" << ret << ")");
   }

   return mLocalRTPPort;
}

void 
RemoteParticipantDialogSet::processMediaStreamReadyEvent(const StunTuple& rtpTuple, const StunTuple& rtcpTuple)
{
   InfoLog( << "processMediaStreamReadyEvent: rtpTuple=" << rtpTuple << " rtcpTuple=" << rtcpTuple);
   mRtpTuple = rtpTuple;
   mRtcpTuple = rtcpTuple;   // Check if we had operations pending on the media stream being ready

   if(mPendingInvite.get() != 0)
   {
      // Pending Invite Request
      doSendInvite(mPendingInvite);
      mPendingInvite.reset();
   }

   if(mPendingOfferAnswer.mSdp.get() != 0)
   {
      // Pending Offer or Answer
      doProvideOfferAnswer(mPendingOfferAnswer.mOffer, 
                           mPendingOfferAnswer.mSdp, 
                           mPendingOfferAnswer.mInviteSessionHandle, 
                           mPendingOfferAnswer.mPostOfferAnswerAccept, 
                           mPendingOfferAnswer.mPostAnswerAlert);
      resip_assert(mPendingOfferAnswer.mSdp.get() == 0);
   }
}

void 
RemoteParticipantDialogSet::processMediaStreamErrorEvent(unsigned int errorCode)
{
   InfoLog( << "processMediaStreamErrorEvent, error=" << errorCode);

   // Note:  in the case of an initial invite we must issue the invite in order for dum to cleanup state
   //         properly - this is not ideal, since it may cause endpoint phone device to ring a little 
   //         before receiving the cancel
   if(mPendingInvite.get() != 0)
   {
      // Pending Invite Request - Falling back to using local address/port - but then end() immediate
      doSendInvite(mPendingInvite);
      mPendingInvite.reset();
   }

   // End call
   if(mNumDialogs > 0)
   {
      std::map<DialogId, RemoteParticipant*>::iterator it;
      for(it = mDialogs.begin(); it != mDialogs.end(); it++)
      {
         it->second->destroyParticipant();
      }
   }
   else
   {
      end();
   }
}

void 
RemoteParticipantDialogSet::onMediaStreamReady(const StunTuple& rtpTuple, const StunTuple& rtcpTuple)
{
   // Get event into dum queue, so that callback is on dum thread
   MediaStreamReadyEvent* event = new MediaStreamReadyEvent(*this, rtpTuple, rtcpTuple);
   mDum.post(event);
}

void 
RemoteParticipantDialogSet::onMediaStreamError(unsigned int errorCode)
{
   // Get event into dum queue, so that callback is on dum thread
   MediaStreamErrorEvent* event = new MediaStreamErrorEvent(*this, errorCode);
   mDum.post(event);
}

void 
RemoteParticipantDialogSet::sendInvite(SharedPtr<SipMessage> invite)
{
   if(mRtpTuple.getTransportType() != reTurn::StunTuple::None)
   {
      doSendInvite(invite);
   }
   else
   {
      // Wait until media stream is ready
      mPendingInvite = invite;
   }
}

void 
RemoteParticipantDialogSet::doSendInvite(SharedPtr<SipMessage> invite)
{
   // Fix up address and port in SDP if we have remote info
   // Note:  the only time we don't is if there was an error preparing the media stream
   if(mRtpTuple.getTransportType() != reTurn::StunTuple::None)
   {
      SdpContents* sdp  = dynamic_cast<SdpContents*>(invite->getContents());
      if (sdp)
      {
         sdp->session().media().front().port() = mRtpTuple.getPort();
         sdp->session().connection() = SdpContents::Session::Connection(mRtpTuple.getAddress().is_v4() ? SdpContents::IP4 : SdpContents::IP6, mRtpTuple.getAddress().to_string().c_str());  // c=
      }
   }

   // Send the invite
   mDum.send(invite);
}

void 
RemoteParticipantDialogSet::provideOffer(std::auto_ptr<resip::SdpContents> offer, resip::InviteSessionHandle& inviteSessionHandle, bool postOfferAccept)
{
   if(mRtpTuple.getTransportType() != reTurn::StunTuple::None)
   {
      doProvideOfferAnswer(true /* offer */, offer, inviteSessionHandle, postOfferAccept, false);
   }
   else
   {
      resip_assert(mPendingOfferAnswer.mSdp.get() == 0);
      mPendingOfferAnswer.mOffer = true;
      mPendingOfferAnswer.mSdp = offer;
      mPendingOfferAnswer.mInviteSessionHandle = inviteSessionHandle;
      mPendingOfferAnswer.mPostOfferAnswerAccept = postOfferAccept;
      mPendingOfferAnswer.mPostAnswerAlert = false;
   }
}

void 
RemoteParticipantDialogSet::provideAnswer(std::auto_ptr<resip::SdpContents> answer, resip::InviteSessionHandle& inviteSessionHandle, bool postAnswerAccept, bool postAnswerAlert)
{
   if(mRtpTuple.getTransportType() != reTurn::StunTuple::None)
   {
      doProvideOfferAnswer(false /* offer */, answer, inviteSessionHandle, postAnswerAccept, postAnswerAlert);
   }
   else
   {
      resip_assert(mPendingOfferAnswer.mSdp.get() == 0);
      mPendingOfferAnswer.mOffer = false;
      mPendingOfferAnswer.mSdp = answer;
      mPendingOfferAnswer.mInviteSessionHandle = inviteSessionHandle;
      mPendingOfferAnswer.mPostOfferAnswerAccept = postAnswerAccept;
      mPendingOfferAnswer.mPostAnswerAlert = postAnswerAlert;
   }
}

void 
RemoteParticipantDialogSet::doProvideOfferAnswer(bool offer, std::auto_ptr<resip::SdpContents> sdp, resip::InviteSessionHandle& inviteSessionHandle, bool postOfferAnswerAccept, bool postAnswerAlert)
{
   if(inviteSessionHandle.isValid() && !inviteSessionHandle->isTerminated())
   {
      // Fix up address and port in SDP if we have remote info
      // Note:  the only time we don't is if there was an error preparing the media stream
      if(mRtpTuple.getTransportType() != reTurn::StunTuple::None)
      {
         sdp->session().media().front().port() = mRtpTuple.getPort();
         sdp->session().connection() = SdpContents::Session::Connection(mRtpTuple.getAddress().is_v4() ? SdpContents::IP4 : SdpContents::IP6, mRtpTuple.getAddress().to_string().c_str());  // c=
      }

      if(offer)
      {
         inviteSessionHandle->provideOffer(*sdp);
      }
      else
      {
         inviteSessionHandle->provideAnswer(*sdp);
      }

      // Adjust RTP Streams
      dynamic_cast<RemoteParticipant*>(inviteSessionHandle->getAppDialog().get())->adjustRTPStreams(offer);

      // Do Post Answer Operations
      ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(inviteSessionHandle.get());
      if(sis)
      {
         if(postAnswerAlert)
         {
            sis->provisional(180,true);
         }
         if(postOfferAnswerAccept)
         {
            sis->accept();
         }
      }
   }
}

void 
RemoteParticipantDialogSet::accept(resip::InviteSessionHandle& inviteSessionHandle)
{
   // If we have a pending answer, then just flag to accept when complete
   if(mPendingOfferAnswer.mSdp.get() != 0 &&
      !mPendingOfferAnswer.mOffer)
   {
      mPendingOfferAnswer.mPostOfferAnswerAccept = true;
   }
   else if(inviteSessionHandle.isValid())
   {
      ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(inviteSessionHandle.get());
      if(sis)
      {
         sis->accept();
      }
   }
}

SharedPtr<MediaInterface>
RemoteParticipantDialogSet::getMediaInterface()
{
   if(!mMediaInterface)
   {
      // Get the media interface from the active participant
      if(mUACOriginalRemoteParticipant)
      {
         mMediaInterface = mUACOriginalRemoteParticipant->getMediaInterface();
      }
      else if(mDialogs.size() > 0)
      {
         // All participants in the set will have the same media interface - query from first
         resip_assert(mDialogs.begin()->second);
         mMediaInterface = mDialogs.begin()->second->getMediaInterface();
      }
   }
   resip_assert(mMediaInterface);
   return mMediaInterface;
}

int 
RemoteParticipantDialogSet::getConnectionPortOnBridge() 
{ 
   if(mConnectionPortOnBridge==-1)
   {
      getLocalRTPPort();  // This call will create a MediaConnection if not already created at this point
   }
   return mConnectionPortOnBridge; 
}

void 
RemoteParticipantDialogSet::freeMediaResources()
{
   if(mMediaConnectionId)
   {
      getMediaInterface()->getInterface()->deleteConnection(mMediaConnectionId);
      mMediaConnectionId = 0;
   }

   // Delete custom sockets - Note:  Must be done before MediaStream is deleted
   if(mRtpSocket)
   { 
      delete mRtpSocket;
      mRtpSocket = 0;
   }
   if(mRtcpSocket) 
   {
      delete mRtcpSocket;
      mRtcpSocket = 0;
   }

   // Delete Media Stream
   if(mMediaStream)
   {
      delete mMediaStream;
      mMediaStream = 0;
   }

   // Add the RTP port back to the pool
   if(mLocalRTPPort)
   {
      mConversationManager.freeRTPPort(mLocalRTPPort);
      mLocalRTPPort = 0;
   }
}

void 
RemoteParticipantDialogSet::setActiveDestination(const char* address, unsigned short rtpPort, unsigned short rtcpPort)
{
#ifdef DISABLE_FLOWMANAGER_IF_NO_NAT_TRAVERSAL
   if(mNatTraversalMode != MediaStream::NoNatTraversal)
   {
#else
   if(!mMediaStream)
      WarningLog(<<"mMediaStream == NULL, no RTP will be transmitted");
#endif
   if(mMediaStream && mMediaStream->getRtpFlow())
   {
      mMediaStream->getRtpFlow()->setActiveDestination(address, rtpPort);
   }
   if(mMediaStream && mMediaStream->getRtcpFlow())
   {
      mMediaStream->getRtcpFlow()->setActiveDestination(address, rtcpPort);
   }
#ifdef DISABLE_FLOWMANAGER_IF_NO_NAT_TRAVERSAL
   }
   else
   {
      getMediaInterface()->getInterface()->setConnectionDestination(mMediaConnectionId,
                                      address, 
                                      rtpPort,  /* audio rtp port */
                                      rtcpPort, /* audio rtcp port */
                                      -1 /* video rtp port */, 
                                      -1 /* video rtcp port */);
   }
#endif
}

void 
RemoteParticipantDialogSet::startDtlsClient(const char* address, unsigned short rtpPort, unsigned short rtcpPort)
{
#ifdef USE_SSL
   if(mMediaStream && mMediaStream->getRtpFlow())
   {
      mMediaStream->getRtpFlow()->startDtlsClient(address, rtpPort);
   }
   if(mMediaStream && mMediaStream->getRtcpFlow())
   {
      mMediaStream->getRtcpFlow()->startDtlsClient(address, rtcpPort);
   }
#endif
}

void 
RemoteParticipantDialogSet::setRemoteSDPFingerprint(const resip::Data& fingerprint)
{
   if(mMediaStream && mMediaStream->getRtpFlow())
   {
      mMediaStream->getRtpFlow()->setRemoteSDPFingerprint(fingerprint);
   }
   if(mMediaStream && mMediaStream->getRtcpFlow())
   {
      mMediaStream->getRtcpFlow()->setRemoteSDPFingerprint(fingerprint);
   }
}

bool 
RemoteParticipantDialogSet::createSRTPSession(MediaStream::SrtpCryptoSuite cryptoSuite, const char* remoteKey, unsigned int remoteKeyLen)
{
   if(mMediaStream)
   {
      mSrtpCryptoSuite = cryptoSuite;  // update cypto suite to negotiated value
      mMediaStream->createOutboundSRTPSession(cryptoSuite, mLocalSrtpSessionKey.data(), mLocalSrtpSessionKey.size());
      return mMediaStream->createInboundSRTPSession(cryptoSuite, remoteKey, remoteKeyLen);
   }
   WarningLog(<<"createSRTPSession: can't create SRTP session without media stream, mMediaStream = " << mMediaStream);
   return false;
}

RemoteParticipant* 
RemoteParticipantDialogSet::createUACOriginalRemoteParticipant(ParticipantHandle handle)
{
   resip_assert(!mUACOriginalRemoteParticipant);
   RemoteParticipant *participant = new RemoteParticipant(handle, mConversationManager, mDum, *this);  
   mUACOriginalRemoteParticipant = participant;
   mActiveRemoteParticipantHandle = participant->getParticipantHandle();  // Store this since it may not be safe to access mUACOriginalRemoteParticipant pointer after corresponding Dialog has been created
   return participant;
}

AppDialog* 
RemoteParticipantDialogSet::createAppDialog(const SipMessage& msg)
{
   mNumDialogs++;

   if(mUACOriginalRemoteParticipant)  // UAC DialogSet
   {
      // Need to either return participant already created, or create a new one.
      if(mNumDialogs > 1)
      {
         // forking occured and we now have multiple dialogs in this dialog set
         RemoteParticipant* participant = new RemoteParticipant(mConversationManager, mDum, *this);

         InfoLog(<< "Forking occurred for original UAC participant handle=" << mUACOriginalRemoteParticipant->getParticipantHandle() << 
                    " this is leg number " << mNumDialogs << " new handle=" << participant->getParticipantHandle());

         // Create Related Conversations for each conversation UACOriginalRemoteParticipant was in when first Dialog is created
         std::list<ConversationHandle>::iterator it;
         for(it = mUACOriginalConversationHandles.begin(); it != mUACOriginalConversationHandles.end(); it++)
         {
            Conversation* conversation = mConversationManager.getConversation(*it);
            if(conversation)
            {
               conversation->createRelatedConversation(participant, mActiveRemoteParticipantHandle);
            }
         }

         mDialogs[DialogId(msg)] = participant;
         return participant;
      }
      else
      {
         // Get Conversations from Remote Participant and store - used later for creating related conversations
         const Participant::ConversationMap& conversations = mUACOriginalRemoteParticipant->getConversations();
         Participant::ConversationMap::const_iterator it;
         for(it = conversations.begin(); it != conversations.end(); it++)
         {
            mUACOriginalConversationHandles.push_back(it->second->getHandle());
         }

         mDialogs[DialogId(msg)] = mUACOriginalRemoteParticipant;
         return mUACOriginalRemoteParticipant;
      }
   }
   else
   {
      RemoteParticipant *participant = new RemoteParticipant(mConversationManager, mDum, *this);
      mActiveRemoteParticipantHandle = participant->getParticipantHandle();
      mDialogs[DialogId(msg)] = participant;  // Note:  !slg! DialogId is not quite right here, since there is no To Tag on the INVITE
      return participant;
   }
}

void 
RemoteParticipantDialogSet::setProposedSdp(ParticipantHandle handle, const resip::SdpContents& sdp)
{
   if(mProposedSdp) delete mProposedSdp;
   mProposedSdp = 0;
   InfoLog(<< "setProposedSdp: handle=" << handle << ", proposedSdp=" << sdp);
   mProposedSdp = SdpHelperResip::createSdpFromResipSdp(sdp);
}

void 
RemoteParticipantDialogSet::setUACConnected(const DialogId& dialogId, ParticipantHandle partHandle)
{
   resip_assert(mUACConnectedDialogId.getCallId().empty());
   mUACConnectedDialogId = dialogId;
   mActiveRemoteParticipantHandle = partHandle;
   if(mForkSelectMode == ConversationManager::ForkSelectAutomatic)
   {
      std::map<DialogId, RemoteParticipant*>::iterator it;
      for(it = mDialogs.begin(); it != mDialogs.end(); it++)
      {
         if(it->first != dialogId)
         {
            InfoLog(<< "Connected to forked leg " << dialogId << " - stale dialog " << it->first << " and related conversation(s) will be ended.");      
            it->second->destroyConversations();
         }
      }
   }
}

bool 
RemoteParticipantDialogSet::isUACConnected()
{
   return !mUACConnectedDialogId.getCallId().empty();
}

bool 
RemoteParticipantDialogSet::isStaleFork(const DialogId& dialogId)
{
   return (!mUACConnectedDialogId.getCallId().empty() && dialogId != mUACConnectedDialogId);
}

void 
RemoteParticipantDialogSet::removeDialog(const DialogId& dialogId)
{
   std::map<DialogId, RemoteParticipant*>::iterator it = mDialogs.find(dialogId);
   if(it != mDialogs.end())
   {
      if(it->second == mUACOriginalRemoteParticipant) mUACOriginalRemoteParticipant = 0;
      mDialogs.erase(it);
   }

   // If we have no more dialogs and we never went connected - make sure we cancel the Invite transaction
   if(mDialogs.size() == 0 && !isUACConnected())
   {
      end();
   }
}

ConversationManager::ParticipantForkSelectMode 
RemoteParticipantDialogSet::getForkSelectMode()
{
   return mForkSelectMode;
}

////////////////////////////////////////////////////////////////////////////////
// DialogSetHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
RemoteParticipantDialogSet::onTrying(AppDialogSetHandle, const SipMessage& msg)
{
   if(!isUACConnected() && mUACOriginalRemoteParticipant)
   {
      InfoLog(<< "onTrying: handle=" << mUACOriginalRemoteParticipant->getParticipantHandle() << ", " << msg.brief());
      //mConversationManager.onParticipantProceeding(mHandle, msg);
   }
}

void 
RemoteParticipantDialogSet::onNonDialogCreatingProvisional(AppDialogSetHandle, const SipMessage& msg)
{
   resip_assert(msg.header(h_StatusLine).responseCode() != 100);
   // It possible to get a provisional from another fork after receiving a 200 - if so, don't generate an event
   if(!isUACConnected() && mUACOriginalRemoteParticipant)
   {
      InfoLog(<< "onNonDialogCreatingProvisional: handle=" << mUACOriginalRemoteParticipant->getParticipantHandle() << ", " << msg.brief());
      if(mUACOriginalRemoteParticipant->getParticipantHandle()) mConversationManager.onParticipantAlerting(mUACOriginalRemoteParticipant->getParticipantHandle(), msg);
   }
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
