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

#include <media/kurento/Object.hxx>

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
class KurentoMediaStackAdapter;

/**
  This class represent a remote participant.  A remote participant is a 
  participant with a network connection to a remote entity.  This
  implementation is for a SIP / RTP connections.
*/

class KurentoRemoteParticipant : public virtual RemoteParticipant, public virtual KurentoParticipant
{
public:
   KurentoRemoteParticipant(ParticipantHandle partHandle,   // UAC
                     ConversationManager& conversationManager,
                     KurentoMediaStackAdapter& kurentoMediaStackAdapter,
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet);

   KurentoRemoteParticipant(ConversationManager& conversationManager,            // UAS or forked leg
                     KurentoMediaStackAdapter& kurentoMediaStackAdapter,
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet);

   virtual ~KurentoRemoteParticipant();

   virtual void buildSdpOffer(bool holdSdp, CallbackSdpReady sdpReady, bool preferExistingSdp = false) override;

   virtual int getConnectionPortOnBridge() override;
   virtual bool hasInput() override { return true; }
   virtual bool hasOutput() override { return true; }
   virtual int getMediaConnectionId();

   virtual void applyBridgeMixWeights() override;
   virtual void applyBridgeMixWeights(Conversation* removedConversation) override;

   virtual void adjustRTPStreams(bool sendingOffer=false) override;

   void onIceGatheringDone() { mIceGatheringDone = true; };

   virtual void addToConversation(Conversation *conversation, unsigned int inputGain = 100, unsigned int outputGain = 100) override;
   virtual void removeFromConversation(Conversation *conversation) override;

   virtual std::shared_ptr<kurento::BaseRtpEndpoint> getEndpoint() { return mEndpoint; }; // FIXME Kurento
   std::shared_ptr<kurento::MediaElement> mMultiqueue; // FIXME Kurento
   bool mWaitingModeVideo = false;
   std::shared_ptr<kurento::PlayerEndpoint> mPlayer; // FIXME Kurento
   std::shared_ptr<kurento::PassThroughElement> mPassThrough; // FIXME Kurento

   virtual void waitingMode();
   virtual std::shared_ptr<kurento::MediaElement> getWaitingModeElement();

   virtual bool onMediaControlEvent(resip::MediaControlContents::MediaControl& mediaControl) override;
   virtual bool onTrickleIce(resip::TrickleIceContents& trickleIce) override;

   virtual void requestKeyframe() override;

protected:
   virtual bool mediaStackPortAvailable() override;

   virtual KurentoRemoteParticipantDialogSet& getKurentoDialogSet() { return dynamic_cast<KurentoRemoteParticipantDialogSet&>(getDialogSet()); };

   virtual bool holdPreferExistingSdp() override { return true; };

private:
   kurento::BaseRtpEndpoint* newEndpoint();
   virtual bool initEndpointIfRequired(bool isWebRTC);
   virtual void doIceGathering(kurento::ContinuationString sdpReady);
   virtual void createAndConnectElements(kurento::ContinuationVoid cConnected);
   virtual void buildSdpAnswer(const resip::SdpContents& offer, CallbackSdpReady sdpReady) override;

   std::shared_ptr<kurento::BaseRtpEndpoint> mEndpoint;
   volatile bool mIceGatheringDone;  // FIXME Kurento use a concurrency primitive, e.g. condition_variable

   std::chrono::time_point<std::chrono::steady_clock> mLastLocalKeyframeRequest = std::chrono::steady_clock::now();

public: // FIXME
   bool mRemoveExtraMediaDescriptors;
   bool mSipRtpEndpoint;
   bool mReuseSdpAnswer;
   bool mWSAcceptsKeyframeRequests;
   resip::SdpContents* mLastRemoteSdp;
   bool mWaitingAnswer;  // have sent an offer, waiting for peer to send answer
   bool mTrickleIcePermitted = false; // FIXME - complete
   bool mWebRTCOutgoing = false; // FIXME - use WebRTC for outgoing call
};

}

#endif


/* ====================================================================

 Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2021-2022, Daniel Pocock https://danielpocock.com
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
