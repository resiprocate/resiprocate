
#if (_MSC_VER >= 1600)
#include <stdint.h>       // Use Visual Studio's stdint.h
#define _MSC_STDINT_H_    // This define will ensure that stdint.h in sipXport tree is not used
#endif

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
#include "KurentoMediaStackAdapter.hxx"
#include "ConversationManagerCmds.hxx"
#include "KurentoConversation.hxx"
#include "Participant.hxx"
#include "KurentoBridgeMixer.hxx"
#include "DtmfEvent.hxx"
#include "KurentoRemoteParticipant.hxx"
#include "Conversation.hxx"
#include <rutil/WinLeakCheck.hxx>

#include "cajun/json/elements.h"

#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4355 )
#endif

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

KurentoMediaStackAdapter::KurentoMediaStackAdapter(ConversationManager& conversationManager, const Data& kurentoUri)
: MediaStackAdapter(conversationManager),
  mKurentoUri(kurentoUri),
  mKurentoManager(5000),  // FIXME - make this value configurable
  mKurentoTOSValue(0) // FIXME - make this value configurable
{
   init();
}

KurentoMediaStackAdapter::KurentoMediaStackAdapter(ConversationManager& conversationManager, const Data& kurentoUri, int defaultSampleRate, int maxSampleRate)
: MediaStackAdapter(conversationManager),
  mKurentoUri(kurentoUri),
  mKurentoManager(5000),  // FIXME - make this value configurable
  mKurentoTOSValue(0) // FIXME - make this value configurable
{
   init(defaultSampleRate, maxSampleRate);
}

void
KurentoMediaStackAdapter::init(int defaultSampleRate, int maxSampleRate)
{
   // Connect to the Kurento server
   DebugLog(<<"trying to connect to Kurento host " << mKurentoUri);
   mKurentoConnection = mKurentoManager.getKurentoConnection(mKurentoUri.c_str()); // FIXME wait for connection
   mPipeline = make_shared<kurento::MediaPipeline>(mKurentoConnection);
   mPipeline->create([this]{
      DebugLog(<<"pipeline created with ID " << mPipeline->getId());
   });  // FIXME - wait for creation
}

KurentoMediaStackAdapter::~KurentoMediaStackAdapter()
{
   getBridgeMixer().reset();   // Make sure the mixer is destroyed before the media interface
}

void
KurentoMediaStackAdapter::conversationManagerReady(ConversationManager* conversationManager)
{
}

void
KurentoMediaStackAdapter::setUserAgent(UserAgent* userAgent)
{
   // FIXME for Kurento
//   if (mMediaInterface)
//   {
//      // Enable/Disable DTMF digit logging according to UserAgentMasterProfile setting
//      mMediaInterface->allowLoggingDTMFDigits(userAgent->getUserAgentMasterProfile()->dtmfDigitLoggingEnabled());
//   }

}

ConversationHandle
KurentoMediaStackAdapter::createSharedMediaInterfaceConversation(ConversationHandle sharedMediaInterfaceConvHandle, ConversationManager::AutoHoldMode autoHoldMode)
{
   if (isShuttingDown()) return 0;  // Don't allow new things to be created when we are shutting down

   ConversationHandle convHandle = getNewConversationHandle();

   CreateConversationCmd* cmd = new CreateConversationCmd(&getConversationManager(), convHandle, autoHoldMode, sharedMediaInterfaceConvHandle);
   post(cmd);
   return convHandle;
}

void
KurentoMediaStackAdapter::outputBridgeMatrixImpl(ConversationHandle convHandle)
{
   // Note: convHandle of 0 only makes sense if sipXGlobalMediaInterfaceMode is enabled
   if (convHandle == 0)
   {
      if (getBridgeMixer() != 0)
      {
         getBridgeMixer()->outputBridgeMixWeights();
      }
      else
      {
         WarningLog(<< "ConversationManager::outputBridgeMatrix request with no conversation handle is not appropriate for current MediaInterfaceMode");
      }
   }
   else
   {
      Conversation* conversation = getConversation(convHandle);
      if (conversation)
      {
         if (conversation->getBridgeMixer() != 0)
         {
            conversation->getBridgeMixer()->outputBridgeMixWeights();
         }
         else
         {
            WarningLog(<< "ConversationManager::outputBridgeMatrix requested conversation wihtout a mixer/media interface, conversationHandle=" << convHandle);
         }
      }
      else
      {
         WarningLog(<< "ConversationManager::outputBridgeMatrix requested for non-existing conversationHandle=" << convHandle);
      }
   }
}

