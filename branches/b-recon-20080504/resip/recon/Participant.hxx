#if !defined(Participant_hxx)
#define Participant_hxx

#include "ConversationManager.hxx"
#include <map>

namespace useragent
{
class ConversationManager;

/**
  This is the base class for a RemoteParticipant, LocalParticipant and a
  MediaResourceParticipant.  It implements the common functionality of all
  participants, such as managing which Conversations that are participant
  belongs to.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class Participant
{
   public:  

      typedef std::map<ConversationManager::ConversationHandle,Conversation*> ConversationMap;

      Participant(ConversationManager::ParticipantHandle partHandle,
                  ConversationManager& conversationManager);  

      Participant(ConversationManager& conversationManager);

      virtual ~Participant();

      virtual ConversationManager::ParticipantHandle getParticipantHandle() { return mHandle; }
      virtual void addToConversation(Conversation *conversation, unsigned int inputGain = 100, unsigned int outputGain = 100);
      virtual void removeFromConversation(Conversation *conversation);
      virtual void copyConversationsToParticipant(Participant* destParticipant);
      virtual unsigned int getNumConversations() { return (unsigned int)mConversations.size(); }
      const ConversationMap& getConversations() { return mConversations; }

      virtual void setHandle(ConversationManager::ParticipantHandle partHandle);
      virtual void replaceWithParticipant(Participant* replacingParticipant);
      virtual int getConnectionPortOnBridge() = 0;

      virtual void destroyParticipant() = 0;

   protected:       
      ConversationManager::ParticipantHandle mHandle;
      ConversationManager &mConversationManager;
      ConversationMap mConversations;
};

}

#endif


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
