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
#include "SipXRemoteParticipantDialogSet.hxx"
#include "SipXRemoteParticipant.hxx"
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

#include <utility>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

SipXRemoteParticipantDialogSet::SipXRemoteParticipantDialogSet(SipXConversationManager& sipXConversationManager,
                                                       ConversationManager::ParticipantForkSelectMode forkSelectMode,
                                                       std::shared_ptr<ConversationProfile> conversationProfile) :
   RemoteParticipantDialogSet(sipXConversationManager, forkSelectMode, conversationProfile),
   mSipXConversationManager(sipXConversationManager),
   mLocalRTPPort(0),
   mAllocateLocalRTPPortFailed(false),
   mFlowContext(new FlowContext()),
   mPeerExpectsSAVPF(false),
   mNatTraversalMode(flowmanager::MediaStream::NoNatTraversal),
   mMediaStream(0),
   mRtpSocket(0),
   mRtcpSocket(0),
   mMediaConnectionId(0),
   mConnectionPortOnBridge(-1)
{

   InfoLog(<< "SipXRemoteParticipantDialogSet created.");
}

SipXRemoteParticipantDialogSet::~SipXRemoteParticipantDialogSet()
{
   freeMediaResources();
   InfoLog(<< "SipXRemoteParticipantDialogSet destroyed.  mActiveRemoteParticipantHandle=" << getActiveRemoteParticipantHandle());
}

unsigned int 
SipXRemoteParticipantDialogSet::getLocalRTPPort()
{
   if(mLocalRTPPort == 0 && !mAllocateLocalRTPPortFailed)
   {
      mLocalRTPPort = mSipXConversationManager.getRTPPortManager()->allocateRTPPort();
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
         DebugLog(<<"no ConversationProfile in DialogSet::mUserProfile");
         profile = getConversationProfile().get();
      }
      if(!profile)
      {
         DebugLog(<<"no ConversationProfile in RemoteParticipantDialogSet::mConversationProfile, falling back to default for UAC");
         profile = mSipXConversationManager.getUserAgent()->getDefaultOutgoingConversationProfile().get();
      }

      OsStatus ret;
      Data connectionAddr = profile->sessionCaps().session().connection().getAddress();
      DebugLog(<< "getLocalRTPPort: Using local connection address: " << connectionAddr);
      // Create localBinding Tuple - note:  transport may be changed depending on NAT traversal mode
      StunTuple localBinding(StunTuple::UDP, asio::ip::address::from_string(connectionAddr.c_str()), mLocalRTPPort);

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
         setSecureMediaMode(ConversationProfile::NoSecureMedia);
      }
      else
#endif
      {
         setSecureMediaMode(profile->secureMediaMode());
         setSecureMediaRequired(profile->secureMediaRequired());
      }      

      // Set other Srtp properties
      mLocalSrtpSessionKey = Random::getCryptoRandom(SRTP_MASTER_KEY_LEN);
      setSecureMediaRequired(profile->secureMediaRequired());
      
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
         mMediaStream = mSipXConversationManager.getFlowManager().createMediaStream(
                     *this, 
                     localBinding, 
                     true /* rtcp? */, 
                     mNatTraversalMode, 
                     profile->natTraversalServerHostname().c_str(), 
                     profile->natTraversalServerPort(), 
                     profile->stunUsername().c_str(), 
                     profile->stunPassword().c_str(),
                     profile->forceCOMedia(),
                     mFlowContext);

         // New Remote Participant - create media Interface connection
         mRtpSocket = new FlowManagerSipXSocket(mMediaStream->getRtpFlow(), mSipXConversationManager.mSipXTOSValue);
         mRtcpSocket = new FlowManagerSipXSocket(mMediaStream->getRtcpFlow(), mSipXConversationManager.mSipXTOSValue);

         ret = getMediaInterface()->createConnection(mMediaConnectionId, getActiveRemoteParticipantHandle(), mRtpSocket, mRtcpSocket);