void
KurentoMediaStackAdapter::buildSessionCapabilities(const resip::Data& ipaddress,
                               const std::vector<unsigned int>& codecIds, resip::SdpContents& sessionCaps)
{
   // FIXME Kurento:
   // - check who calls this (called from testUA, reConServer, ...
   // - adapt for Kurento

   // as a hack, we populate it with a static copy of the SDP from Kurento

   // FIXME: ipaddress is assumed to be IP4
   Data caps("v=0\r\n"
             "o=- 3843572881 3843572881 IN IP4 " + ipaddress + "\r\n"
             "s=Kurento Media Server\r\n"
             "c=IN IP4 " + ipaddress + "\r\n"
             "t=0 0\r\n"
             "m=audio 34712 RTP/AVP 0\r\n"
             "a=sendrecv\r\n"
             "a=rtcp:34713\r\n"
             "a=rtpmap:0 PCMU/8000\r\n"
             "a=ssrc:866007149 cname:user2141419334@host-99aef493\r\n"
             "m=video 2962 RTP/AVP 97 126\r\n"
             "a=sendrecv\r\n"
             "a=rtcp:2963\r\n"
             "a=rtpmap:97 H264/90000\r\n"
             "a=rtpmap:126 H264/90000\r\n"
             "a=fmtp:97 packetization-mode=0;profile-level-id=420016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16\r\n"
             "a=fmtp:126 packetization-mode=1;profile-level-id=428016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16\r\n"
             "a=ssrc:1014502683 cname:user2141419334@host-99aef493\r\n");

   HeaderFieldValue hfv(caps.data(), caps.size());
   Mime type("application", "sdp");
   SdpContents sdp(hfv, type);
   sessionCaps = sdp;
}

bool
KurentoMediaStackAdapter::supportsMultipleMediaInterfaces()
{
   return true;
}

bool
KurentoMediaStackAdapter::canConversationsShareParticipants(Conversation* conversation1, Conversation* conversation2)
{
   return false;
}

Conversation *
KurentoMediaStackAdapter::createConversationInstance(ConversationHandle handle,
      RelatedConversationSet* relatedConversationSet,  // Pass NULL to create new RelatedConversationSet
      ConversationHandle sharedMediaInterfaceConvHandle,
      ConversationManager::AutoHoldMode autoHoldMode)
{
   return new KurentoConversation(handle, getConversationManager(), *this, relatedConversationSet, sharedMediaInterfaceConvHandle, autoHoldMode);
}

LocalParticipant *
KurentoMediaStackAdapter::createLocalParticipantInstance(ParticipantHandle partHandle)
{
   return 0;
}

MediaResourceParticipant *
KurentoMediaStackAdapter::createMediaResourceParticipantInstance(ParticipantHandle partHandle, resip::Uri mediaUrl)
{
   return 0; // FIXME Kurento - implement MediaResourceParticipant in Kurento
}

RemoteParticipant *
KurentoMediaStackAdapter::createRemoteParticipantInstance(DialogUsageManager& dum, RemoteParticipantDialogSet& rpds)
{
   KurentoRemoteParticipant *rp = new KurentoRemoteParticipant(getConversationManager(), *this, dum, rpds);
   return rp;
}

RemoteParticipant *
KurentoMediaStackAdapter::createRemoteParticipantInstance(ParticipantHandle partHandle, DialogUsageManager& dum, RemoteParticipantDialogSet& rpds)
{
   KurentoRemoteParticipant *rp = new KurentoRemoteParticipant(partHandle, getConversationManager(), *this, dum, rpds);
   return rp;
}

RemoteParticipantDialogSet *
KurentoMediaStackAdapter::createRemoteParticipantDialogSetInstance(
      ConversationManager::ParticipantForkSelectMode forkSelectMode,
      std::shared_ptr<ConversationProfile> conversationProfile)
{
   return new KurentoRemoteParticipantDialogSet(getConversationManager(), *this, forkSelectMode, conversationProfile);
}

void
KurentoMediaStackAdapter::process()
{
   mKurentoManager.process();
}

void
KurentoMediaStackAdapter::setRTCPEventLoggingHandler(std::shared_ptr<flowmanager::RTCPEventLoggingHandler> h)
{
   // FIXME Kurento
}

void
KurentoMediaStackAdapter::initializeDtlsFactory(const resip::Data& defaultAoR)
{
   // FIXME Kurento
}


/* ====================================================================

 Copyright (c) 2021, SIP Spectrum, Inc. www.sipspectrum.com
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
