
#include <rutil/Log.hxx>

#include "LibWebRTCConversation.hxx"
#include "Participant.hxx"
#include "LocalParticipant.hxx"
#include "RemoteParticipant.hxx"
#include "MediaResourceParticipant.hxx"
#include "UserAgent.hxx"
#include "RelatedConversationSet.hxx"
#include "ReconSubsystem.hxx"
#include "LibWebRTCRemoteParticipant.hxx"


#include <rutil/Logger.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace recon;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

// The LibWebRTC integration is currently only designed to
// bridge two participants.  LibWebRTC itself can support
// rooms with more than two participants.
// FIXME: adapt to support rooms with more than two participants
#define MAX_PARTICIPANTS 2

LibWebRTCConversation::LibWebRTCConversation(ConversationHandle handle,
                           ConversationManager& conversationManager,
                           LibWebRTCMediaStackAdapter& libwebrtcMediaStackAdapter,
                           RelatedConversationSet* relatedConversationSet,
                           ConversationHandle sharedMediaInterfaceConvHandle,
                           ConversationManager::AutoHoldMode autoHoldMode)
: Conversation(handle, conversationManager, relatedConversationSet, sharedMediaInterfaceConvHandle, autoHoldMode, MAX_PARTICIPANTS),
  mLibWebRTCMediaStackAdapter(libwebrtcMediaStackAdapter),
  mLibWebRTCReInviteOnParticipantsPresent(true)
{
   if(mLibWebRTCMediaStackAdapter.supportsMultipleMediaInterfaces())
   {

      // FIXME LibWebRTC
      if (isSharingMediaInterfaceWithAnotherConversation())
      {
         //LibWebRTCConversation* sharedFlowConversation = dynamic_cast<LibWebRTCConversation*>(mLibWebRTCMediaStackAdapter.getConversation(sharedMediaInterfaceConvHandle));
         //mMediaInterface = sharedFlowConversation->getMediaInterface();
      }
      else
      {
         //std::shared_ptr<BridgeMixer> mixer;
         //mLibWebRTCMediaStackAdapter.createMediaInterfaceAndMixer(false /* giveFocus?*/,    // Focus will be given when local participant is added
         //                                                  mMediaInterface,
         //                                                  mixer);
         //setBridgeMixer(mixer);
      }
   }
   //InfoLog(<< "mBridgeMixer " << getBridgeMixerShared().use_count() << " " << getBridgeMixer());

   std::shared_ptr<ConfigParse> cfg = getConfig();
   if(cfg)
   {
      mLibWebRTCReInviteOnParticipantsPresent = cfg->getConfigBool("LibWebRTCReInviteOnParticipantsPresent",
         mLibWebRTCReInviteOnParticipantsPresent);
   }
}

LibWebRTCConversation::~LibWebRTCConversation()
{
   getBridgeMixerShared().reset();       // Make sure the mixer is destroyed before the media interface
   //mMediaInterface.reset();
}

void
LibWebRTCConversation::onParticipantAdded(Participant* participant)
{
}

