#if !defined(RemoteIMSessionParticipant_hxx)
#define RemoteIMSessionParticipant_hxx

#include <map>

#include "ConversationManager.hxx"
#include "Participant.hxx"
#include "RemoteParticipant.hxx"
#include "RemoteIMSessionParticipantDialogSet.hxx"
#include "Participant.hxx"

#include <resip/dum/AppDialogSet.hxx>
#include <resip/dum/AppDialog.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/DialogSetHandler.hxx>
#include <resip/dum/SubscriptionHandler.hxx>

#include <memory>

namespace resip
{
class DialogUsageManager;
class SipMessage;
}

// Disable warning 4250
// VS2019 give a 4250 warning:  
// RemoteIMSessionParticipant.hxx(70, 1): warning C4250 : 'recon::RemoteIMSessionParticipant' : inherits 'recon::RemoteParticipant::recon::RemoteParticipant::addToConversation' via dominance
#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4250 )
#endif

namespace recon
{
class ConversationManager;

/**
  This class represents an IM Session remote participant.  A remote participant is a 
  participant with a network connection to a remote entity.  This
  implementation is for a SIP / IM Sessions.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class RemoteIMSessionParticipant : public virtual RemoteParticipant, public virtual Participant
{
public:
   RemoteIMSessionParticipant(ParticipantHandle partHandle,   // UAC
                     ConversationManager& conversationManager,
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet);

   RemoteIMSessionParticipant(ConversationManager& conversationManager,   // UAS or forked leg
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet);

   virtual ~RemoteIMSessionParticipant();

   // We want to no-op for local holds, since they don't make sense for message sessions
   virtual void checkHoldCondition() override {}
   virtual void setLocalHold(bool hold) override {}
   virtual void notifyIncomingParticipant(const resip::SipMessage& msg, bool autoAnswer, ConversationProfile& conversationProfile) override;
   virtual void hold() override {}
   virtual void unhold() override {}

   virtual void buildSdpOffer(bool holdSdp, CallbackSdpReady sdpReady, bool preferExistingSdp = false) override;
   virtual void adjustRTPStreams(bool sendingOffer = false) override {} // nothing to do, we don't manage RTP streams

   virtual int getConnectionPortOnBridge() override { return -1; } // doesn't interact with mixing bridge
   virtual bool hasInput() override { return false; }
   virtual bool hasOutput() override { return false; }

   virtual void requestKeyframe() override;

protected:

   virtual RemoteIMSessionParticipantDialogSet& getIMSessionDialogSet() { return dynamic_cast<RemoteIMSessionParticipantDialogSet&>(getDialogSet()); }
   virtual bool mediaStackPortAvailable() override { return true; } // doesn't use media stack, just return availabiltiy as true

private:
   virtual void buildSdpAnswer(const resip::SdpContents& offer, CallbackSdpReady sdpReady) override;
};

}

#endif


/* ====================================================================

 Copyright (c) 2022, SIP Spectrum, Inc.  http://www.sipspectrum.com
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
