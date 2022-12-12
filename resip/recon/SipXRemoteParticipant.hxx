#if !defined(SipXRemoteParticipant_hxx)
#define SipXRemoteParticipant_hxx

#include <map>

#include "ConversationManager.hxx"
#include "Participant.hxx"
#include "RemoteParticipant.hxx"
#include "SipXRemoteParticipant.hxx"
#include "SipXRemoteParticipantDialogSet.hxx"
#include "SipXParticipant.hxx"

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

namespace sdpcontainer
{
class Sdp; 
class SdpMediaLine;
}

// Disable warning 4250
// VS2019 give a 4250 warning:  
// SipXRemoteParticipant.hxx(80,1): warning C4250: 'recon::SipXRemoteParticipant': inherits 'recon::RemoteParticipant::recon::RemoteParticipant::addToConversation' via dominance
#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4250 )
#endif

namespace recon
{
class SipXMediaStackAdapter;

/**
  This class represent a remote participant.  A remote participant is a 
  participant with a network connection to a remote entity.  This
  implementation is for a SIP / RTP connections.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class SipXRemoteParticipant : public virtual RemoteParticipant, public virtual SipXParticipant
{
public:
   SipXRemoteParticipant(ParticipantHandle partHandle,   // UAC
                     ConversationManager& conversationManager,
                     SipXMediaStackAdapter& sipXMediaStackAdapter,
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet);

   SipXRemoteParticipant(ConversationManager& conversationManager,
                     SipXMediaStackAdapter& sipXMediaStackAdapter,            // UAS or forked leg
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet);

   virtual ~SipXRemoteParticipant();

   virtual unsigned int getLocalRTPPort();
   virtual void buildSdpOffer(bool holdSdp, CallbackSdpReady sdpReady, bool preferExistingSdp = false);

   virtual int getConnectionPortOnBridge();
   virtual bool hasInput() { return true; }
   virtual bool hasOutput() { return true; }
   virtual int getMediaConnectionId();

   virtual void adjustRTPStreams(bool sendingOffer=false);

   virtual void requestKeyframe() override;

protected:
   virtual bool mediaStackPortAvailable();

   // Note:  Returns non-null the majority of the time, can return null on object destruction
   virtual SipXRemoteParticipantDialogSet* getSipXDialogSet() { return dynamic_cast<SipXRemoteParticipantDialogSet*>(&getDialogSet()); }

private:
   bool answerMediaLine(resip::SdpContents::Session::Medium& mediaSessionCaps, const sdpcontainer::SdpMediaLine& sdpMediaLine, resip::SdpContents& answer, bool potential);
   virtual void buildSdpAnswer(const resip::SdpContents& offer, CallbackSdpReady sdpReady) override;
};

}

#endif


/* ====================================================================

 Copyright (c) 2021-2022, SIP Spectrum, Inc. www.sipspectrum.com
 Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2021-2022, Daniel Pocock https://danielpocock.com
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