void
LibWebRTCConversation::confirmParticipant(Participant* participant)
{
   DebugLog(<<"confirmParticipant handle: " << getHandle()
            << " numRemote: " << getNumRemoteParticipants()
            << " participant: " << participant->getParticipantHandle());
   LibWebRTCRemoteParticipant *_p = dynamic_cast<LibWebRTCRemoteParticipant*>(participant);
   std::shared_ptr<libwebrtc::BaseRtpEndpoint> answeredEndpoint = _p->getEndpoint();
   if(getNumRemoteParticipants() < 2)
   {
      DebugLog(<<"we are first in the conversation");
      //_p->waitingMode(); // FIXME - loopback is setup in buildSdpOffer/buildSdpAnswer logic
      return;
   }
   if(getNumRemoteParticipants() > 2)
   {
      WarningLog(<<"participants already here, can't join, numRemoteParticipants = " << getNumRemoteParticipants());
      return;
   }
   DebugLog(<<"joining a Conversation with an existing Participant");

   if(!answeredEndpoint)
   {
      ErrLog(<<"our endpoint is not initialized"); // FIXME
      return;
   }
   if(!_p->getWaitingModeElement()->valid())
   {
      DebugLog(<<"the local endpoint is not yet in a valid state");
      return;
   }
   DebugLog(<<"attempting to join two RemoteParticipants");
   _p->getWaitingModeElement()->disconnect([this, _p, answeredEndpoint]{
      answeredEndpoint->disconnect([this, _p, answeredEndpoint]{
         // Find the other Participant / endpoint

         Conversation::ParticipantMap& m = getParticipants();
         LibWebRTCRemoteParticipant* krp = 0; // FIXME - better to use shared_ptr
         Conversation::ParticipantMap::iterator _it = m.begin();
         for(;_it != m.end() && krp == 0; _it++)
         {
            krp = dynamic_cast<LibWebRTCRemoteParticipant*>(_it->second.getParticipant());
            if(krp == _p)
            {
               krp = 0;
            }
         }
         resip_assert(krp);
         std::shared_ptr<libwebrtc::BaseRtpEndpoint> otherEndpoint = krp->getEndpoint();

         krp->getWaitingModeElement()->disconnect([this, _p, answeredEndpoint, otherEndpoint, krp]{
            otherEndpoint->disconnect([this, _p, answeredEndpoint, otherEndpoint, krp]{
               otherEndpoint->connect([this, _p, answeredEndpoint, otherEndpoint, krp]{
                  answeredEndpoint->connect([this, _p, answeredEndpoint, otherEndpoint, krp]{
                     InfoLog(<<"peers connected");
                     if(mLibWebRTCReInviteOnParticipantsPresent)
                     {
                        krp->reInvite();
                     }

                     ParticipantHandle partHandle1 = _p->getParticipantHandle();
                     ParticipantHandle partHandle2 = krp->getParticipantHandle();
                     for(int i = 1000; i <= 5000; i+=1000)
                     {
                        std::chrono::milliseconds _i = std::chrono::milliseconds(i);
                        std::chrono::milliseconds __i = std::chrono::milliseconds(i + 500);
                        getConversationManager().requestKeyframe(partHandle1, _i);
                        getConversationManager().requestKeyframe(partHandle2, _i);
                        getConversationManager().requestKeyframeFromPeer(partHandle1, __i);
                        getConversationManager().requestKeyframeFromPeer(partHandle2, __i);
                     }

                  }, *otherEndpoint);
               }, *answeredEndpoint);
            }, *krp->getWaitingModeElement());
         }, *otherEndpoint); // otherEndpoint->disconnect()
      }, *_p->getWaitingModeElement());
   }, *answeredEndpoint);  // answeredEndpoint->disconnect()
}

void
LibWebRTCConversation::onParticipantRemoved(Participant* participant)
{
   DebugLog(<<"onParticipantRemoved, checking for remaining participants");
   LibWebRTCRemoteParticipant *_p = dynamic_cast<LibWebRTCRemoteParticipant*>(participant);
   std::shared_ptr<libwebrtc::BaseRtpEndpoint> myEndpoint = _p->getEndpoint();
   Conversation::ParticipantMap& m = getParticipants();
   LibWebRTCRemoteParticipant* krp = 0; // FIXME - better to use shared_ptr
   Conversation::ParticipantMap::iterator _it = m.begin();
   for(;_it != m.end() && krp == 0; _it++)
   {
      krp = dynamic_cast<LibWebRTCRemoteParticipant*>(_it->second.getParticipant());
      if(krp == _p)
      {
         krp = 0;
      }
   }
   if(krp)
   {
      DebugLog(<<"remaining participant found");
      std::shared_ptr<libwebrtc::BaseRtpEndpoint> otherEndpoint = krp->getEndpoint();
      otherEndpoint->disconnect([this, krp, myEndpoint, otherEndpoint](){
         myEndpoint->disconnect([this, krp](){
            krp->waitingMode();
         }, *otherEndpoint);
      }, *myEndpoint);
   }

   return;
}

/* ====================================================================

 Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2022, Daniel Pocock https://danielpocock.com
 Copyright (c) 2021, SIP Spectrum, Inc. www.sipspectrum.com
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
