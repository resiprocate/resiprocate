#if !defined(LibWebRTCRemoteParticipant_hxx)
#define LibWebRTCRemoteParticipant_hxx

#include <map>

#include "ConversationManager.hxx"
#include "Participant.hxx"
#include "RemoteParticipant.hxx"
#include "LibWebRTCRemoteParticipant.hxx"
#include "LibWebRTCRemoteParticipantDialogSet.hxx"
#include "LibWebRTCParticipant.hxx"

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
// LibWebRTCRemoteParticipant.hxx(80,1): warning C4250: 'recon::LibWebRTCRemoteParticipant': inherits 'recon::RemoteParticipant::recon::RemoteParticipant::addToConversation' via dominance
#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4250 )
#endif

namespace recon
{
class LibWebRTCMediaStackAdapter;

/**
  This class represent a remote participant.  A remote participant is a 
  participant with a network connection to a remote entity.  This
  implementation is for a SIP / RTP connections.
*/

class LibWebRTCRemoteParticipant : public virtual RemoteParticipant, public virtual LibWebRTCParticipant
{
public:
   LibWebRTCRemoteParticipant(ParticipantHandle partHandle,   // UAC
                     ConversationManager& conversationManager,
                     LibWebRTCMediaStackAdapter& libwebrtcMediaStackAdapter,
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet);

   LibWebRTCRemoteParticipant(ConversationManager& conversationManager,            // UAS or forked leg
                     LibWebRTCMediaStackAdapter& libwebrtcMediaStackAdapter,
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet);

   virtual ~LibWebRTCRemoteParticipant();

   virtual void buildSdpOffer(bool holdSdp, CallbackSdpReady sdpReady, bool preferExistingSdp = false);

   virtual int getConnectionPortOnBridge();
   virtual bool hasInput() { return true; }
   virtual bool hasOutput() { return true; }
   virtual int getMediaConnectionId();

   virtual void applyBridgeMixWeights() override;
   virtual void applyBridgeMixWeights(Conversation* removedConversation) override;

   virtual void adjustRTPStreams(bool sendingOffer=false);

   void onIceGatheringDone() { mIceGatheringDone = true; };

   virtual void addToConversation(Conversation *conversation, unsigned int inputGain = 100, unsigned int outputGain = 100) override;
   virtual void removeFromConversation(Conversation *conversation) override;

   virtual std::shared_ptr<libwebrtc::BaseRtpEndpoint> getEndpoint() { return mEndpoint; }; // FIXME LibWebRTC
   std::shared_ptr<libwebrtc::MediaElement> mMultiqueue; // FIXME LibWebRTC
   bool mWaitingModeVideo = false;
   std::shared_ptr<libwebrtc::PlayerEndpoint> mPlayer; // FIXME LibWebRTC
   std::shared_ptr<libwebrtc::PassThroughElement> mPassThrough; // FIXME LibWebRTC

   virtual void waitingMode();
   virtual std::shared_ptr<libwebrtc::MediaElement> getWaitingModeElement();

   virtual bool onMediaControlEvent(resip::MediaControlContents::MediaControl& mediaControl);
   virtual bool onTrickleIce(resip::TrickleIceContents& trickleIce) override;

   virtual void requestKeyframe() override;

protected:
   virtual bool mediaStackPortAvailable();

   virtual LibWebRTCRemoteParticipantDialogSet& getLibWebRTCDialogSet() { return dynamic_cast<LibWebRTCRemoteParticipantDialogSet&>(getDialogSet()); };

   virtual bool holdPreferExistingSdp() override { return true; };

private:
   libwebrtc::BaseRtpEndpoint* newEndpoint();
   virtual bool initEndpointIfRequired(bool isWebRTC);
   virtual void doIceGathering(libwebrtc::ContinuationString sdpReady);
   virtual void createAndConnectElements(libwebrtc::ContinuationVoid cConnected);
   virtual void buildSdpAnswer(const resip::SdpContents& offer, CallbackSdpReady sdpReady) override;

   std::shared_ptr<libwebrtc::BaseRtpEndpoint> mEndpoint;
   volatile bool mIceGatheringDone;  // FIXME LibWebRTC use a concurrency primitive, e.g. condition_variable

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
 Copyright (c) 2022, Daniel Pocock https://danielpocock.com
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
