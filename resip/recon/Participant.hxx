#if !defined(Participant_hxx)
#define Participant_hxx

#include "ConversationManager.hxx"
#include <map>

namespace recon
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

      typedef std::map<ConversationHandle,Conversation*> ConversationMap;

      Participant(ParticipantHandle partHandle,
                  ConversationManager& conversationManager);  

      Participant(ConversationManager& conversationManager);

      virtual ~Participant();

      virtual ParticipantHandle getParticipantHandle() { return mHandle; }
      virtual void addToConversation(Conversation *conversation, unsigned int inputGain = 100, unsigned int outputGain = 100);
      virtual void removeFromConversation(Conversation *conversation);
      virtual void copyConversationsToParticipant(Participant* destParticipant);
      virtual unsigned int getNumConversations() { return (unsigned int)mConversations.size(); }
      const ConversationMap& getConversations() { return mConversations; }

      virtual void setHandle(ParticipantHandle partHandle);
      virtual void replaceWithParticipant(Participant* replacingParticipant);

      virtual int getConnectionPortOnBridge() = 0;
      virtual resip::SharedPtr<MediaInterface> getMediaInterface();
      virtual void applyBridgeMixWeights();
      virtual void applyBridgeMixWeights(Conversation* removedConversation);  

      virtual void destroyParticipant() = 0;

   protected:       
      ParticipantHandle mHandle;
      ConversationManager &mConversationManager;
      ConversationMap mConversations;
};

}

#endif


/* ====================================================================

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
