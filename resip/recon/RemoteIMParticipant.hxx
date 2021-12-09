#if !defined(RemoteIMParticipant_hxx)
#define RemoteIMParticipant_hxx

#include <map>

#include "ConversationManager.hxx"
#include "IMParticipantBase.hxx"
#include "Participant.hxx"

#include <resip/dum/AppDialogSet.hxx>

#include <memory>

namespace resip
{
class DialogUsageManager;
class SipMessage;
class Contents;
}

namespace recon
{

/**
  This class represent an Instant Message remote participant.  A remote participant is a 
  participant with a network connection to a remote entity.  This
  implementation is for a SIP / SIMPLE mappings.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class RemoteIMParticipant : public IMParticipantBase, public Participant, public resip::AppDialogSet
{
public:
   RemoteIMParticipant(ParticipantHandle partHandle, ConversationManager& conversationManager);  // For incoming
   RemoteIMParticipant(ParticipantHandle partHandle, ConversationManager& conversationManager,   // For createRemoteIMParticipant
      const resip::NameAddr& destination, std::shared_ptr<ConversationProfile> conversationProfile = nullptr);

   virtual ~RemoteIMParticipant();


   virtual int getConnectionPortOnBridge() override { return -1; }  // Is not involved in audio mixing bridge
   virtual bool hasInput() override { return false; }   // Is not involved in audio mixing bridge
   virtual bool hasOutput() override { return false; }  // Is not involved in audio mixing bridge
   virtual void applyBridgeMixWeights() override {}     // Is not involved in audio mixing bridge
   virtual void applyBridgeMixWeights(Conversation* removedConversation) override {} // Is not involved in audio mixing bridge

   virtual void destroyParticipant() override;
   virtual void sendInstantMessage(std::unique_ptr<resip::Contents> contents) override;
   virtual void accept();
   virtual void reject(unsigned int rejectCode);

   virtual bool doesMessageMatch(resip::SipMessage message);


protected:
   friend class ConversationManager;

   // ClientPagerMessageHandler ///////////////////////////////////////////////////
   virtual void onSuccess(resip::ClientPagerMessageHandle, const resip::SipMessage& status);
   virtual void onFailure(resip::ClientPagerMessageHandle, const resip::SipMessage& status, std::unique_ptr<resip::Contents> contents);

   // ServerPagerMessageHandler ///////////////////////////////////////////////////
   virtual void onMessageArrived(resip::ServerPagerMessageHandle, const resip::SipMessage& message);

private:
   void relayInstantMessageToConversations(const resip::SipMessage& msg);

   resip::DialogUsageManager &mDum;
   resip::ServerPagerMessageHandle mServerPagerMessageHandle;
   resip::ClientPagerMessageHandle mClientPagerMessageHandle;
   resip::NameAddr mRemoteUri;
   resip::Data mRemoteAorNoPort;  // stored around for faster matching
   resip::Data mLocalAorNoPort;   // stored around for faster matching
   resip::Data mRemoteDisplayName;
   resip::SipMessage mInitialIncomingMessage;
   std::shared_ptr<ConversationProfile> mConversationProfile;
   int mNumOutstandingSends;
   bool mDelayedDestroyPending;
};

}

#endif


/* ====================================================================

 Copyright (c) 2021, SIP Spectrum, Inc http://www.sipspectrum.com
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
