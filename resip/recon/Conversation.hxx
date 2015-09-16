#if !defined(Conversation_hxx)
#define Conversation_hxx

#include "ConversationManager.hxx"
#include "ConversationParticipantAssignment.hxx"

namespace recon
{
class Participant;
class LocalParticipant;
class RemoteParticipant;
class RelatedConversationSet;
class BridgeMixer;

/**
  This class is used to manage membership of participants.

  Conversations are used to designate the audio properties
  between a number of managed participants.  By default all
  participants in a Conversation can talk to and hear all 
  other participants.  A modified participants contribution 
  setting to a conversation can change this.

  If a remote participant is a sole member in a conversation, 
  then he/she will be put on hold.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class Conversation 
{
public:  
   Conversation(ConversationHandle handle, 
                ConversationManager& conversationManager,
                RelatedConversationSet* relatedConversationSet=0,  // Pass NULL to create new RelatedConversationSet 
                bool broadcastOnly = false);   
   ~Conversation();

   void addParticipant(Participant* participant, unsigned int inputGain = 100, unsigned int outputGain = 100);
   void removeParticipant(Participant* participant);
   void modifyParticipantContribution(Participant* participant, unsigned int inputGain, unsigned int outputGain);

   unsigned int getNumLocalParticipants() { return mNumLocalParticipants; }
   unsigned int getNumRemoteParticipants() { return mNumRemoteParticipants; }
   bool shouldHold();
   bool broadcastOnly();
   void notifyRemoteParticipantsOfHoldChange();

   void createRelatedConversation(RemoteParticipant* newForkedParticipant, ParticipantHandle origParticipantHandle);
   void join(Conversation* conversation);  // move all non-duplicate participants from this conversation to passed in conversation and destroy this one

   void destroy();

   ConversationHandle getHandle() { return mHandle; }

   void notifyMediaEvent(int mediaConnectionId, MediaEvent::MediaEventType eventType);

   /**
     Notifies a Conversation when an RFC2833 DTMF event is received from a
     particular remote participant.

     @param mediaConnectionId sipX media connectionId for the participant who sent the signal
     @param dtmf Integer representation of the DTMF tone received (from RFC2833 event codes)
     @param duration Duration (in milliseconds) of the DTMF tone received
     @param up Set to true if the DTMF key is up (otherwise down)
   */
   void notifyDtmfEvent(int mediaConnectionId, int dtmf, int duration, bool up);

protected:
   friend class Participant;
   friend class LocalParticipant;
   friend class RemoteParticipant;
   friend class MediaResourceParticipant;
   void registerParticipant(Participant *, unsigned int inputGain=100, unsigned int outputGain=100);
   void unregisterParticipant(Participant *);

   friend class BridgeMixer;
   typedef std::map<ParticipantHandle, ConversationParticipantAssignment> ParticipantMap;
   ParticipantMap& getParticipants() { return mParticipants; }  

   resip::SharedPtr<MediaInterface> getMediaInterface() const { resip_assert(mMediaInterface); return mMediaInterface; }

private: 
   ConversationHandle mHandle;
   ConversationManager& mConversationManager;
   RelatedConversationSet *mRelatedConversationSet;

   ParticipantMap mParticipants;
   Participant* getParticipant(ParticipantHandle partHandle);
   bool mDestroying;
   unsigned int mNumLocalParticipants;
   unsigned int mNumRemoteParticipants;
   unsigned int mNumMediaParticipants;
   bool mBroadcastOnly;

   // sipX Media related members
   BridgeMixer* getBridgeMixer() { return mBridgeMixer; }
   resip::SharedPtr<MediaInterface> mMediaInterface;  
   BridgeMixer* mBridgeMixer;
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
