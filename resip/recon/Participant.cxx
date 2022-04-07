#include "ConversationManager.hxx"
#include "ReconSubsystem.hxx"
#include "Participant.hxx"
#include "Conversation.hxx"
#include "UserAgent.hxx"
#include "MediaStackAdapter.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

Participant::Participant(ParticipantHandle partHandle,
                         ConversationManager& conversationManager)
: mHandle(partHandle),
  mConversationManager(conversationManager)
{
   mConversationManager.registerParticipant(this);
   //InfoLog(<< "Participant created, handle=" << mHandle);
}

Participant::Participant(ConversationManager& conversationManager)
: mHandle(0),
  mConversationManager(conversationManager)
{
   setHandle(mConversationManager.getNewParticipantHandle());
   //InfoLog(<< "Participant created, handle=" << mHandle);
}

Participant::~Participant()
{
   // Note:  We cannot call Conversation::unregisterParticipant here, since dynamic_cast in the fn will not work - 
   //        thus unregister must be implemented in sub-classes destructors (see ~LocalParticipant and ~RemoteParticipant)
   //InfoLog(<< "Participant destroyed, handle=" << mHandle);
   if(mHandle != 0) mConversationManager.onParticipantDestroyed(mHandle);
   setHandle(0);        // unregister from Conversation Manager
}

void 
Participant::setHandle(ParticipantHandle partHandle)
{
   if(mHandle == partHandle) return;  // already set

   // unregister old handle if set
   if(mHandle)
   {
      mConversationManager.unregisterParticipant(this);
   }
   mHandle = partHandle;
   if(mHandle)
   {
      mConversationManager.registerParticipant(this);
   }
}

void
Participant::addToConversation(Conversation* conversation, unsigned int inputGain, unsigned int outputGain)
{
   resip_assert(conversation);
   if(mConversations.find(conversation->getHandle()) != mConversations.end()) return;  // already present

   InfoLog(<< "Participant handle=" << mHandle << " added to conversation=" << conversation->getHandle() << ", inputGain=" << inputGain << ", outputGain=" << outputGain);

   mConversations[conversation->getHandle()] = conversation;
   conversation->registerParticipant(this, inputGain, outputGain);
}

void 
Participant::removeFromConversation(Conversation *conversation)
{
   resip_assert(conversation);
   InfoLog(<< "Participant handle=" << mHandle << " removed from conversation=" << conversation->getHandle());
   mConversations.erase(conversation->getHandle());  // Note: this must come before next line - since unregisterParticipant may end up destroying conversation
   conversation->unregisterParticipant(this);
}

void
Participant::unregisterFromAllConversations()
{
   // unregister from Conversations
   ConversationMap::iterator it;
   for (it = mConversations.begin(); it != mConversations.end(); it++)
   {
      it->second->unregisterParticipant(this);
   }
   mConversations.clear();
}

void 
Participant::copyConversationsToParticipant(Participant* destParticipant)
{
   ConversationMap::iterator it;
   for(it = mConversations.begin(); it != mConversations.end(); it++)
   {
      destParticipant->addToConversation(it->second);  // Will over-write our entry in the conversations participant map
   }
}

void
Participant::replaceWithParticipant(Participant* replacingParticipant)
{
   replacingParticipant->setHandle(mHandle);              // Set handle on replacing participant
   copyConversationsToParticipant(replacingParticipant);  // Will over-write our entry in the conversations participant map
   Conversation* firstAssociatedConversation=0;
   if(mConversations.size() > 0)
   {
      // If we are running in sipXConversationMediaInterfaceMode, then the call to applyBridgeMixWeights below must be
      // passed a conversation pointer, since this participant will not be able to find the appropriate 
      // BridgeMixer after it's conversation list is cleared.
      firstAssociatedConversation = mConversations.begin()->second;
   }
   mConversations.clear();  // Clear so that we won't remove replaced reference from Conversation 
   mHandle = 0;             // Set to 0 so that we won't remove replaced reference from ConversationManager
   resip_assert((!mConversationManager.getMediaStackAdapter().supportsMultipleMediaInterfaces()) ||  // We are either running in sipXGlobalMediaInterfaceMode
                firstAssociatedConversation != 0);                            // or we are running in sipXConversationMediaInterfaceMode and must have belonged to a conversation
   applyBridgeMixWeights(firstAssociatedConversation);  // Ensure we remove ourselves from the bridge mix matrix
}

void
Participant::applyBridgeMixWeights()
{
   return; // FIXME Kurento
   // Only need to do work on the bridge mixer if we are using it (ie: portOnBridge != -1)
   if (getConnectionPortOnBridge() != -1)
   {
      BridgeMixer* mixer = 0;
      if (!mConversationManager.getMediaStackAdapter().supportsMultipleMediaInterfaces())
      {
         resip_assert(mConversationManager.getBridgeMixer() != 0);
         mixer = mConversationManager.getBridgeMixer().get();
      }
      else
      {
         // Note:  For this mode, the recon code ensures that all conversations a participant 
         //        is added to will share the same media interface, so using the first 
         //        conversation is sufficient.
         resip_assert(mConversations.begin()->second->getBridgeMixer() != 0);
         mixer = mConversations.begin()->second->getBridgeMixer();
      }
      resip_assert(mixer);
      if (mixer)
      {
         mixer->calculateMixWeightsForParticipant(this);
      }
   }
}

// Special version of this call used only when a participant
// is removed from a conversation.  Required when sipXConversationMediaInterfaceMode
// is used, in order to get a pointer to the bridge mixer
// for a participant (ie. LocalParticipant) that has no currently
// assigned conversations.
void 
Participant::applyBridgeMixWeights(Conversation* removedConversation)
{
   BridgeMixer* mixer=0;
   return; // FIXME Kurento
   if(!mConversationManager.getMediaStackAdapter().supportsMultipleMediaInterfaces())
   {
      resip_assert(mConversationManager.getBridgeMixer() != 0);
      mixer = mConversationManager.getBridgeMixer().get();
   }
   else
   {
      resip_assert(removedConversation->getBridgeMixer() != 0);
      mixer = removedConversation->getBridgeMixer();
   }
   resip_assert(mixer);
   if(mixer)
   {
      mixer->calculateMixWeightsForParticipant(this);
   }
}

/* ====================================================================

 Copyright (c) 2021-2022, SIP Spectrum, Inc. www.sipspectrum.com
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
