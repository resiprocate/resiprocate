#if !defined(SipXRemoteParticipant_hxx)
#define SipXRemoteParticipant_hxx

#include <map>

#include "ConversationManager.hxx"
#include "RemoteParticipant.hxx"
#include "SipXRemoteParticipantDialogSet.hxx"

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

namespace recon
{
class ConversationManager;

/**
  This class represent a remote participant.  A remote participant is a 
  participant with a network connection to a remote entity.  This
  implementation is for a SIP / RTP connections.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class SipXRemoteParticipant : public RemoteParticipant
{
public:
   SipXRemoteParticipant(ParticipantHandle partHandle,   // UAC
                     ConversationManager& conversationManager, 
                     resip::DialogUsageManager& dum,
                     SipXRemoteParticipantDialogSet& remoteParticipantDialogSet);

   SipXRemoteParticipant(ConversationManager& conversationManager,            // UAS or forked leg
                     resip::DialogUsageManager& dum,
                     SipXRemoteParticipantDialogSet& remoteParticipantDialogSet);

   virtual ~SipXRemoteParticipant();

   virtual unsigned int getLocalRTPPort();
   void buildSdpOffer(bool holdSdp, resip::SdpContents& offer);

   virtual int getConnectionPortOnBridge();
   virtual int getMediaConnectionId();

   virtual void adjustRTPStreams(bool sendingOffer=false);

private:       
   SipXRemoteParticipantDialogSet& mSipXDialogSet() { return dynamic_cast<SipXRemoteParticipantDialogSet&>(mDialogSet); };

   bool answerMediaLine(resip::SdpContents::Session::Medium& mediaSessionCaps, const sdpcontainer::SdpMediaLine& sdpMediaLine, resip::SdpContents& answer, bool potential);
   bool buildSdpAnswer(const resip::SdpContents& offer, resip::SdpContents& answer);
   bool formMidDialogSdpOfferOrAnswer(const resip::SdpContents& localSdp, const resip::SdpContents& remoteSdp, resip::SdpContents& newSdp, bool offer);
   void setLocalSdp(const resip::SdpContents& sdp);
   void setRemoteSdp(const resip::SdpContents& sdp, bool answer=false);
   void setRemoteSdp(const resip::SdpContents& sdp, sdpcontainer::Sdp* remoteSdp);

   sdpcontainer::Sdp* mLocalSdpSipX;
   sdpcontainer::Sdp* mRemoteSdpSipX;
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
