#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// sipX includes
#if (_MSC_VER >= 1600)
#include <stdint.h>       // Use Visual Studio's stdint.h
#define _MSC_STDINT_H_    // This define will ensure that stdint.h in sipXport tree is not used
#endif

#include "ConversationManager.hxx"
#include "ReconSubsystem.hxx"
#include "KurentoRemoteParticipantDialogSet.hxx"
#include "KurentoRemoteParticipant.hxx"
#include "Conversation.hxx"
#include "UserAgent.hxx"
#include "MediaStreamEvent.hxx"

// Flowmanager Includes
#include "reflow/FlowManager.hxx"
#include "reflow/Flow.hxx"
#include "reflow/MediaStream.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/Random.hxx>
#include <rutil/DnsUtil.hxx>
#include <resip/stack/SipFrag.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/ServerInviteSession.hxx>

#include <rutil/WinLeakCheck.hxx>

#include <utility>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

KurentoRemoteParticipantDialogSet::KurentoRemoteParticipantDialogSet(KurentoConversationManager& kurentoConversationManager,
                                                       ConversationManager::ParticipantForkSelectMode forkSelectMode,
                                                       std::shared_ptr<ConversationProfile> conversationProfile) :
   RemoteParticipantDialogSet(kurentoConversationManager, forkSelectMode, conversationProfile),
   mKurentoConversationManager(kurentoConversationManager),
   mLocalRTPPort(0),
   mAllocateLocalRTPPortFailed(false),
   mPeerExpectsSAVPF(false),
   //mNatTraversalMode(flowmanager::MediaStream::NoNatTraversal),
   mMediaConnectionId(0),
   mConnectionPortOnBridge(-1)
{

   InfoLog(<< "KurentoRemoteParticipantDialogSet created.");
}

KurentoRemoteParticipantDialogSet::~KurentoRemoteParticipantDialogSet()
{
   freeMediaResources();
   InfoLog(<< "KurentoRemoteParticipantDialogSet destroyed.  mActiveRemoteParticipantHandle=" << getActiveRemoteParticipantHandle());
}

void 
KurentoRemoteParticipantDialogSet::processMediaStreamReadyEvent(std::shared_ptr<MediaStreamReadyEvent::StreamParams> streamParams)
{
   MediaStreamReadyEvent::ReTurnParams* params = dynamic_cast<MediaStreamReadyEvent::ReTurnParams*>(streamParams.get());
   InfoLog( << "processMediaStreamReadyEvent: streamParams: " << *params);
   mRtpTuple = params->getRtpTuple();
   mRtcpTuple = params->getRtcpTuple();   // Check if we had operations pending on the media stream being ready

   RemoteParticipantDialogSet::processMediaStreamReadyEvent(streamParams);
}

void 
KurentoRemoteParticipantDialogSet::onMediaStreamReady(const StunTuple& rtpTuple, const StunTuple& rtcpTuple)
{
   // Get event into dum queue, so that callback is on dum thread
   MediaStreamReadyEvent* event = new MediaStreamReadyEvent(*this, std::make_shared<MediaStreamReadyEvent::ReTurnParams>(rtpTuple, rtcpTuple));
   mDum.post(event);
}

void
KurentoRemoteParticipantDialogSet::onMediaStreamError(unsigned int errorCode)
{
   // Get event into dum queue, so that callback is on dum thread
   MediaStreamErrorEvent* event = new MediaStreamErrorEvent(*this, errorCode);
   mDum.post(event);
}


/* FIXME Kurento std::shared_ptr<SipXMediaInterface>
KurentoRemoteParticipantDialogSet::getMediaInterface()
{
   if(!mMediaInterface)
   {
      // Get the media interface from the active participant
      if(getUACOriginalRemoteParticipant())
      {
         mMediaInterface = dynamic_cast<SipXParticipant*>(getUACOriginalRemoteParticipant())->getMediaInterface();
      }
      else if(mDialogs.size() > 0)
      {
         // All participants in the set will have the same media interface - query from first
         resip_assert(mDialogs.begin()->second);
         mMediaInterface = dynamic_cast<SipXParticipant*>(mDialogs.begin()->second)->getMediaInterface();
      }
   }
   resip_assert(mMediaInterface);
   return mMediaInterface;
} */

int 
KurentoRemoteParticipantDialogSet::getConnectionPortOnBridge()
{ 
   if(mConnectionPortOnBridge==-1)
   {
      // FIXME Kurento getLocalRTPPort();  // This call will create a MediaConnection if not already created at this point
   }
   return mConnectionPortOnBridge; 
}

void 
KurentoRemoteParticipantDialogSet::freeMediaResources()
{
   // FIXME Kurento
/*   if(mMediaConnectionId)
   {
      getMediaInterface()->deleteConnection(mMediaConnectionId);
      mMediaConnectionId = 0;
   }*/


}

void 
KurentoRemoteParticipantDialogSet::setActiveDestination(const char* address, unsigned short rtpPort, unsigned short rtcpPort)
{
// FIXME Kurento
}

void 
KurentoRemoteParticipantDialogSet::startDtlsClient(const char* address, unsigned short rtpPort, unsigned short rtcpPort)
{
// FIXME Kurento
}

void 
KurentoRemoteParticipantDialogSet::setRemoteSDPFingerprint(const resip::Data& fingerprint)
{
// FIXME Kurento
}

bool 
KurentoRemoteParticipantDialogSet::createSRTPSession(flowmanager::MediaStream::SrtpCryptoSuite cryptoSuite, const char* remoteKey, unsigned int remoteKeyLen)
{
   // FIXME Kurento
   WarningLog(<<"createSRTPSession: FIXME");
   return false;
}

AppDialog* 
KurentoRemoteParticipantDialogSet::createAppDialog(const SipMessage& msg)
{
   /* FIXME Kurento (HOMER) if(mFlowContext->getSipCallId().empty())
   {
      mFlowContext->setSipCallId(msg.header(h_CallId).value());
   } */

   return RemoteParticipantDialogSet::createAppDialog(msg);
}

void 
KurentoRemoteParticipantDialogSet::setActiveRemoteParticipantHandle(ParticipantHandle handle)
{ 
   RemoteParticipantDialogSet::setActiveRemoteParticipantHandle(handle);
   // Maintain MediaInterface mapping if connectionId is already allocated
   if (mMediaConnectionId > 0)
   {
      // FIXME Kurento getMediaInterface()->updateConnectionIdToPartipantHandleMapping(mMediaConnectionId, getActiveRemoteParticipantHandle());
   }
}

bool
KurentoRemoteParticipantDialogSet::isAsyncMediaSetup()
{
   //return mRtpTuple.getTransportType() == reTurn::StunTuple::None;
   return false; // FIXME Kurento
}

void
KurentoRemoteParticipantDialogSet::fixUpSdp(SdpContents* sdp)
{

   if (sdp)
   {
      // FIXME Kurento
      sdp->session().origin().user() = "-";
   //   sdp->session().media().front().port() = mRtpTuple.getPort();
   //   sdp->session().connection() = SdpContents::Session::Connection(mRtpTuple.getAddress().is_v4() ? SdpContents::IP4 : SdpContents::IP6, mRtpTuple.getAddress().to_string().c_str());  // c=
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
