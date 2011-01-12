

#include "ConversationManager.hxx"
#include "ReconSubsystem.hxx"
#include "RemoteParticipantDialogSet.hxx"
#include "RemoteParticipant.hxx"
#include "Conversation.hxx"
#include "UserAgent.hxx"
#include "MediaStreamEvent.hxx"

// Flowmanager Includes
#include "FlowManager.hxx"
#include "Flow.hxx"
#include "MediaStream.hxx"

#include "sdp/SdpHelperResip.hxx"
#include "sdp/Sdp.hxx"
#include "sdp/SdpMediaLine.hxx"
#include "sdp/SdpCandidate.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/Random.hxx>
#include <rutil/DnsUtil.hxx>
#include <resip/stack/SipFrag.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/ServerInviteSession.hxx>

//#define DISABLE_FLOWMANAGER_IF_NO_NAT_TRAVERSAL
#include <rutil/WinLeakCheck.hxx>

#ifdef WIN32
   #define sleepMs(t) Sleep(t)
#else
   #define sleepMs(t) usleep(t*1000)
#endif

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

RemoteParticipantDialogSet::RemoteParticipantDialogSet(ConversationManager& conversationManager,
                                                       ConversationManager::ParticipantForkSelectMode forkSelectMode) :
   AppDialogSet(*conversationManager.mDum),
   mConversationManager(conversationManager),
   mUACOriginalRemoteParticipant(0),
   mNumDialogs(0),
   mForkSelectMode(forkSelectMode),
   mUACConnectedDialogId(Data::Empty, Data::Empty, Data::Empty),
   mUACRedirectedDialogId(Data::Empty, Data::Empty, Data::Empty),
   mActiveRemoteParticipantHandle(0),
   mNatTraversalMode(flowmanager::MediaStream::NoNatTraversal),
   mProposedSdp(0),
   mSecureMediaMode(ConversationManager::NoSecureMedia),
   mSecureMediaRequired(false),
   mMediaConnectionId(0),
   mConnectionPortOnBridge(-1),
   mAudioDirection(ConversationManager::MediaDirection_SendReceive),
   mVideoDirection(ConversationManager::MediaDirection_None)
{
   mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_NONE ]        = 0;
   mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_AUDIO ]       = 0;
   mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_VIDEO ]       = 0;
   mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_TEXT ]        = 0;
   mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_APPLICATION ] = 0;
   mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_MESSAGE ]     = 0;

   resetIceAttribs();

   InfoLog(<< "RemoteParticipantDialogSet created.");
}

