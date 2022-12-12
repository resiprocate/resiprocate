#include "KurentoMediaStackAdapter.hxx"
#include "ReconSubsystem.hxx"
#include "KurentoParticipant.hxx"
#include "KurentoConversation.hxx"
#include "UserAgent.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

KurentoParticipant::KurentoParticipant(ParticipantHandle partHandle,
                         ConversationManager::ParticipantType participantType,
                         ConversationManager& conversationManager,
                         KurentoMediaStackAdapter& kurentoMediaStackAdapter)
: Participant(partHandle, participantType, conversationManager),
  mKurentoMediaStackAdapter(kurentoMediaStackAdapter)
{
}

KurentoParticipant::KurentoParticipant(ConversationManager::ParticipantType participantType,
                         ConversationManager& conversationManager,
                         KurentoMediaStackAdapter& kurentoMediaStackAdapter)
: Participant(participantType, conversationManager),
  mKurentoMediaStackAdapter(kurentoMediaStackAdapter)
{
}

KurentoParticipant::~KurentoParticipant()
{
}

/* FIXME Kurento std::shared_ptr<KurentoMediaInterface>
KurentoParticipant::getMediaInterface()
{
   switch(mKurentoMediaStackAdapter.getMediaInterfaceMode())
   {
   case KurentoMediaStackAdapter::sipXGlobalMediaInterfaceMode:
      resip_assert(mKurentoMediaStackAdapter.getMediaInterface() != 0);
      return mKurentoMediaStackAdapter.getMediaInterface();
   case KurentoMediaStackAdapter::sipXConversationMediaInterfaceMode:
   {
      // Note:  For this mode, the recon code ensures that all conversations a participant 
      //        is added to will share the same media interface, so using the first 
      //        conversation is sufficient.
      KurentoConversation *firstConversation = dynamic_cast<KurentoConversation*>(mConversations.begin()->second);
      resip_assert(firstConversation->getMediaInterface() != 0);
      return firstConversation->getMediaInterface();
   }
   default:
      resip_assert(false);
      return nullptr;
   }
}*/

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
