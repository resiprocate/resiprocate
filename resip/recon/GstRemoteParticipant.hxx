#if !defined(GstRemoteParticipant_hxx)
#define GstRemoteParticipant_hxx

#include <functional>
#include <map>

#include <media/gstreamer/GstRtpManager.hxx>

#include "ConversationManager.hxx"
#include "Participant.hxx"
#include "RemoteParticipant.hxx"
#include "GstRemoteParticipantDialogSet.hxx"
#include "GstParticipant.hxx"

#include <resip/dum/AppDialogSet.hxx>
#include <resip/dum/AppDialog.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/DialogSetHandler.hxx>
#include <resip/dum/SubscriptionHandler.hxx>

#include <memory>

#include <gst/sdp/gstsdpmessage.h>
#include <glibmm/object.h>

#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>

namespace resip
{
class DialogUsageManager;
class SipMessage;
}

// Disable warning 4250
// VS2019 give a 4250 warning:  
// GstRemoteParticipant.hxx(80,1): warning C4250: 'recon::GstRemoteParticipant': inherits 'recon::RemoteParticipant::recon::RemoteParticipant::addToConversation' via dominance
#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4250 )
#endif

namespace recon
{
class GstMediaStackAdapter;

/**
  This class represent a remote participant.  A remote participant is a 
  participant with a network connection to a remote entity.  This
  implementation is for a SIP / RTP connections.
*/

class GstRemoteParticipant : public virtual RemoteParticipant, public virtual GstParticipant
{
public:
   GstRemoteParticipant(ParticipantHandle partHandle,   // UAC
                     ConversationManager& conversationManager,
                     GstMediaStackAdapter& gstreamerMediaStackAdapter,
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet);

   GstRemoteParticipant(ConversationManager& conversationManager,            // UAS or forked leg
                     GstMediaStackAdapter& gstreamerMediaStackAdapter,
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet);

   virtual ~GstRemoteParticipant();

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

   //virtual std::shared_ptr<gstreamer::BaseRtpEndpoint> getEndpoint() { return mEndpoint; }; // FIXME Gst
   //std::shared_ptr<gstreamer::MediaElement> mMultiqueue; // FIXME Gst
   bool mWaitingModeVideo = false;
   //std::shared_ptr<gstreamer::PlayerEndpoint> mPlayer; // FIXME Gst
   //std::shared_ptr<gstreamer::PassThroughElement> mPassThrough; // FIXME Gst

   virtual void waitingMode();
   //virtual std::shared_ptr<gstreamer::MediaElement> getWaitingModeElement();

   virtual bool onMediaControlEvent(resip::MediaControlContents::MediaControl& mediaControl);
   virtual bool onTrickleIce(resip::TrickleIceContents& trickleIce) override;

   virtual void requestKeyframe() override;

protected:
   virtual bool mediaStackPortAvailable();

   virtual GstRemoteParticipantDialogSet& getGstDialogSet() { return dynamic_cast<GstRemoteParticipantDialogSet&>(getDialogSet()); };

   virtual bool holdPreferExistingSdp() override { return true; };

private:
   //gstreamer::BaseRtpEndpoint* newEndpoint();
   void linkOutgoingPipeline(unsigned int streamId, Glib::RefPtr<Gst::Bin> bin, const Glib::RefPtr<Gst::Caps> caps);
   virtual bool initEndpointIfRequired(bool isWebRTC);
   //virtual void doIceGathering(gstreamer::ContinuationString sdpReady);
   //typedef Glib::ustring StreamKey;
   typedef resip::Data StreamKey;
   virtual StreamKey getKeyForStream(const Glib::RefPtr<Gst::Caps>& caps) const;
   virtual bool isWebRTCSession() const;
   virtual Glib::RefPtr<Gst::Pad> createIncomingPipeline(Glib::RefPtr<Gst::Pad> pad);
   //virtual void createOutgoingPipeline(const Glib::RefPtr<Gst::Pad>& pad);
   //typedef Gst::EncodeBin EncodeEntry;
   //typedef Gst::Queue EncodeEntry;
   typedef Gst::Bin EncodeEntry;
   //typedef Gst::DecodeBin DecodeEntry;
   typedef Gst::Bin DecodeEntry;
   virtual Glib::RefPtr<Gst::Bin> createOutgoingPipeline(const Glib::RefPtr<Gst::Caps> caps);
   virtual void debugGraph();
   virtual void createAndConnectElements(bool isWebRTC, std::function<void()> cConnected, CallbackSdpReady sdpReady);
   virtual void createAndConnectElementsWebRTC(std::function<void()> cConnected, CallbackSdpReady sdpReady);
   virtual void createAndConnectElementsStandard(std::function<void()> cConnected, CallbackSdpReady sdpReady);
   virtual void setLocalDescription(GstWebRTCSessionDescription* gstwebrtcdesc);
   virtual void setRemoteDescription(GstWebRTCSessionDescription* gstwebrtcdesc);
   virtual void buildSdpAnswer(const resip::SdpContents& offer, CallbackSdpReady sdpReady) override;

   bool onGstBusMessage(const Glib::RefPtr<Gst::Bus>&, const Glib::RefPtr<Gst::Message>& message);
   virtual void onRemoteIceCandidate(const resip::Data& candidate, unsigned int lineIndex, const resip::Data& mid);

   //std::shared_ptr<gstreamer::BaseRtpEndpoint> mEndpoint;
   volatile bool mIceGatheringDone;  // FIXME Gst use a concurrency primitive, e.g. condition_variable

   std::chrono::time_point<std::chrono::steady_clock> mLastLocalKeyframeRequest = std::chrono::steady_clock::now();

   Glib::RefPtr<Gst::Caps> mCapsVideo;
   Glib::RefPtr<Gst::Caps> mCapsAudio;

public: // FIXME
   bool mRemoveExtraMediaDescriptors;
   bool mSipRtpEndpoint;
   bool mReuseSdpAnswer;
   bool mWSAcceptsKeyframeRequests;
   resip::SdpContents* mLastRemoteSdp;
   bool mWaitingAnswer;  // have sent an offer, waiting for peer to send answer
   bool mTrickleIcePermitted = false; // FIXME - complete
   bool mWebRTCOutgoing = false; // FIXME - use WebRTC for outgoing call

   //GstElement* mPipeline;
   //GstElement* mRtpTransportElement;
   Glib::RefPtr<Gst::Pipeline> mPipeline;
   Glib::RefPtr<Gst::Element> mRtpTransportElement;

   std::map<StreamKey, Glib::RefPtr<DecodeEntry>> mDecodes;
   std::map<StreamKey, Glib::RefPtr<EncodeEntry>> mEncodes;

   std::function<void(const Glib::RefPtr<Gst::Pad>& pad)> mPadAdded;

   /*class MyWebRTCElement : public Gst::Element
         {
            public:
               MyWebRTCElement() : Gst::Element(Glib::ConstructParams()) {};
         };*/

   std::shared_ptr<resipgst::GstRtpManager> mRtpManager = 0;  // FIXME - move to MediaStackAdapter
   std::shared_ptr<resipgst::GstRtpSession> mRtpSession = 0;
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