RemoteParticipantDialogSet::~RemoteParticipantDialogSet()
{
   if(mMediaConnectionId)
   {
      // !jjg! let the media stuff clean itself up?
      //mConversationManager.getMediaInterface()->deleteConnection(mMediaConnectionId);
   }

   RTPPortAllocator* rtpAllocator = mConversationManager.mRTPAllocator;

   // todo: Probably all this port freeing stuff can be done in a loop
   if(mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_AUDIO ] != 0)
   {
      rtpAllocator->freeRTPPort(mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_AUDIO ]);
      mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_AUDIO ] = 0;

      // Probably this check is redundant, but just to be safe
      if( mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_AUDIO ]->isSendingRtp() )
         mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_AUDIO ]->stopRtpSend();

      // Probably this check is redundant, but just to be safe
      if( mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_AUDIO ]->isReceivingRtp() )
         mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_AUDIO ]->stopRtpReceive();
   }

   if(mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_VIDEO ] != 0)
   {
      rtpAllocator->freeRTPPort(mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_VIDEO ]);
      mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_VIDEO ] = 0;

      // Probably this check is redundant, but just to be safe
      if( mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_VIDEO ]->isSendingRtp() )
         mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_VIDEO ]->stopRtpSend();

      // Probably this check is redundant, but just to be safe
      if( mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_VIDEO ]->isReceivingRtp() )
         mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_VIDEO ]->stopRtpReceive();
   }

   if(mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_TEXT ] != 0)
   {
      rtpAllocator->freeRTPPort(mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_TEXT ]);
      mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_TEXT ] = 0;

      // Probably this check is redundant, but just to be safe
      if( mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_TEXT ]->isSendingRtp() )
         mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_TEXT ]->stopRtpSend();

      // Probably this check is redundant, but just to be safe
      if( mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_TEXT ]->isReceivingRtp() )
         mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_TEXT ]->stopRtpReceive();
   }

   if(mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_APPLICATION ] != 0)
   {
      rtpAllocator->freeRTPPort(mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_APPLICATION ]);
      mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_APPLICATION ] = 0;

      // Probably this check is redundant, but just to be safe
      if( mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_APPLICATION ]->isSendingRtp() )
         mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_APPLICATION ]->stopRtpSend();

      // Probably this check is redundant, but just to be safe
      if( mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_APPLICATION ]->isReceivingRtp() )
         mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_APPLICATION ]->stopRtpReceive();
   }

   if(mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_MESSAGE ] != 0)
   {
      rtpAllocator->freeRTPPort(mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_MESSAGE ]);
      mLocalRTPPortMap[ SdpMediaLine::MEDIA_TYPE_MESSAGE ] = 0;

      // Probably this check is redundant, but just to be safe
      if( mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_MESSAGE ]->isSendingRtp() )
         mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_MESSAGE ]->stopRtpSend();

      // Probably this check is redundant, but just to be safe
      if( mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_MESSAGE ]->isReceivingRtp() )
         mRtpStreamMap[ SdpMediaLine::MEDIA_TYPE_MESSAGE ]->stopRtpReceive();
   }

   // If we have no dialogs and mUACOriginalRemoteParticipant is set, then we have not passed
   // ownership of mUACOriginalRemoteParticipant to DUM - so we need to delete the participant
   if(mNumDialogs == 0 && mUACOriginalRemoteParticipant)
   {
      delete mUACOriginalRemoteParticipant;
   }

   // Delete Media Stream
   for (MediaStreamMap::const_iterator msIt = mMediaStreamMap.begin(); msIt != mMediaStreamMap.end(); ++msIt)
   {
      delete (flowmanager::MediaStream*)(msIt->second);
   }

   // Delete Sdp memory
   if(mProposedSdp) delete mProposedSdp;

   mRtpStreamMap.clear();

   InfoLog(<< "RemoteParticipantDialogSet destroyed.  mActiveRemoteParticipantHandle=" << mActiveRemoteParticipantHandle);
}

SharedPtr<UserProfile>
RemoteParticipantDialogSet::selectUASUserProfile(const SipMessage& msg)
{
   return mConversationManager.getIncomingConversationProfile(msg);
}

