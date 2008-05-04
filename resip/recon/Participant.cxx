#include "ConversationManager.hxx"
#include "UserAgentSubsystem.hxx"
#include "Participant.hxx"
#include "Conversation.hxx"
#include "UserAgent.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>

using namespace useragent;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM UserAgentSubsystem::USERAGENT

Participant::Participant(ConversationManager::ParticipantHandle partHandle,
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
Participant::setHandle(ConversationManager::ParticipantHandle partHandle)
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
   assert(conversation);
   if(mConversations.find(conversation->getHandle()) != mConversations.end()) return;  // already present

   mConversations[conversation->getHandle()] = conversation;
   conversation->registerParticipant(this, inputGain, outputGain);
}

void 
Participant::removeFromConversation(Conversation *conversation)
{
   assert(conversation);
   //InfoLog(<< "Participant handle=" << mHandle << " removed from conversation=" << conversation->getHandle());
   mConversations.erase(conversation->getHandle());  // Note: this must come before next line - since unregisterParticipant may end up destroying conversation
   conversation->unregisterParticipant(this);
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
   mConversations.clear();  // Clear so that we won't remove replaced reference from Conversation 
   mHandle = 0;             // Set to 0 so that we won't remove replaced reference from ConversationManager
   
   mConversationManager.getBridgeMixer().calculateMixWeightsForParticipant(this);  // Ensure we remove ourselves from the bridge port matrix
}


/* ====================================================================

 Original contribution Copyright (C) 2008 Plantronics, Inc.
 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */

