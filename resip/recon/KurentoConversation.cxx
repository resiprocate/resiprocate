
#include <rutil/Log.hxx>

#include "KurentoConversation.hxx"
#include "Participant.hxx"
#include "LocalParticipant.hxx"
#include "RemoteParticipant.hxx"
#include "MediaResourceParticipant.hxx"
#include "UserAgent.hxx"
#include "RelatedConversationSet.hxx"
#include "ReconSubsystem.hxx"


#include <rutil/Logger.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace recon;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

// The Kurento integration is currently only designed to
// bridge two participants.  Kurento itself can support
// rooms with more than two participants.
// FIXME: adapt to support rooms with more than two participants
#define MAX_PARTICIPANTS 2

KurentoConversation::KurentoConversation(ConversationHandle handle,
                           ConversationManager& conversationManager,
                           KurentoConversationManager& kurentoConversationManager,
                           RelatedConversationSet* relatedConversationSet,
                           ConversationHandle sharedMediaInterfaceConvHandle,
                           ConversationManager::AutoHoldMode autoHoldMode)
: Conversation(handle, conversationManager, relatedConversationSet, sharedMediaInterfaceConvHandle, autoHoldMode, MAX_PARTICIPANTS),
  mKurentoConversationManager(kurentoConversationManager)
{
   if(mKurentoConversationManager.supportsMultipleMediaInterfaces())
   {

      // FIXME Kurento
      if (isSharingMediaInterfaceWithAnotherConversation())
      {
         //KurentoConversation* sharedFlowConversation = dynamic_cast<KurentoConversation*>(mKurentoConversationManager.getConversation(sharedMediaInterfaceConvHandle));
         //mMediaInterface = sharedFlowConversation->getMediaInterface();
      }
      else
      {
         //std::shared_ptr<BridgeMixer> mixer;
         //mKurentoConversationManager.createMediaInterfaceAndMixer(false /* giveFocus?*/,    // Focus will be given when local participant is added
         //                                                  mMediaInterface,
         //                                                  mixer);
         //setBridgeMixer(mixer);
      }
   }
   //InfoLog(<< "mBridgeMixer " << getBridgeMixerShared().use_count() << " " << getBridgeMixer());
}

KurentoConversation::~KurentoConversation()
{
   getBridgeMixerShared().reset();       // Make sure the mixer is destroyed before the media interface
   //mMediaInterface.reset();
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