unsigned int
RemoteParticipantDialogSet::getLocalRTPPort( const sdpcontainer::SdpMediaLine::SdpMediaType& mediaType, ConversationProfile* profile /* = NULL */ )
{
   if(mLocalRTPPortMap[ mediaType ] == 0)
   {
      RTPPortAllocator* rtpAllocator = mConversationManager.mRTPAllocator;
      bool isUAC = false;

      unsigned int ulRTPPort = 0, ulRTCPPort = 0;
      if( !rtpAllocator->allocateRTPPort( ulRTPPort, ulRTCPPort ))
      {
         WarningLog(<< "Could not allocate a free RTP port for RemoteParticipantDialogSet!");
         return 0;
      }

      mLocalRTPPortMap[ mediaType ] = ulRTPPort;
      InfoLog(<< "Port allocated: " << mLocalRTPPortMap[ mediaType ]);

      // UAS Dialogs should have a user profile at this point - for UAC to get default outgoing
      if ( profile == NULL )
      {
         profile = mConversationManager.getDefaultOutgoingConversationProfile().get();
         isUAC = true;
      }

      // Create localBinding Tuple - note:  transport may be changed depending on NAT traversal mode
      StunTuple localBinding(StunTuple::UDP, boost::asio::ip::address::from_string(profile->sessionCaps().session().connection().getAddress().c_str()), mLocalRTPPortMap[ mediaType ]);

      switch(profile->natTraversalMode())
      {
      case ConversationProfile::Ice:
         // UDP with Stun Binding discovery, plus ICE candidates and connectivity checks
         mNatTraversalMode = flowmanager::MediaStream::Ice;
         break;
      case ConversationProfile::StunBindDiscovery:
         // Use straight UDP with Stun Binding discovery
         mNatTraversalMode = flowmanager::MediaStream::StunBindDiscovery;
         break;
      case ConversationProfile::TurnUdpAllocation:
         // Use Udp turn media relay
         mNatTraversalMode = flowmanager::MediaStream::TurnAllocation;
         break;
      case ConversationProfile::TurnTcpAllocation:
         // Use Tcp turn media relay
         localBinding.setTransportType(StunTuple::TCP);
         mNatTraversalMode = flowmanager::MediaStream::TurnAllocation;
         break;
      case ConversationProfile::TurnTlsAllocation:
         // Use TLS turn media relay
         localBinding.setTransportType(StunTuple::TLS);
         mNatTraversalMode = flowmanager::MediaStream::TurnAllocation;
         break;
      case ConversationProfile::NoNatTraversal:
      default:
         // Use straight UDP
         mNatTraversalMode = flowmanager::MediaStream::NoNatTraversal;
         break;
      }

      // Set other Srtp properties
      mLocalSrtpSessionKey = Random::getCryptoRandom(SRTP_MASTER_KEY_LEN);

#ifdef DISABLE_FLOWMANAGER_IF_NO_NAT_TRAVERSAL
      if(mNatTraversalMode != MediaStream::NoNatTraversal)
      {
#endif
         flowmanager::MediaStream* ms = mConversationManager.getFlowManager().createMediaStream(
                     *this,
                     localBinding,
                     true /* rtcp? */,
                     mNatTraversalMode,
                     profile->natTraversalServerHostname().c_str(),
                     profile->natTraversalServerPort(),
                     profile->stunUsername().c_str(),
                     profile->stunPassword().c_str());
         mMediaStreamMap[mediaType] = ms;

         // New Remote Participant - create media Interface connection
         MediaStack::MediaType mediaStackMediaType = (mediaType == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO ? MediaStack::MediaType_Audio : MediaStack::MediaType_Video);
         boost::shared_ptr<RtpStream> rtpStream = mConversationManager.getMediaStack()->createRtpStream(mediaStackMediaType, ms->getRtpFlow(), ms->getRtcpFlow(),
            mediaType == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO ? profile->audioDataQOS() : profile->videoDataQOS(),
            profile->signalingQOS());
         mRtpStreamMap[ mediaType ] = rtpStream;
#ifdef DISABLE_FLOWMANAGER_IF_NO_NAT_TRAVERSAL
      }
      else
      {
         // !jjg! fixme
         ret = mConversationManager.getMediaInterface()->createConnection(mMediaConnectionId,
                                                                 profile->sessionCaps().session().connection().getAddress().c_str(),
                                                                 mLocalRTPPort);
         mRtpTuple = localBinding;  // Just treat media stream as immediately ready using the localBinding in the SDP
      }
#endif
   }

   return mLocalRTPPortMap[ mediaType ];
}

sdpcontainer::SdpMediaLine::SdpMediaType
RemoteParticipantDialogSet::getMediaStreamType(flowmanager::MediaStream* ms)
{
   MediaStreamMap::const_iterator it = mMediaStreamMap.begin();
   for (; it != mMediaStreamMap.end(); ++it)
   {
      if (it->second == ms)
      {
         return it->first;
      }
   }
   return sdpcontainer::SdpMediaLine::MEDIA_TYPE_NONE;
}

void
RemoteParticipantDialogSet::processMediaStreamReadyEvent(flowmanager::MediaStream* ms, const StunTuple& rtpTuple, const StunTuple& rtcpTuple)
{
   sdpcontainer::SdpMediaLine::SdpMediaType mediaType = getMediaStreamType(ms);

   InfoLog( << "processMediaStreamReadyEvent: rtpTuple=" << rtpTuple << " rtcpTuple=" << rtcpTuple);
   mRtpTupleMap[mediaType] = rtpTuple;
   mRtcpTupleMap[mediaType] = rtcpTuple;   // Check if we had operations pending on the media stream being ready

   if (allStreamsReady())
   {
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
         assert(mPendingOfferAnswer.mSdp.get() == 0);
      }
   }
}

void
RemoteParticipantDialogSet::processMediaStreamErrorEvent(flowmanager::MediaStream* ms, unsigned int errorCode)
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
      resip::Data endReason;
      {
         resip::DataStream ds(endReason);
         ds << "media stream error: " << errorCode;
      }
      std::map<DialogId, RemoteParticipant*>::iterator it;
      for(it = mDialogs.begin(); it != mDialogs.end(); it++)
      {
         it->second->destroyParticipant(endReason);
      }
   }
   else
   {
      end();
   }
}

