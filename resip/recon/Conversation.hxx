#if !defined(Conversation_hxx)
#define Conversation_hxx

#include "ConversationManager.hxx"
#include "ConversationParticipantAssignment.hxx"

namespace reconserver
{
class MyConversationManager;  // FIXME Kurento
}
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
  then he/she will be put on hold for the default AutoHoldMode
  of enabled.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class Conversation 
{
protected:
   Conversation(ConversationHandle handle, 
                ConversationManager& conversationManager,
                RelatedConversationSet* relatedConversationSet,  // Pass NULL to create new RelatedConversationSet 
                ConversationHandle sharedMediaInterfaceConvHandle,
                ConversationManager::AutoHoldMode autoHoldMode,
                unsigned int maxParticipants = 0);
public:
   virtual ~Conversation();

   void addParticipant(Participant* participant, unsigned int inputGain = 100, unsigned int outputGain = 100);
   void removeParticipant(Participant* participant);
   void modifyParticipantContribution(Participant* participant, unsigned int inputGain, unsigned int outputGain);

   unsigned int getNumLocalParticipants() { return mNumLocalParticipants; }
   unsigned int getNumRemoteParticipants() { return mNumRemoteParticipants; }
   unsigned int getNumRemoteIMParticipants() { return mNumRemoteIMParticipants; }
   unsigned int getNumMediaParticipants() { return mNumMediaParticipants; }
   bool shouldHold();
   bool broadcastOnly();
   void notifyRemoteParticipantsOfHoldChange();
   void relayInstantMessageToRemoteParticipants(ParticipantHandle sourceParticipant, const resip::Data& senderDisplayName, const resip::SipMessage& msg);

   void createRelatedConversation(RemoteParticipant* newForkedParticipant, ParticipantHandle origParticipantHandle);
   void join(Conversation* conversation);  // move all non-duplicate participants from this conversation to passed in conversation and destroy this one

   void destroy();

   ConversationHandle getHandle() { return mHandle; }
   void setMaxParticipants(unsigned int maxParticipants) { mMaxParticipants = maxParticipants; };
   unsigned int getMaxParticipants() const { return mMaxParticipants; };

protected:
   friend class Participant;
   friend class SipXParticipant;
   friend class LocalParticipant;
   friend class SipXLocalParticipant;
   friend class RemoteParticipant;
   friend class SipXRemoteParticipant;
   friend class KurentoRemoteParticipant;
   friend class MediaResourceParticipant;
   friend class SipXMediaResourceParticipant;
   void registerParticipant(Participant *, unsigned int inputGain=100, unsigned int outputGain=100);
   void unregisterParticipant(Participant *);

   friend class BridgeMixer;
   friend class SipXBridgeMixer;
   friend class KurentoBridgeMixer;
   typedef std::map<ParticipantHandle, ConversationParticipantAssignment> ParticipantMap;
   ParticipantMap& getParticipants() { return mParticipants; }  

   friend class AddParticipantCmd;
   friend class JoinConversationCmd;
   friend class MoveParticipantCmd;

   // sipX Media related members
   // Note: these are only set here if sipXConversationMediaInterfaceMode is used
   friend class ConversationManager;
   friend class SipXConversationManager;
   friend class KurentoConversationManager;
   BridgeMixer* getBridgeMixer() noexcept { return mBridgeMixer.get(); }
   std::shared_ptr<BridgeMixer> getBridgeMixerShared() { return mBridgeMixer; }
   virtual void setBridgeMixer(std::shared_ptr<BridgeMixer> mixer) { mBridgeMixer = mixer; }
   virtual bool isSharingMediaInterfaceWithAnotherConversation() { return mSharingMediaInterfaceWithAnotherConversation; }


private: 
   ConversationHandle mHandle;
   ConversationManager& mConversationManager;
   RelatedConversationSet *mRelatedConversationSet;

   ParticipantMap mParticipants;
   friend class reconserver::MyConversationManager; // FIXME Kurento
   Participant* getParticipant(ParticipantHandle partHandle);
   bool mDestroying;
   unsigned int mNumLocalParticipants;
   unsigned int mNumRemoteParticipants;
   unsigned int mNumRemoteIMParticipants;
   unsigned int mNumMediaParticipants;
   ConversationManager::AutoHoldMode mAutoHoldMode;
   unsigned int mMaxParticipants;

   // sipX Media related members
   // Note: these are only set here if sipXConversationMediaInterfaceMode is used
   std::shared_ptr<BridgeMixer> mBridgeMixer;
   bool mSharingMediaInterfaceWithAnotherConversation;
};

}

#endif


/* ====================================================================

 Copyright (c) 2021, SIP Spectrum, Inc.
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
