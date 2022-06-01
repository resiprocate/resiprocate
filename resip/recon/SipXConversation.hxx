#if !defined(SipXConversation_hxx)
#define SipXConversation_hxx

#include "Conversation.hxx"
#include "ConversationManager.hxx"
#include "SipXConversationManager.hxx"
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
  then he/she will be put on hold for the default AutoHoldMode
  of enabled.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class SipXConversation : public Conversation
{
public:  
   SipXConversation(ConversationHandle handle,
                ConversationManager& conversationManager,
                SipXConversationManager& sipXConversationManager,
                RelatedConversationSet* relatedConversationSet,  // Pass NULL to create new RelatedConversationSet 
                ConversationHandle sharedMediaInterfaceConvHandle,
                ConversationManager::AutoHoldMode autoHoldMode);
   virtual ~SipXConversation();

protected:
   friend class Participant;
   friend class SipXParticipant;
   friend class LocalParticipant;
   friend class SipXLocalParticipant;
   friend class RemoteParticipant;
   friend class SipXRemoteParticipant;
   friend class MediaResourceParticipant;
   friend class SipXMediaResourceParticipant;

   friend class BridgeMixer;
   friend class SipXBridgeMixer;

   friend class AddParticipantCmd;
   friend class JoinConversationCmd;
   friend class MoveParticipantCmd;
   std::shared_ptr<SipXMediaInterface> getMediaInterface() const { resip_assert(mMediaInterface); return mMediaInterface; }

   virtual void onParticipantAdded(Participant* participant);
   virtual void onParticipantRemoved(Participant* participant);

private: 
   SipXConversationManager& mSipXConversationManager;

   // sipX Media related members
   friend class ConversationManager;
   friend class SipXConversationManager;
   // Note: these are only set here if sipXConversationMediaInterfaceMode is used
   std::shared_ptr<SipXMediaInterface> mMediaInterface;
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