void
RemoteParticipantDialogSet::onMediaStreamReady(flowmanager::MediaStream* ms, const StunTuple& rtpTuple, const StunTuple& rtcpTuple)
{
   // Get event into dum queue, so that callback is on dum thread
   MediaStreamReadyEvent* event = new MediaStreamReadyEvent(getHandle(), ms, rtpTuple, rtcpTuple);
   mDum.post(event);
}

void
RemoteParticipantDialogSet::onMediaStreamError(flowmanager::MediaStream* ms, unsigned int errorCode)
{
   // Get event into dum queue, so that callback is on dum thread
   MediaStreamErrorEvent* event = new MediaStreamErrorEvent(getHandle(), ms, errorCode);
   mDum.post(event);
}

bool
RemoteParticipantDialogSet::allStreamsReady()
{
   unsigned int numMediaStreams = 0;
   MediaStreamMap::const_iterator msIt = mMediaStreamMap.begin();
   for (; msIt != mMediaStreamMap.end(); ++msIt)
   {
      if ((flowmanager::MediaStream*)msIt->second)
      {
         numMediaStreams++;
      }
   }
   bool allReady = (mRtpTupleMap.size() == numMediaStreams);

   for (StunTupleMap::const_iterator tupleIt = mRtpTupleMap.begin(); tupleIt != mRtpTupleMap.end(); ++tupleIt)
   {
      if (tupleIt->second.getTransportType() == reTurn::StunTuple::None)
      {
         allReady = false;
         break;
      }
   }
   return allReady;
}

void
RemoteParticipantDialogSet::sendInvite(SharedPtr<SipMessage> invite)
{
   if(allStreamsReady())
   {
      doSendInvite(invite);
   }
   else
   {
      // Wait until media stream is ready
      mPendingInvite = invite;
   }
}

UInt32 
computeCandidatePriority(unsigned int typePref, unsigned int localPref, unsigned int componentId)
{
   UInt32 priority = (2^24)*typePref + (2^8)*localPref + (2^0)*(256 - componentId);
   return priority;
}

void
RemoteParticipantDialogSet::setIceCandidates(resip::SdpContents::Session::Medium& medium, 
                                             const resip::Data& hostIp, unsigned int hostPort,
                                             const resip::Data& rtpSrflxIp, unsigned int rtpSrflxPort,
                                             const resip::Data& rtcpSrflxIp, unsigned int rtcpSrflxPort)
{
   medium.clearAttribute("candidate");
   bool bLocalCandidatesOnly = false;
   if (resip::isEqualNoCase(hostIp, rtpSrflxIp) || resip::isEqualNoCase(hostIp, rtcpSrflxIp))
   {
      // STUN probably failed, but we still need to include candidates for our host IP/ports since
      // the other end may be behind a NAT and we need them to open up a pinhole by attempting 
      // connectivity checks
      bLocalCandidatesOnly = true;
   }

   // first do RTP candidates
   resip::Data localCandidate;
   {
      UInt32 priority = computeCandidatePriority(126, 65535, 1);
      resip::DataStream localCandidateDS(localCandidate);
      localCandidateDS << "1 1 UDP " << priority << " " << hostIp << " " << hostPort << " typ host";
   }
   resip::Data publicCandidate;
   {
      UInt32 priority = computeCandidatePriority(124, 65535, 1); // note that 125 is reserved for peer reflexive
      resip::DataStream publicCandidateDS(publicCandidate);
      publicCandidateDS << "2 1 UDP " << priority << " " << rtpSrflxIp << " " << rtpSrflxPort << " typ srflx raddr " << hostIp << " rport " << hostPort;
   }
   medium.addAttribute("candidate", localCandidate);
   if (!bLocalCandidatesOnly)
   {
      medium.addAttribute("candidate", publicCandidate);
   }

   // now RTCP candidates
   localCandidate = resip::Data::Empty;
   {
      UInt32 priority = computeCandidatePriority(126, 65535, 2);
      resip::DataStream localCandidateDS(localCandidate);
      localCandidateDS << "1 2 UDP " << priority << " " << hostIp << " " << hostPort+1 << " typ host";
   }
   publicCandidate = resip::Data::Empty;
   {
      UInt32 priority = computeCandidatePriority(124, 65535, 2); // note that 125 is reserved for peer reflexive
      resip::DataStream publicCandidateDS(publicCandidate);
      publicCandidateDS << "2 2 UDP " << priority << " " << rtcpSrflxIp << " " << rtcpSrflxPort << " typ srflx raddr " << hostIp << " rport " << hostPort+1;
   }
   medium.addAttribute("candidate", localCandidate);
   if (!bLocalCandidatesOnly)
   {
      medium.addAttribute("candidate", publicCandidate);
   }
}

