#if !defined(KurentoRemoteParticipant_hxx)
#define KurentoRemoteParticipant_hxx

#include <map>

#include "ConversationManager.hxx"
#include "Participant.hxx"
#include "RemoteParticipant.hxx"
#include "KurentoRemoteParticipant.hxx"
#include "KurentoRemoteParticipantDialogSet.hxx"
#include "KurentoParticipant.hxx"

#include <resip/dum/AppDialogSet.hxx>
#include <resip/dum/AppDialog.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/DialogSetHandler.hxx>
#include <resip/dum/SubscriptionHandler.hxx>

#include <memory>

#include <kurento-client/KurentoClientLogDefs.h>

class ReConKurentoClientLogSink : public KurentoClientLogSink
{
public:
   virtual void log(KurentoClientLogSink::Level level, const char *s) override;
};

namespace resip
{
class DialogUsageManager;
class SipMessage;
}

// Disable warning 4250
// VS2019 give a 4250 warning:  
// KurentoRemoteParticipant.hxx(80,1): warning C4250: 'recon::KurentoRemoteParticipant': inherits 'recon::RemoteParticipant::recon::RemoteParticipant::addToConversation' via dominance
#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4250 )
#endif

namespace recon
{
class KurentoConversationManager;

/**
  This class represent a remote participant.  A remote participant is a 
  participant with a network connection to a remote entity.  This
  implementation is for a SIP / RTP connections.
*/

class KurentoRemoteParticipant : public virtual RemoteParticipant, public virtual KurentoParticipant
{
public:
   KurentoRemoteParticipant(ParticipantHandle partHandle,   // UAC
                     KurentoConversationManager& conversationManager,
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet);

   KurentoRemoteParticipant(KurentoConversationManager& conversationManager,            // UAS or forked leg
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet);

   virtual ~KurentoRemoteParticipant();

   virtual void buildSdpOffer(bool holdSdp, resip::SdpContents& offer);

   virtual int getConnectionPortOnBridge();
   virtual bool hasInput() { return true; }
   virtual bool hasOutput() { return true; }
   virtual int getMediaConnectionId();

   virtual void adjustRTPStreams(bool sendingOffer=false);

protected:
   virtual bool mediaStackPortAvailable();

   virtual KurentoRemoteParticipantDialogSet& getKurentoDialogSet() { return dynamic_cast<KurentoRemoteParticipantDialogSet&>(getDialogSet()); };

   virtual void connectToKurento(kurento_client::KurentoClient& client);

private:       
   bool buildSdpAnswer(const resip::SdpContents& offer, resip::SdpContents& answer);

   void setEndpointId(const resip::Data& endpointId);
   resip::Data mEndpointId;
};

}

#endif


/* ====================================================================

 Copyright (c) 2021, SIP Spectrum, Inc. www.sipspectrum.com
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