#ifdef DISABLE_FLOWMANAGER_IF_NO_NAT_TRAVERSAL
      }
      else
      {
         ret = getMediaInterface()->createConnection(mMediaConnectionId, getActiveRemoteParticipantHandle()
                                                     connectionAddr.c_str(),
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
SipXRemoteParticipantDialogSet::processMediaStreamReadyEvent(std::shared_ptr<MediaStreamReadyEvent::StreamParams> streamParams)
{
   MediaStreamReadyEvent::ReTurnParams* params = dynamic_cast<MediaStreamReadyEvent::ReTurnParams*>(streamParams.get());
   InfoLog( << "processMediaStreamReadyEvent: streamParams: " << *params);
   mRtpTuple = params->getRtpTuple();
   mRtcpTuple = params->getRtcpTuple();   // Check if we had operations pending on the media stream being ready

   RemoteParticipantDialogSet::processMediaStreamReadyEvent(streamParams);
}

void 
SipXRemoteParticipantDialogSet::onMediaStreamReady(const StunTuple& rtpTuple, const StunTuple& rtcpTuple)
{
   // Get event into dum queue, so that callback is on dum thread
   MediaStreamReadyEvent* event = new MediaStreamReadyEvent(*this, std::make_shared<MediaStreamReadyEvent::ReTurnParams>(rtpTuple, rtcpTuple));
   mDum.post(event);
}

void
SipXRemoteParticipantDialogSet::onMediaStreamError(unsigned int errorCode)
{
   // Get event into dum queue, so that callback is on dum thread
   MediaStreamErrorEvent* event = new MediaStreamErrorEvent(*this, errorCode);
   mDum.post(event);
}


std::shared_ptr<SipXMediaInterface>
SipXRemoteParticipantDialogSet::getMediaInterface()
{
   if(!mMediaInterface)
   {
      // Get the media interface from the active participant
      if(getUACOriginalRemoteParticipant())
      {
         mMediaInterface = getUACOriginalRemoteParticipant()->getMediaInterface();
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
SipXRemoteParticipantDialogSet::getConnectionPortOnBridge()
{ 
   if(mConnectionPortOnBridge==-1)
   {
      getLocalRTPPort();  // This call will create a MediaConnection if not already created at this point
   }
   return mConnectionPortOnBridge; 
}

void 
SipXRemoteParticipantDialogSet::freeMediaResources()
{
   if(mMediaConnectionId)
   {
      getMediaInterface()->deleteConnection(mMediaConnectionId);
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
      mSipXConversationManager.getRTPPortManager()->freeRTPPort(mLocalRTPPort);
      mLocalRTPPort = 0;
   }
}

void 
SipXRemoteParticipantDialogSet::setActiveDestination(const char* address, unsigned short rtpPort, unsigned short rtcpPort)
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
SipXRemoteParticipantDialogSet::startDtlsClient(const char* address, unsigned short rtpPort, unsigned short rtcpPort)
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
SipXRemoteParticipantDialogSet::setRemoteSDPFingerprint(const resip::Data& fingerprint)
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
SipXRemoteParticipantDialogSet::createSRTPSession(MediaStream::SrtpCryptoSuite cryptoSuite, const char* remoteKey, unsigned int remoteKeyLen)
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

AppDialog* 
SipXRemoteParticipantDialogSet::createAppDialog(const SipMessage& msg)
{
   if(mFlowContext->getSipCallId().empty())
   {
      mFlowContext->setSipCallId(msg.header(h_CallId).value());
   }

   return RemoteParticipantDialogSet::createAppDialog(msg);
}

void 
SipXRemoteParticipantDialogSet::setActiveRemoteParticipantHandle(ParticipantHandle handle)
{ 
   RemoteParticipantDialogSet::setActiveRemoteParticipantHandle(handle);
   // Maintain MediaInterface mapping if connectionId is already allocated
   if (mMediaConnectionId > 0)
   {
      getMediaInterface()->updateConnectionIdToPartipantHandleMapping(mMediaConnectionId, getActiveRemoteParticipantHandle());
   }
}

bool
SipXRemoteParticipantDialogSet::isAsyncMediaSetup()
{
   return mRtpTuple.getTransportType() == reTurn::StunTuple::None;
}

void
SipXRemoteParticipantDialogSet::fixUpSdp(SdpContents* sdp)
{
   if (sdp)
   {
      sdp->session().media().front().port() = mRtpTuple.getPort();
      sdp->session().connection() = SdpContents::Session::Connection(mRtpTuple.getAddress().is_v4() ? SdpContents::IP4 : SdpContents::IP6, mRtpTuple.getAddress().to_string().c_str());  // c=
   }
}


/* ====================================================================

 Copyright (c) 2021, SIP Spectrum, Inc.
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