void
RemoteParticipantDialogSet::resetIceAttribs()
{
   mIceUFrag = resip::Random::getCryptoRandomHex(3);
   mIcePwd = resip::Random::getCryptoRandomHex(16);
}

void
RemoteParticipantDialogSet::setIceUsernameAndPassword(resip::SdpContents::Session& session)
{
   ConversationProfile* profile = dynamic_cast<ConversationProfile*>(getUserProfile().get());
   if (profile && profile->natTraversalMode() == ConversationProfile::Ice)
   {
      assert(!mIceUFrag.empty() && !mIcePwd.empty());
      session.clearAttribute("ice-ufrag");
      session.clearAttribute("ice-pwd");
      session.addAttribute("ice-ufrag", mIceUFrag/*resip::Random::getCryptoRandomHex(3)*/);
      session.addAttribute("ice-pwd", mIcePwd/*resip::Random::getCryptoRandomHex(16)*/);

      MediaStreamMap::iterator msIt = mMediaStreamMap.begin();
      for (; msIt != mMediaStreamMap.end(); ++msIt)
      {
         msIt->second->setLocalIcePassword(mIcePwd);
      }
   }
}

void
RemoteParticipantDialogSet::setAddressFromStunResult(resip::SdpContents* sdp)
{
   // Fix up address and port in SDP if we have remote info
   // Note:  the only time we don't is if there was an error preparing the media stream
   ConversationProfile* profile = dynamic_cast<ConversationProfile*>(getUserProfile().get());
   bool useOldStyleHold = false;
   bool iceEnabled = false;
   if (profile)
   {
      useOldStyleHold = profile->useRfc2543Hold();
      iceEnabled = (profile->natTraversalMode() == ConversationProfile::Ice);
   }

   if (sdp)
   {
      std::list<resip::SdpContents::Session::Medium>::iterator mediaIt = sdp->session().media().begin();
      for (; mediaIt != sdp->session().media().end(); ++mediaIt)
      {
         if (isEqualNoCase(mediaIt->name(), "audio"))
         {
            StunTupleMap::const_iterator rtpTupleIt = mRtpTupleMap.find(sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO);
            StunTupleMap::const_iterator rtcpTupleIt = mRtcpTupleMap.find(sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO);
            MediaStreamMap::const_iterator msIt = mMediaStreamMap.find(sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO);
            if (rtpTupleIt != mRtpTupleMap.end() && rtcpTupleIt != mRtcpTupleMap.end() && msIt != mMediaStreamMap.end())
            {
	            if (mediaIt->port() != 0) // 0 implies we have rejected this medium, e.g. due to a codec mismatch
	            {
                  mediaIt->port() = rtpTupleIt->second.getPort();

                  if (!useOldStyleHold || !resip::isEqualNoCase(sdp->session().connection().getAddress(), "0.0.0.0"))
                  {
                     // !jjg! kind of a hack, but it's more likely that we'd have audio only
                     sdp->session().connection() = SdpContents::Session::Connection(rtpTupleIt->second.getAddress().is_v4() ? SdpContents::IP4 : SdpContents::IP6, rtpTupleIt->second.getAddress().to_string().c_str());  // c=
                     mConnectionAddress = sdp->session().connection().getAddress();
                  }
                  if (iceEnabled)
                  {
                     flowmanager::Flow* rtpFlow = msIt->second->getRtpFlow();
                     setIceCandidates(*mediaIt, 
                        rtpFlow->getLocalTuple().getAddress().to_string().c_str(), rtpFlow->getLocalTuple().getPort(), 
                        rtpTupleIt->second.getAddress().to_string().c_str(), rtpTupleIt->second.getPort(), 
                        rtcpTupleIt->second.getAddress().to_string().c_str(), rtcpTupleIt->second.getPort());
                  }
               }
            }
         }
         else if (isEqualNoCase(mediaIt->name(), "video"))
         {
            StunTupleMap::const_iterator rtpTupleIt = mRtpTupleMap.find(sdpcontainer::SdpMediaLine::MEDIA_TYPE_VIDEO);
            StunTupleMap::const_iterator rtcpTupleIt = mRtcpTupleMap.find(sdpcontainer::SdpMediaLine::MEDIA_TYPE_VIDEO);
            MediaStreamMap::const_iterator msIt = mMediaStreamMap.find(sdpcontainer::SdpMediaLine::MEDIA_TYPE_VIDEO);
            if (rtpTupleIt != mRtpTupleMap.end() && rtcpTupleIt != mRtcpTupleMap.end() && msIt != mMediaStreamMap.end())
            {
               if (mediaIt->port() != 0) // 0 implies we have rejected this medium, e.g. due to a codec mismatch
               {
                  mediaIt->port() = rtpTupleIt->second.getPort();

                  if (iceEnabled)
                  {
                     flowmanager::Flow* rtpFlow = msIt->second->getRtpFlow();
                     setIceCandidates(*mediaIt, 
                        rtpFlow->getLocalTuple().getAddress().to_string().c_str(), rtpFlow->getLocalTuple().getPort(), 
                        rtpTupleIt->second.getAddress().to_string().c_str(), rtpTupleIt->second.getPort(),
                        rtcpTupleIt->second.getAddress().to_string().c_str(), rtcpTupleIt->second.getPort());
                  }
               }
            }
         }
      }
   }
}

