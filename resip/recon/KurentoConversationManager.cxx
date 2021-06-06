
#if (_MSC_VER >= 1600)
#include <stdint.h>       // Use Visual Studio's stdint.h
#define _MSC_STDINT_H_    // This define will ensure that stdint.h in sipXport tree is not used
#endif

// Kurento includes
#include <kurento-client/KurentoClient.h>

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
#include "KurentoConversationManager.hxx"
#include "ConversationManagerCmds.hxx"
#include "KurentoConversation.hxx"
#include "Participant.hxx"
#include "KurentoBridgeMixer.hxx"
#include "DtmfEvent.hxx"
#include "KurentoRemoteParticipant.hxx"
#include "Conversation.hxx"
#include <rutil/WinLeakCheck.hxx>

#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4355 )
#endif

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

KurentoConversationManager::KurentoConversationManager(const resip::Data& kurentoUri)
: ConversationManager(),
  mKurentoUri(kurentoUri),
  mKurentoTOSValue(0)
{
   init();
}

KurentoConversationManager::KurentoConversationManager(const resip::Data& kurentoUri, int defaultSampleRate, int maxSampleRate)
: ConversationManager(),
  mKurentoUri(kurentoUri),
  mKurentoTOSValue(0)
{
   init(defaultSampleRate, maxSampleRate);
}

void
KurentoConversationManager::init(int defaultSampleRate, int maxSampleRate)
{

   // Connect to the Kurento server

   Uri uri(mKurentoUri);
   std::string kHost = std::string(uri.host().c_str());
   std::string kPort = std::to_string(uri.port());

   mKurentoClient.getMConnectionHandler()->createConnection(kHost, kPort);

}

KurentoConversationManager::~KurentoConversationManager()
{
   getBridgeMixer().reset();   // Make sure the mixer is destroyed before the media interface
}

void
KurentoConversationManager::setUserAgent(UserAgent* userAgent)
{
   ConversationManager::setUserAgent(userAgent);

   // FIXME for Kurento
//   if (mMediaInterface)
//   {
//      // Enable/Disable DTMF digit logging according to UserAgentMasterProfile setting
//      mMediaInterface->allowLoggingDTMFDigits(userAgent->getUserAgentMasterProfile()->dtmfDigitLoggingEnabled());
//   }

}

ConversationHandle
KurentoConversationManager::createSharedMediaInterfaceConversation(ConversationHandle sharedMediaInterfaceConvHandle, AutoHoldMode autoHoldMode)
{
   if (isShuttingDown()) return 0;  // Don't allow new things to be created when we are shutting down

   ConversationHandle convHandle = getNewConversationHandle();

   CreateConversationCmd* cmd = new CreateConversationCmd(this, convHandle, autoHoldMode, sharedMediaInterfaceConvHandle);
   post(cmd);
   return convHandle;
}

void 
KurentoConversationManager::outputBridgeMatrix(ConversationHandle convHandle)
{
   OutputBridgeMixWeightsCmd* cmd = new OutputBridgeMixWeightsCmd(this, convHandle);
   post(cmd);
}

void
KurentoConversationManager::outputBridgeMatrixImpl(ConversationHandle convHandle)
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
KurentoConversationManager::buildSdpOffer(ConversationProfile* profile, SdpContents& offer)
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

//void
//KurentoConversationManager::buildSessionCapabilities(const resip::Data& ipaddress, unsigned int numCodecIds,
//                                              unsigned int codecIds[], resip::SdpContents& sessionCaps)
//{
   // FIXME Kurento:
   // - check who calls this (called from testUA, reConServer, ...
   // - adapt for Kurento
//}

bool
KurentoConversationManager::supportsMultipleMediaInterfaces()
{
   return true;
}

bool
KurentoConversationManager::canConversationsShareParticipants(Conversation* conversation1, Conversation* conversation2)
{
   return false;
}

Conversation *
KurentoConversationManager::createConversationInstance(ConversationHandle handle,
      RelatedConversationSet* relatedConversationSet,  // Pass NULL to create new RelatedConversationSet
      ConversationHandle sharedMediaInterfaceConvHandle,
      ConversationManager::AutoHoldMode autoHoldMode)
{
   return new KurentoConversation(handle, *this, relatedConversationSet, sharedMediaInterfaceConvHandle, autoHoldMode);
}

LocalParticipant *
KurentoConversationManager::createLocalParticipantInstance(ParticipantHandle partHandle)
{
   return 0;
}

MediaResourceParticipant *
KurentoConversationManager::createMediaResourceParticipantInstance(ParticipantHandle partHandle, resip::Uri mediaUrl)
{
   return 0; // FIXME Kurento - implement MediaResourceParticipant in Kurento
}

RemoteParticipant *
KurentoConversationManager::createRemoteParticipantInstance(DialogUsageManager& dum, RemoteParticipantDialogSet& rpds)
{
   return new KurentoRemoteParticipant(*this, dum, rpds);
}

RemoteParticipant *
KurentoConversationManager::createRemoteParticipantInstance(ParticipantHandle partHandle, DialogUsageManager& dum, RemoteParticipantDialogSet& rpds)
{
   return new KurentoRemoteParticipant(partHandle, *this, dum, rpds);
}

RemoteParticipantDialogSet *
KurentoConversationManager::createRemoteParticipantDialogSetInstance(
      ConversationManager::ParticipantForkSelectMode forkSelectMode,
      std::shared_ptr<ConversationProfile> conversationProfile)
{
   return new KurentoRemoteParticipantDialogSet(*this, forkSelectMode, conversationProfile);
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