void
RemoteParticipantDialogSet::doSendInvite(SharedPtr<SipMessage> invite)
{
   SdpContents* sdp  = dynamic_cast<SdpContents*>(invite->getContents());
   setIceUsernameAndPassword(sdp->session());
   setAddressFromStunResult(sdp);

   // Give the app a chance to adorn the INVITE
   mConversationManager.onReadyToSendInvite(mActiveRemoteParticipantHandle, *invite);

   // Send the invite
   mDum.send(invite);
}

void
RemoteParticipantDialogSet::provideOffer(std::auto_ptr<resip::SdpContents> offer, resip::InviteSessionHandle& inviteSessionHandle, bool postOfferAccept)
{
   if(allStreamsReady())
   {
      doProvideOfferAnswer(true /* offer */, offer, inviteSessionHandle, postOfferAccept, false);
   }
   else
   {
      assert(mPendingOfferAnswer.mSdp.get() == 0);
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
   if(allStreamsReady())
   {
      doProvideOfferAnswer(false /* offer */, answer, inviteSessionHandle, postAnswerAccept, postAnswerAlert);
   }
   else
   {
      assert(mPendingOfferAnswer.mSdp.get() == 0);
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
   if(inviteSessionHandle.isValid())
   {
      if (!inviteSessionHandle->isTerminated())
      {
         setIceUsernameAndPassword(sdp->session());
         setAddressFromStunResult(sdp.get());

         if(offer)
         {
            inviteSessionHandle->provideOffer(*sdp);
         }
         else
         {
            inviteSessionHandle->provideAnswer(*sdp);
         }

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

void
RemoteParticipantDialogSet::setIceRole(bool controlling)
{
   MediaStreamMap::iterator msIt = mMediaStreamMap.begin();
   for (; msIt != mMediaStreamMap.end(); ++msIt)
   {
      flowmanager::MediaStream* ms = msIt->second;

      if(ms && ms->getRtpFlow())
      {
         ms->getRtpFlow()->setIceRole(controlling);
         ms->getRtpFlow()->setPeerReflexiveCandidatePriority(computeCandidatePriority(125, 65535, 1));
      }
      if(ms && ms->getRtcpFlow())
      {
         ms->getRtcpFlow()->setIceRole(controlling);
         ms->getRtcpFlow()->setPeerReflexiveCandidatePriority(computeCandidatePriority(125, 65535, 2));
      }
   }
}

void 
RemoteParticipantDialogSet::setShortTermCredentials(sdpcontainer::SdpMediaLine::SdpMediaType mediaType, const resip::Data& username, const resip::Data& password)
{
   flowmanager::MediaStream* ms = mMediaStreamMap[mediaType];
   if (ms)
   {
      ms->setOutgoingIceUsernameAndPassword(username, password);
   }
}

void
RemoteParticipantDialogSet::setActiveDestination(sdpcontainer::SdpMediaLine::SdpMediaType mediaType, const char* address, unsigned short rtpPort, unsigned short rtcpPort, const sdpcontainer::SdpMediaLine::SdpCandidateList& candidates)
{
#ifdef DISABLE_FLOWMANAGER_IF_NO_NAT_TRAVERSAL
   if(mNatTraversalMode != MediaStream::NoNatTraversal)
   {
#endif
   flowmanager::MediaStream* ms = mMediaStreamMap[mediaType];

   std::vector<reTurn::IceCandidate> rtpCandidates;
   std::vector<reTurn::IceCandidate> rtcpCandidates;
   SdpMediaLine::SdpCandidateList::const_iterator it = candidates.begin();
   for (; it != candidates.end(); ++it)
   {
      // .jjg. todo - do even more validation here
      if (!(it->getId() == RTP_COMPONENT_ID || it->getId() == RTCP_COMPONENT_ID))
      {
         DebugLog(<< "ignoring ICE candidate with invalid component ID " << it->getId());
         continue;
      }
      StunTuple connAddr((it->getTransport() == SdpCandidate::CANDIDATE_TRANSPORT_TYPE_UDP ? StunTuple::UDP : StunTuple::TCP),
         boost::asio::ip::address::from_string(it->getConnectionAddress().c_str()),
         it->getPort());
      StunTuple relatedAddr;
      if (it->getRelatedAddress().size() > 0 && it->getRelatedPort() != 0)
      {
         relatedAddr.setTransportType(connAddr.getTransportType());
         relatedAddr.setAddress(boost::asio::ip::address::from_string(it->getRelatedAddress().c_str()));
         relatedAddr.setPort(it->getRelatedPort());
      }
      reTurn::IceCandidate c(connAddr, IceCandidate::CandidateType_Unknown, it->getPriority(), it->getFoundation(), it->getId(), relatedAddr);
      if (it->getId() == ms->getRtpFlow()->getComponentId())
      {
         rtpCandidates.push_back(c);
      }
      else if (it->getId() == ms->getRtcpFlow()->getComponentId())
      {
         rtcpCandidates.push_back(c);
      }
      else
      {
         ErrLog(<< "unrecognized componentId: " << it->getId());
         assert(0);
      }
   }
   if(ms && ms->getRtpFlow())
   {
      ms->getRtpFlow()->setActiveDestination(address, rtpPort, rtpCandidates);
   }
   if(ms && ms->getRtcpFlow())
   {
      ms->getRtcpFlow()->setActiveDestination(address, rtcpPort, rtcpCandidates);
   }
#ifdef DISABLE_FLOWMANAGER_IF_NO_NAT_TRAVERSAL
   }
   else
   {
   mConversationManager.getMediaInterface()->setConnectionDestination(mMediaConnectionId,
                                      address,
                                      rtpPort,  /* audio rtp port */
                                      rtcpPort, /* audio rtcp port */
                                      -1 /* video rtp port */,
                                      -1 /* video rtcp port */);
   }
#endif
}

#ifdef USE_DTLS
void
RemoteParticipantDialogSet::startDtlsClient(const char* address, unsigned short rtpPort, unsigned short rtcpPort)
{
   if(mMediaStream && mMediaStream->getRtpFlow())
   {
      mMediaStream->getRtpFlow()->startDtlsClient(address, rtpPort);
   }
   if(mMediaStream && mMediaStream->getRtcpFlow())
   {
      mMediaStream->getRtcpFlow()->startDtlsClient(address, rtcpPort);
   }
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
#endif // USE_DTLS

bool
RemoteParticipantDialogSet::createSRTPSession(sdpcontainer::SdpMediaLine::SdpMediaType mediaType, flowmanager::MediaStream::SrtpCryptoSuite cryptoSuite, const char* remoteKey, unsigned int remoteKeyLen)
{
   flowmanager::MediaStream* ms = mMediaStreamMap[mediaType];
   if(ms)
   {
      mSrtpCryptoSuite = cryptoSuite;  // update cypto suite to negotiated value
      ms->createOutboundSRTPSession(cryptoSuite, mLocalSrtpSessionKey.data(), mLocalSrtpSessionKey.size());
      return ms->createInboundSRTPSession(cryptoSuite, remoteKey, remoteKeyLen);
   }
   return false;
}

RemoteParticipant*
RemoteParticipantDialogSet::createUACOriginalRemoteParticipant(ParticipantHandle handle)
{
   assert(!mUACOriginalRemoteParticipant);
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
         RemoteParticipant* participant = new RemoteParticipant(mConversationManager, mDum, *this, mUACOriginalRemoteParticipant);

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
      DialogId dlgId(msg);
      if (!msg.header(h_To).exists(p_tag))
      {
         // we don't have a local tag assigned yet in the UAS case, so
         // just assign empty; we'll check for Empty again when we remove the dialog
         dlgId = DialogId(dlgId.getCallId(), Data::Empty, dlgId.getRemoteTag());
      }
      mDialogs[dlgId] = participant;
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
   assert(mUACConnectedDialogId.getCallId().empty());
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

void
RemoteParticipantDialogSet::setUACRedirected(const DialogId& dialogId)
{
   mUACRedirectedDialogId = dialogId;
}

void
RemoteParticipantDialogSet::destroyStaleDialogs(const DialogId& dialogId)
{
   if(mForkSelectMode == ConversationManager::ForkSelectAutomatic)
   {
      std::map<DialogId, RemoteParticipant*>::iterator it;
      for(it = mDialogs.begin(); it != mDialogs.end(); it++)
      {
         if(it->first != dialogId)
         {
            InfoLog(<< "3xx received on forked leg " << dialogId << " - stale dialog " << it->first << " and related conversation(s) will be ended.");
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
RemoteParticipantDialogSet::isUACRedirected()
{
   return !mUACRedirectedDialogId.getCallId().empty();
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
   if(it == mDialogs.end())
   {
      // try again, since the dialog ID stored in mDialogs might not have a local tag
      // if this is the UAS case
      DialogId dialogIdWithoutLocal(dialogId.getCallId(), Data::Empty, dialogId.getRemoteTag());
      it = mDialogs.find(dialogIdWithoutLocal);
   }

   if(it != mDialogs.end())
   {
      if(it->second == mUACOriginalRemoteParticipant) mUACOriginalRemoteParticipant = 0;
      mDialogs.erase(it);
   }

   // If we have no more dialogs and we never went connected - make sure we cancel the Invite transaction
   if(mDialogs.size() == 0 && !isUACConnected() && !isUACRedirected())
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
RemoteParticipantDialogSet::onNonDialogCreatingProvisional(resip::AppDialogSetHandle, const resip::SipMessage& msg)
{
   assert(msg.header(h_StatusLine).responseCode() != 100);
   // It possible to get a provisional from another fork after receiving a 200 - if so, don't generate an event
   if(!isUACConnected() && mUACOriginalRemoteParticipant)
   {
      InfoLog(<< "onNonDialogCreatingProvisional: handle=" << mUACOriginalRemoteParticipant->getParticipantHandle() << ", " << msg.brief());

      resip::SdpContents* sdp = dynamic_cast<resip::SdpContents*>(msg.getContents());
      if (sdp)
      {
         if (mUACOriginalRemoteParticipant->getParticipantHandle())
         {
            mUACOriginalRemoteParticipant->handleNonDialogCreatingProvisionalWithEarlyMedia(msg, *sdp);
         }
      }
      else
      {
         if(mUACOriginalRemoteParticipant->getParticipantHandle()) mConversationManager.onParticipantAlerting(mUACOriginalRemoteParticipant->getParticipantHandle(), msg);
      }
   }
}

void
RemoteParticipantDialogSet::onRedirectReceived(resip::AppDialogSetHandle, const resip::SipMessage& msg)
{
   assert(msg.header(h_StatusLine).responseCode() != 100);
   // It possible to get a provisional from another fork after receiving a 200 - if so, don't generate an event
   if(!isUACConnected() && !isUACRedirected() && mUACOriginalRemoteParticipant)
   {
      InfoLog(<< "onRedirectReceived: handle=" << mUACOriginalRemoteParticipant->getParticipantHandle() << ", " << msg.brief());

      if(mUACOriginalRemoteParticipant->getParticipantHandle())
      {
         resip::DialogId dialogId(msg);
         setUACRedirected(dialogId);
         destroyStaleDialogs(dialogId);
         mConversationManager.onLocalParticipantRedirected(mUACOriginalRemoteParticipant->getParticipantHandle(), msg);
      }
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
