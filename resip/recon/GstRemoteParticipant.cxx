#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <regex>

#include "GstMediaStackAdapter.hxx"

#include "GstRemoteParticipant.hxx"
#include "Conversation.hxx"
#include "UserAgent.hxx"
#include "DtmfEvent.hxx"
#include "ReconSubsystem.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/Random.hxx>
#include <resip/stack/DtmfPayloadContents.hxx>
#include <resip/stack/SdpContents.hxx>
#include <resip/stack/SipFrag.hxx>
#include <resip/stack/ExtensionHeader.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/ClientInviteSession.hxx>
#include <resip/dum/ServerInviteSession.hxx>
#include <resip/dum/ClientSubscription.hxx>
#include <resip/dum/ServerOutOfDialogReq.hxx>
#include <resip/dum/ServerSubscription.hxx>

#include <rutil/WinLeakCheck.hxx>
#include <media/gstreamer/GStreamerUtils.hxx>
#include <media/gstreamer/GstRtpManager.hxx>

#include <reflow/HEPRTCPEventLoggingHandler.hxx>

#include <memory>
#include <utility>

#include <glibmm/error.h>
#include <glibmm/signalproxy.h>
#include <gst/gst.h>
#include <gst/gsterror.h>
#include <gst/sdp/gstsdpmessage.h>
#include <gstreamermm/pipeline.h>
//#include <gstreamermm/promise.h>

#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>

using namespace resipgst;
using namespace recon;
using namespace resip;
using namespace std;

using namespace Gst;
using Glib::RefPtr;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

/* The demo code for webrtcbin provides a useful example.
 *
 * This appears to be the current location of the demos:
 *
 * https://gitlab.freedesktop.org/gstreamer/gstreamer/-/tree/main/subprojects/gst-examples/webrtc
 *
 * Previous locations:
 *
 * https://github.com/centricular/gstwebrtc-demos
 * https://github.com/imdark/gstreamer-webrtc-demo
 */


// These are currently used as defaults for creating an SDP offer for WebRTC
#define GST_RTP_CAPS_OPUS "application/x-rtp,media=audio,encoding-name=OPUS,clock-rate=48000,payload=96"
#define GST_RTP_CAPS_VP8 "application/x-rtp,media=video,encoding-name=VP8,clock-rate=90000,payload=97"

// These are currently used as defaults for creating an SDP offer for non-WebRTC
#define GST_RTP_CAPS_PCMA "application/x-rtp,media=audio,encoding-name=PCMA,clock-rate=8000,payload=8"
#define GST_RTP_CAPS_H264 "application/x-rtp,media=video,encoding-name=H264,clock-rate=90000,payload=120"

// used with g_signal_connect
void voidStub(GstElement * webrtcbin, void *u)
{
   StackLog(<<"voidStub invoked");
   std::function<void()> &func = *static_cast<std::function<void()>*>(u);
   func();
}
/*void propertyStateStub(GstElement * webrtcbin, GParamSpec * pspec,
   void *u)
{
   std::function<void(GParamSpec *)> &func = *static_cast<std::function<void(GParamSpec *)>*>(u);
   func(pspec);
}*/

void promise_stub(GstPromise* promise, void *u) {
  StackLog(<<"promise_stub invoked");
  // context is static_cast<void*>(&f) below. We reverse the cast to
  // void* and call the std::function object.
  auto _u = static_cast<std::function<void(GstPromise* , gpointer)>*>(u);
  std::function<void(GstPromise*, gpointer)> &func = *_u;
  func(promise, NULL);
  delete _u;  // each promise has a copy of the function object that must be deleted
}

// UAC
GstRemoteParticipant::GstRemoteParticipant(ParticipantHandle partHandle,
                                     ConversationManager& conversationManager,
                                     GstMediaStackAdapter& gstreamerMediaStackAdapter,
                                     DialogUsageManager& dum,
                                     RemoteParticipantDialogSet& remoteParticipantDialogSet)
: Participant(partHandle, ConversationManager::ParticipantType_Remote, conversationManager),
  RemoteParticipant(partHandle, conversationManager, dum, remoteParticipantDialogSet),
  GstParticipant(partHandle, ConversationManager::ParticipantType_Remote, conversationManager, gstreamerMediaStackAdapter),
  mIceGatheringDone(false),
  mRemoveExtraMediaDescriptors(false),
  mSipRtpEndpoint(true),
  mReuseSdpAnswer(false),
  mWSAcceptsKeyframeRequests(true),
  mLastRemoteSdp(0),
  mWaitingAnswer(false),
  mWebRTCOutgoing(getDialogSet().getConversationProfile()->mediaEndpointMode() == ConversationProfile::WebRTC)
{
   InfoLog(<< "GstRemoteParticipant created (UAC), handle=" << mHandle);

   initRtpManager();
}

// UAS - or forked leg
GstRemoteParticipant::GstRemoteParticipant(ConversationManager& conversationManager,
                                     GstMediaStackAdapter& gstreamerMediaStackAdapter,
                                     DialogUsageManager& dum, 
                                     RemoteParticipantDialogSet& remoteParticipantDialogSet)
: Participant(ConversationManager::ParticipantType_Remote, conversationManager),
  RemoteParticipant(conversationManager, dum, remoteParticipantDialogSet),
  GstParticipant(ConversationManager::ParticipantType_Remote, conversationManager, gstreamerMediaStackAdapter),
  mIceGatheringDone(false),
  mRemoveExtraMediaDescriptors(false),
  mSipRtpEndpoint(true),
  mReuseSdpAnswer(false),
  mWSAcceptsKeyframeRequests(true),
  mLastRemoteSdp(0),
  mWaitingAnswer(false),
  mWebRTCOutgoing(getDialogSet().getConversationProfile()->mediaEndpointMode() == ConversationProfile::WebRTC)
{
   InfoLog(<< "GstRemoteParticipant created (UAS or forked leg), handle=" << mHandle);

   initRtpManager();
}

void
GstRemoteParticipant::initRtpManager()
{
   // FIXME - IPv6 needed
   // FIXME - MyMessageDecorator will automatically replace 0.0.0.0 with sending IP
   //         but maybe we can do better
   mRtpManager = make_shared<GstRtpManager>("0.0.0.0");

}

GstRemoteParticipant::~GstRemoteParticipant()
{
   // Note:  Ideally this call would exist in the Participant Base class - but this call requires 
   //        dynamic_casts and virtual methods to function correctly during destruction.
   //        If the call is placed in the base Participant class then these things will not
   //        function as desired because a classes type changes as the descructors unwind.
   //        See https://stackoverflow.com/questions/10979250/usage-of-this-in-destructor.
   unregisterFromAllConversations();

   mPipeline->set_state(STATE_NULL);

   InfoLog(<< "GstRemoteParticipant destroyed, handle=" << mHandle);
}

int 
GstRemoteParticipant::getConnectionPortOnBridge()
{
   if(getDialogSet().getActiveRemoteParticipantHandle() == mHandle)
   {
      return -1;  // FIXME Gst
   }
   else
   {
      // If this is not active fork leg, then we don't want to effect the bridge mixer.  
      // Note:  All forked endpoints/participants have the same connection port on the bridge
      return -1;
   }
}

int 
GstRemoteParticipant::getMediaConnectionId()
{ 
   return getGstDialogSet().getMediaConnectionId();
}

void
GstRemoteParticipant::applyBridgeMixWeights()
{
   // FIXME Gst - do we need to implement this?
}

// Special version of this call used only when a participant
// is removed from a conversation.  Required when sipXConversationMediaInterfaceMode
// is used, in order to get a pointer to the bridge mixer
// for a participant (ie. LocalParticipant) that has no currently
// assigned conversations.
void
GstRemoteParticipant::applyBridgeMixWeights(Conversation* removedConversation)
{
   // FIXME Gst - do we need to implement this?
}

void
GstRemoteParticipant::linkOutgoingPipeline(unsigned int streamId, RefPtr<Bin> bin, const Glib::RefPtr<Gst::Caps> caps)
{
   RefPtr<Pad> src = bin->get_static_pad("src");

   RefPtr<Caps> padCaps = Caps::create_from_string("application/x-rtp");
   RefPtr<Gst::PadTemplate> tmpl = PadTemplate::create("send_rtp_sink_%u", PAD_SINK, PAD_REQUEST, padCaps);
   ostringstream padName;
   padName << "sink_" << streamId;
   DebugLog(<< "linking to " << padName.str());
   //RefPtr<Pad> binSink = mRtpTransportElement->create_compatible_pad(queueSrc, caps);
   //RefPtr<Pad> binSink = mRtpTransportElement->get_request_pad(padName.str());
   //RefPtr<Caps> padCaps2 = Caps::create_from_string("application/x-rtp");
   //RefPtr<Pad> binSink = mRtpTransportElement->request_pad(tmpl, padName.str(), padCaps2);
   // is a ghost pad, should have been created already

   // it is static_pad on the rtpbin wrapper
   RefPtr<Pad> binSink;
   bool isWebRTC = isWebRTCSession();
   if(isWebRTC)
   {
      // and it is a request pad on the webrtcbin
      binSink = mRtpTransportElement->create_compatible_pad(src, caps);
   }
   else
   {
      binSink = mRtpTransportElement->get_static_pad(padName.str());
   }
   if(!binSink)
   {
      CritLog(<<"failed to get request pad " << padName.str());
      resip_assert(0);
   }

   src->link(binSink);

   debugGraph();
}

void
sendCapsEvent(RefPtr<Bin> bin, RefPtr<Caps> caps)
{
   // webrtcbin seems to expect this
   // https://gitlab.freedesktop.org/gstreamer/gst-plugins-bad/-/issues/1055#note_631630

   // send RTP caps into the sink towards the webrtcbin transport bin
   RefPtr<Gst::EventCaps> capsEvent = Gst::EventCaps::create(caps);
   RefPtr<Gst::Element> queueOut = RefPtr<Element>::cast_dynamic(bin->get_child("queueOut"));
   queueOut->get_static_pad("sink")->send_event(capsEvent);

   // send audio/raw-something caps into the bin
   //RefPtr<Gst::EventCaps> capsEvent = Gst::EventCaps::create(___caps);
   //sink->send_event(capsEvent);
}

/*unsigned int
getStreamIndex(const Data& streamName)
{
   if(streamName == "audio")
   {
      return 0;
   }
   else if(streamName == "video")
   {
      return 1;
   }
   else
   {
      ErrLog(<<"unknown stream name: " << streamName);
      resip_assert(0);
   }
   return -1;
}*/

void
GstRemoteParticipant::prepareStream(unsigned int streamId, bool loopbackMode)
{
   GstRtpSession::CapsVector capsV = mRtpSession->getOutgoingCaps();
   RefPtr<Caps> caps;
   if(capsV.size() > streamId && capsV[streamId])
   {
      caps = capsV[streamId];
   }

   if(!caps)
   {
      ErrLog(<<"caps not specified");
      return;
   }

   RefPtr<Pad> mediaSink = mRtpSession->getOutgoingPads()[streamId];

   MediaTypeName mediaTypeName = getMediaTypeName(caps);
   resip_assert(mediaTypeName.size());

   //RefPtr<Bin> bin = mRtpSession->createOutgoingPipeline(caps);
   //mPipeline->add(bin);
   // unsigned int streamId = getStreamIndex(streamKey);    // FIXME streamId
   //linkOutgoingPipeline(streamId, bin, caps);
   /*RefPtr<Pad> mediaSink = mRtpSession->createMediaSink(caps, streamId);
   resip_assert(mediaSink);
   mEncodes[mediaTypeName] = mediaSink;*/

   if(loopbackMode)
   {
      // FIXME - move this logic to GstRtpSession
      //sendCapsEvent(bin, caps);
   }
   else
   {
      RefPtr<Element> testSrc = buildTestSource(mediaTypeName);
      resip_assert(testSrc);
      mPipeline->add(testSrc);
      testSrc->get_static_pad("src")->link(mediaSink);
   }
}


bool
GstRemoteParticipant::initEndpointIfRequired(bool isWebRTC)
{
   if(mPipeline)
   {
      DebugLog(<<"mPipeline already exists");
      return false;
   }
   if(!isWebRTC)
   {
      // skip the ICE phase as we don't do ICE for non-WebRTC yet
      mIceGatheringDone = true;
   }
   else
   {
      mIceGatheringDone = false;
   }

   mPipeline = Gst::Pipeline::create();
   mMediaBin = mRtpSession->getMediaBin();
   resip_assert(mMediaBin);
   mRtpTransportElement = mRtpSession->getRtpTransportBin();
   resip_assert(mRtpTransportElement);

   if(isWebRTC)
   {
      // FIXME: use a glibmm-style property
      // FIXME: per-peer bundle-policy
      // FIXME: move to config file
      // https://datatracker.ietf.org/doc/html/draft-ietf-rtcweb-jsep-24#section-4.1.1
      // https://gstreamer.freedesktop.org/documentation/webrtc/index.html?gi-language=c#webrtcbin:bundle-policy
      //GstWebRTCBundlePolicy bundlePolicy = GST_WEBRTC_BUNDLE_POLICY_MAX_BUNDLE;
      GstWebRTCBundlePolicy bundlePolicy = GST_WEBRTC_BUNDLE_POLICY_MAX_COMPAT;
      GValue val = G_VALUE_INIT;
      g_value_init (&val, G_TYPE_ENUM);
      g_value_set_enum (&val, bundlePolicy);
      g_object_set_property(G_OBJECT(mRtpTransportElement->gobj()), "bundle-policy", &val);

      std::shared_ptr<resip::ConfigParse> cfg = mConversationManager.getConfig();
      if(cfg)
      {
         // FIXME: support TURN too
         const Data& natTraversalMode = mConversationManager.getConfig()->getConfigData("NatTraversalMode", "", true);
         const Data& stunServerName = mConversationManager.getConfig()->getConfigData("NatTraversalServerHostname", "", true);
         const int stunServerPort = mConversationManager.getConfig()->getConfigInt("NatTraversalServerPort", 3478);
         if(natTraversalMode == "Bind" && !stunServerName.empty() && stunServerPort > 0)
         {
            Data stunUrl = "stun://" + stunServerName + ":" + stunServerPort;
            InfoLog(<<"using STUN config: " << stunUrl);
            mRtpTransportElement->property<Glib::ustring>("stun-server", stunUrl.c_str());
         }
//#define GST_TURN_USER "test"   // FIXME - need to read this from the config file
//#define GST_TURN_PASSWORD ""
#ifdef GST_TURN_PASSWORD
         else
         {
            ostringstream u;
            u << "turn://"
              << GST_TURN_USER
              << ":"
              << GST_TURN_PASSWORD
              << "@195.8.117.61";  // FIXME read from config
            Glib::ustring turnUrl = u.str();
            mRtpTransportElement->property<Glib::ustring>("turn-server", turnUrl);
            InfoLog(<<"using TURN config: " << turnUrl); // FIXME security - logs password

         }
#endif
      }
   }

   //mPipeline->add(mRtpTransportElement);
   mPipeline->add(mMediaBin);

   // need to create all sink pads on the webrtcbin before it can be used

   mRtpSession->initOutgoingBins();

   // when using webrtcbin we can't go straight into loopback, it looks
   // like we need to create some other source elements and put some caps events
   // and maybe some media through the pipeline before getting the event
   // on-negotiation-needed and completing the webrtcbin setup
   bool loopbackMode = !isWebRTC;

   unsigned int streamCount = mRtpSession->getOutgoingCaps().size();
   if(streamCount == 0)
   {
      // sending an offer
      InfoLog(<<"creating streams for SDP offer");
      // FIXME - these are currently hardcoded in GstRtpSession::buildOffer
      //mRtpSession->addStream(Gst::Caps::create_from_string(GST_RTP_CAPS_PCMA));
      //mRtpSession->addStream(Gst::Caps::create_from_string(GST_RTP_CAPS_H264));
   }
   else
   {
      // received an offer
      InfoLog(<<"creating streams for SDP answer");
      for(unsigned int streamId = 0; streamId < streamCount; streamId++)
      {
         prepareStream(streamId, loopbackMode);
      }
   }

   // FIXME
   RefPtr<Bus> bus = mPipeline->get_bus();
   bus->add_watch(sigc::mem_fun(*this, &GstRemoteParticipant::onGstBusMessage));

   DebugLog(<<"mPipeline and mRtpTransportElement created");

   debugGraph();

   return true;
}

/*void
GstRemoteParticipant::doIceGathering(gstreamer::ContinuationString sdpReady)
{
   std::shared_ptr<gstreamer::WebRtcEndpoint> webRtc = std::static_pointer_cast<gstreamer::WebRtcEndpoint>(mEndpoint);

   std::shared_ptr<gstreamer::EventContinuation> elEventIceCandidateFound =
         std::make_shared<gstreamer::EventContinuation>([this](std::shared_ptr<gstreamer::Event> event){
      DebugLog(<<"received event: " << *event);
      std::shared_ptr<gstreamer::OnIceCandidateFoundEvent> _event =
         std::dynamic_pointer_cast<gstreamer::OnIceCandidateFoundEvent>(event);
      resip_assert(_event.get());

      if(!mTrickleIcePermitted)
      {
         return;
      }
      // FIXME - if we are waiting for a previous INFO to be confirmed,
      //         aggregate the candidates into a vector and send them in bulk
      auto ice = getLocalSdp()->session().makeIceFragment(Data(_event->getCandidate()),
         _event->getLineIndex(), Data(_event->getId()));
      if(ice.get())
      {
         StackLog(<<"about to send " << *ice);
         info(*ice);
      }
      else
      {
         WarningLog(<<"failed to create ICE fragment for mid: " << _event->getId());
      }
   });

   std::shared_ptr<gstreamer::EventContinuation> elIceGatheringDone =
            std::make_shared<gstreamer::EventContinuation>([this, sdpReady](std::shared_ptr<gstreamer::Event> event){
      mIceGatheringDone = true;
      mEndpoint->getLocalSessionDescriptor(sdpReady);
   });

   webRtc->addOnIceCandidateFoundListener(elEventIceCandidateFound, [=](){
      webRtc->addOnIceGatheringDoneListener(elIceGatheringDone, [=](){
         webRtc->gatherCandidates([]{
                  // FIXME - handle the case where it fails
                  // on success, we continue from the IceGatheringDone event handler
         }); // gatherCandidates
      });
   });
}*/

bool
GstRemoteParticipant::isWebRTCSession() const
{
   // FIXME - find a better way to do this
   return mRtpSession->isWebRTC();
}

void
GstRemoteParticipant::onMediaSourceAdded(const RefPtr<Pad>& pad)
{
   Glib::ustring padName = pad->get_name();
   DebugLog(<<"onMediaSourceAdded: on-pad-added, padName: " << padName << " stream ID: " << pad->get_stream_id().raw());
   if(pad->get_direction() == PAD_SRC)
   {
      RefPtr<Caps> caps = pad->get_current_caps();
      /*StreamKey key = getMediaTypeName(caps);
      if(key.empty())
      {
         key = deduceKeyForPadName(padName);
      }
      resip_assert(key.size());*/
      MediaTypeName mediaTypeName = deduceKeyForPadName(padName);
      int streamId = getStreamIdFromPadName(padName);
      if(streamId < 0)
      {
         ErrLog(<<"unable to find encoder for " << padName);
      }
      else
      {
         auto sink = mRtpSession->getOutgoingPads()[streamId];

         if(!sink)
         {
            ErrLog(<<"no encoder configured for stream " << streamId);
            return;
         }

         bool loopback = !isWebRTCSession();

         /*RefPtr<Bin> enc = _enc->second;
         RefPtr<Pad> sink = enc->get_static_pad("sink");*/
         //RefPtr<Pad> sink = _enc->second;
         if(sink->is_linked())
         {
            StackLog(<<"unlinking existing src");
            RefPtr<Pad> src = sink->get_peer();
            if(src->unlink(sink))
            {
               loopback = true;

               // FIXME - destroy the unlinked test source

            }
            else
            {
               ErrLog(<<"failed to unlink existing src");
               resip_assert(0);
            }
         }

         if(loopback)
         {
            StackLog(<<"attempting loopback configuration");
            RefPtr<Element> proxy;
            if(mediaTypeName == "video")
            {
               proxy = Gst::VideoConvert::create();
            }
            else if(mediaTypeName == "audio")
            {
               proxy = Gst::AudioConvert::create();
            }
            else
            {
               CritLog(<<"unexpected streamKey: " << mediaTypeName);
               resip_assert(0);
            }
            mPipeline->add(proxy);
            // convert->link(enc);
            proxy->get_static_pad("src")->link(sink);
            pad->link(proxy->get_static_pad("sink"));
            proxy->sync_state_with_parent();

            mMediaBin->sync_state_with_parent();

            // FIXME - here we just loop the incoming to outgoing,
            //         but we should do that in the Conversation class instead
            DebugLog(<<"completed connection from incoming to outgoing stream");
         }
         else
         {
            // FIXME - disconnect the test sources and do the loopback
            ErrLog(<<"not doing loopback for WebRTC");
         }

         debugGraph();

         // FIXME - delete, now in GstRtpSession
         /*DebugLog(<<"mDecodes.size() == " << mDecodes.size());
         shared_ptr<flowmanager::HEPRTCPEventLoggingHandler> handler =
                  std::dynamic_pointer_cast<flowmanager::HEPRTCPEventLoggingHandler>(
                  mConversationManager.getMediaStackAdapter().getRTCPEventLoggingHandler());
         if(handler && mDecodes.size() == mEncodes.size())
         {
            DebugLog(<<"all pads ready, trying to setup HOMER HEP RTCP");
            //resip::addGstreamerRtcpMonitoringPads(RefPtr<Bin>::cast_dynamic(mRtpTransportElement));
            // Now all audio and video pads, incoming and outgoing,
            // are present.  We can enable the RTCP signals.  If we
            // ask for these signals before the pads are ready then
            // it looks like we don't receive any signals at all.
            RtcpPeerSpecVector peerSpecs = resip::createRtcpPeerSpecs(*getLocalSdp(), *getRemoteSdp());
            const Data& correlationId = getDialogSet().getDialogSetId().getCallId();
            //addGstreamerRtcpMonitoringPads(RefPtr<Bin>::cast_dynamic(mRtpTransportElement),
            //         handler->getHepAgent(), peerSpecs, correlationId);
         }*/

      }
   }
   else
   {
      StackLog(<<"not a source pad: " << padName);
   }
}

RefPtr<Bin>
GstRemoteParticipant::createIncomingPipeline(RefPtr<Pad> pad)
{
   DebugLog(<<"createIncomingPipeline: " << pad->get_name());
   resip_assert(0);  // FIXME - deprecated method
   return RefPtr<Bin>();

   MediaTypeName mediaType = getMediaTypeName(pad->get_current_caps());
   if(mediaType.empty())
   {
      mediaType = deduceKeyForPadName(pad->get_name());
   }

   //RefPtr<Bin> decodeBin = mRtpSession->createDecodeBin(mediaType, pad->get_name(), isWebRTCSession());

   //mDecodes[mediaType] = decodeBin;
   //mPipeline->add(decodeBin);

   //decodeBin->signal_pad_added().connect([this, streamKey](const RefPtr<Pad>& pad){
   //});
   //decodeBin->signal_pad_added().connect(sigc::mem_fun(*this, &GstRemoteParticipant::onMediaSourceAdded));

   // Setup the keyframe request probe
   if(mediaType == "video")
   {
      addGstreamerKeyframeProbe(pad, [this](){requestKeyframeFromPeer();});
   }

   //decodeBin->sync_state_with_parent();

   //return decodeBin;
}

void
GstRemoteParticipant::debugGraph()
{
   RefPtr<Gst::Bin> bin = mPipeline.cast_dynamic(mPipeline);
   storeGstreamerGraph(bin, "reCon-gst-graph");
}

void
GstRemoteParticipant::createAndConnectElements(bool isWebRTC, std::function<void()> cConnected, CallbackSdpReady sdpReady)
{

   if(isWebRTC)
   {
      createAndConnectElementsWebRTC(cConnected, sdpReady);
   }
   else
   {
      createAndConnectElementsStandard(cConnected, sdpReady);
   }
}

void
GstRemoteParticipant::createAndConnectElementsStandard(std::function<void()> cConnected, CallbackSdpReady sdpReady)
{
   DebugLog(<<"going to STATE_READY");
   mPipeline->set_state(STATE_READY);

   mMediaBin->signal_pad_added().connect(sigc::mem_fun(*this, &GstRemoteParticipant::onMediaSourceAdded));
   /*mRtpTransportElement->signal_pad_added().connect([this](const RefPtr<Pad>& pad){
      Glib::ustring padName = pad->get_name();
      DebugLog(<<"WebRTCElement: on-pad-added, padName: " << padName << " stream ID: " << pad->get_stream_id());
      if(pad->get_direction() == PAD_SRC)
      {
         //RefPtr<Caps> rtcpCaps = Gst::Caps::create_from_string("application/x-rtcp");


         // extract ID
         /*std::regex r("recv_rtp_src_(\\d+)_(\\d+)");
         std::smatch match;
         const std::string& s = padName.raw();
         if (std::regex_search(s.begin(), s.end(), match, r))
         {
            unsigned int streamId = atoi(match[1].str().c_str());
            auto& ssrc = match[2];
            StackLog(<<"streamId: " << streamId << " SSRC: " << ssrc);
            RefPtr<Gst::Bin> dec = mDecBin.at(streamId);
            sinkPad = dec->get_static_pad("sink");

            // FIXME - setup HOMER if all pads connected
            //
            // Now all audio and video pads, incoming and outgoing,
            // are present.  We can enable the RTCP signals.  If we
            // ask for these signals before the pads are ready then
            // it looks like we don't receive any signals at all.
            //addGstreamerRtcpMonitoringPads(rtpbin, mHepAgent, mPeerSpecs, mCorrelationId);

            PadLinkReturn ret = newPad->link(sinkPad);

            if (ret != PAD_LINK_OK && ret != PAD_LINK_WAS_LINKED)
            {
               DebugLog(<< "Linking of pads " << padName << " and "
                        << sinkPad->get_name() << " failed.");
            }
            DebugLog(<<"linking done");
            return;
         }* /

         if(regex_search(padName.raw(), regex("^(in|out)bound")))
         {
            DebugLog(<<"found RTCP intercept pad: " << pad->get_name());
            // FIXME - for intercepting RTCP flows in Gstreamer rtpbin
            //resip::linkGstreamerRtcpHomer(mPipeline, pad, hepAgent, peerSpacs, correlationId);
         }
         else
         {
            RefPtr<Bin> incomingBin = createIncomingPipeline(pad);
            pad->link(incomingBin->get_static_pad("sink"));
         }

         debugGraph();
      }
   });*/

   //mRtpTransportElement->sync_state_with_parent();

   DebugLog(<<"going to STATE_PLAYING");
   StateChangeReturn ret = mPipeline->set_state(STATE_PLAYING);
   if (ret == STATE_CHANGE_FAILURE)
   {
      ErrLog(<<"pipeline fail");
      mPipeline.reset();
      sdpReady(false, nullptr);
   }

   mRtpSession->onPlaying(); // FIXME - use a signal in GstRtpSession

   //sdpReady(true, mRtpSession->getLocal());
   cConnected();
}

void
GstRemoteParticipant::createAndConnectElementsWebRTC(std::function<void()> cConnected, CallbackSdpReady sdpReady)
{
   // FIXME: store return value, destroy in destructor

   Glib::signal_any<void>(mRtpTransportElement, "on-negotiation-needed").connect([this, cConnected]()
   {
      DebugLog(<<"onNegotiationNeeded invoked");
      // FIXME - do we need to do anything here?
      // The negotiation effort will be initiated by the
      // requests from the SIP peer or local UA.

      // From looking at the sendrecv demo, it appears that this
      // callback is a hint that the webrtcbin element is in
      // a suitable state for the application to call create-offer

      cConnected();
   });

   Glib::signal_any<void,guint,gchararray>(mRtpTransportElement, "on-ice-candidate").connect([this](guint mline_index, gchararray candidate)
   {
      DebugLog(<<"cOnIceCandidate invoked");
      Data mid("0");  // FIXME - why doesn't webrtcbin provide mid?
      onLocalIceCandidate(candidate, mline_index, mid);
   });

   Glib::signal_any<void,GParamSpec*>(mRtpTransportElement, "notify::ice-gathering-state").connect_notify([this, sdpReady](GParamSpec * pspec)
   {
      DebugLog(<<"cIceGatheringStateNotify invoked");
      GstWebRTCICEGatheringState ice_gather_state;
      g_object_get (mRtpTransportElement->gobj(), "ice-gathering-state", &ice_gather_state,
         NULL);
      const Data& newState = lookupGstreamerStateName(iCEGatheringStates, ice_gather_state);
      DebugLog(<<"ICE gathering state: " << newState);

      if(ice_gather_state == GST_WEBRTC_ICE_GATHERING_STATE_COMPLETE)
      {
         GstWebRTCSessionDescription *_gstwebrtcsdp = nullptr;
         g_object_get (mRtpTransportElement->gobj(), "local-description", &_gstwebrtcsdp,
            NULL);
         if(!_gstwebrtcsdp)
         {
            ErrLog(<<"failed to obtain local-description");
            sdpReady(false, nullptr);
         }

         /* Send SDP to peer */
         std::unique_ptr<SdpContents> _sdp(createSdpContentsFromGstreamer(_gstwebrtcsdp));
         gst_webrtc_session_description_free (_gstwebrtcsdp);
         //_sdp->session().transformLocalHold(isHolding());  // FIXME - Gstreamer can hold?

         mRtpSession->setLocalSdp(std::shared_ptr<SdpContents>(dynamic_cast<SdpContents*>(_sdp->clone())));
         sdpReady(true, std::move(_sdp));
      }
   });

   DebugLog(<<"going to STATE_READY");
   StateChangeReturn ret = mPipeline->set_state(STATE_READY);
   if (ret == STATE_CHANGE_FAILURE)
   {
      ErrLog(<<"pipeline fail");
      mPipeline.reset();
      sdpReady(false, nullptr);
   }

   Glib::signal_any<void,GParamSpec*>(mRtpTransportElement, "notify::ice-connection-state").connect_notify([this](GParamSpec * pspec)
   {
      GstWebRTCICEConnectionState ice_connection_state;
      g_object_get (mRtpTransportElement->gobj(), "ice-connection-state", &ice_connection_state,
         NULL);
      const Data& newState = lookupGstreamerStateName(iCEConnectionStates, ice_connection_state);
      DebugLog(<<"ICE connection state: " << newState);
      if(ice_connection_state == GST_WEBRTC_ICE_CONNECTION_STATE_CONNECTED)
      {
         InfoLog(<<"ICE connected");
      }
   });

   // FIXME - commented out while trying to resolve missing on-negotiation-needed
   Glib::signal_any<void,GParamSpec*>(mRtpTransportElement, "notify::signaling-state").connect_notify([this](GParamSpec * pspec)
   {
      ErrLog(<<"signaling-state");
      GstWebRTCSignalingState signaling_state;
      g_object_get (mRtpTransportElement->gobj(), "signaling-state", &signaling_state,
         NULL);
      const Data& newState = lookupGstreamerStateName(signalingStates, signaling_state);
      DebugLog(<<"signaling state: " << newState);
   });

   mMediaBin->signal_pad_added().connect(sigc::mem_fun(*this, &GstRemoteParticipant::onMediaSourceAdded));
   /*mRtpTransportElement->signal_pad_added().connect([this](const RefPtr<Pad>& pad){
      DebugLog(<<"WebRTCElement: on-pad-added, stream ID: " << pad->get_stream_id());
      if(pad->get_direction() == PAD_SRC)
      {
         RefPtr<Caps> rtcpCaps = Gst::Caps::create_from_string("application/x-rtcp");
         Glib::ustring padName = pad->get_name();
         if(!regex_search(padName.raw(), regex("^(in|out)bound")))
         {
            pad->link(createIncomingPipeline(pad)->get_static_pad("sink"));
         }
         else
         {
            DebugLog(<<"found RTCP pad: " << pad->get_name());
            // FIXME - for intercepting RTCP flows in Gstreamer rtpbin
            //resip::linkGstreamerRtcpHomer(mPipeline, pad, hepAgent, peerSpacs, correlationId);
            debugGraph();
         }
      }
   });*/

   //mRtpTransportElement->sync_state_with_parent();

   DebugLog(<<"going to STATE_PLAYING");
   ret = mPipeline->set_state(STATE_PLAYING);
   if (ret == STATE_CHANGE_FAILURE)
   {
      ErrLog(<<"pipeline fail");
      mPipeline.reset();
      sdpReady(false, nullptr);
   }
}

/*void
GstRemoteParticipant::createAndConnectElements(gstreamer::ContinuationVoid cConnected)
{
   // FIXME - implement listeners for some of the events currently using elEventDebug

   std::shared_ptr<gstreamer::EventContinuation> elError =
         std::make_shared<gstreamer::EventContinuation>([this](std::shared_ptr<gstreamer::Event> event){
      ErrLog(<<"Error from Gst MediaObject: " << *event);
   });

   std::shared_ptr<gstreamer::EventContinuation> elEventDebug =
         std::make_shared<gstreamer::EventContinuation>([this](std::shared_ptr<gstreamer::Event> event){
      DebugLog(<<"received event: " << *event);
   });

   std::shared_ptr<gstreamer::EventContinuation> elEventKeyframeRequired =
         std::make_shared<gstreamer::EventContinuation>([this](std::shared_ptr<gstreamer::Event> event){
      DebugLog(<<"received event: " << *event);
      requestKeyframeFromPeer();
   });

   mEndpoint->create([=]{
      mEndpoint->addErrorListener(elError, [=](){
         mEndpoint->addConnectionStateChangedListener(elEventDebug, [=](){
            mEndpoint->addMediaStateChangedListener(elEventDebug, [=](){
               mEndpoint->addMediaTranscodingStateChangeListener(elEventDebug, [=](){
                  mEndpoint->addMediaFlowInStateChangeListener(elEventDebug, [=](){
                     mEndpoint->addMediaFlowOutStateChangeListener(elEventDebug, [=](){
                        mEndpoint->addKeyframeRequiredListener(elEventKeyframeRequired, [=](){
                           //mMultiqueue->create([this, cConnected]{
                           // mMultiqueue->connect([this, cConnected]{
                           mPlayer->create([this, cConnected]{
                              mPassThrough->create([this, cConnected]{
                                 mEndpoint->connect([this, cConnected]{
                                    mPassThrough->connect([this, cConnected]{
                                       //mPlayer->play([this, cConnected]{
                                       cConnected();
                                       //mPlayer->connect(cConnected, *mEndpoint); // connect
                                       //});
                                    }, *mEndpoint);
                                 }, *mPassThrough);
                              });
                           });
                           //}, *mEndpoint); // mEndpoint->connect
                           // }, *mEndpoint); // mMultiqueue->connect
                           //}); // mMultiqueue->create
                        }); // addKeyframeRequiredListener

                     });
                  });
               });
            });
         });
      });
   }); // create
}*/



void
GstRemoteParticipant::setLocalDescription(GstWebRTCSessionDescription* gstwebrtcdesc)
{
   gstWebRTCSetDescription(mRtpTransportElement, "set-local-description", gstwebrtcdesc);
}

void
GstRemoteParticipant::setRemoteDescription(GstWebRTCSessionDescription* gstwebrtcdesc)
{
   gstWebRTCSetDescription(mRtpTransportElement, "set-remote-description", gstwebrtcdesc);
}

void
GstRemoteParticipant::initRtpSession(bool isWebRTC)
{
   if(!mRtpSession)
   {
      mRtpSession = make_shared<GstRtpSession>(*mRtpManager, isWebRTC);
      mRtpSession->setKeyframeRequestHandler([this](){requestKeyframeFromPeer();});

      shared_ptr<flowmanager::HEPRTCPEventLoggingHandler> handler =
               std::dynamic_pointer_cast<flowmanager::HEPRTCPEventLoggingHandler>(
                        mConversationManager.getMediaStackAdapter().getRTCPEventLoggingHandler());
      if(handler)
      {
         mRtpSession->initHomer(getDialogSet().getDialogSetId().getCallId(),
                  handler->getHepAgent());
      }
   }
}

void
GstRemoteParticipant::buildSdpOffer(bool holdSdp, CallbackSdpReady sdpReady, bool preferExistingSdp)
{
   bool useExistingSdp = false;
   if(getLocalSdp())
   {
      useExistingSdp = preferExistingSdp || mReuseSdpAnswer;
   }

   try
   {
      bool isWebRTC = false; // FIXME

      initRtpSession(isWebRTC);

      // FIXME - these are currently hardcoded in GstRtpSession::buildOffer
      // FIXME - option to select audio only
      // FIXME - other codecs (G.711, H.264, ...)
      /*if(isWebRTC)
      {
         mRtpSession->addLocalCaps(Gst::Caps::create_from_string(GST_RTP_CAPS_VP8));
         mRtpSession->addLocalCaps(Gst::Caps::create_from_string(GST_RTP_CAPS_OPUS));
      }
      else
      {
         mRtpSession->addLocalCaps(Gst::Caps::create_from_string(GST_RTP_CAPS_H264));
         mRtpSession->addLocalCaps(Gst::Caps::create_from_string(GST_RTP_CAPS_PCMA));
      }*/

      bool firstUseEndpoint = initEndpointIfRequired(isWebRTC);

      std::function<void(GstPromise * _promise, gpointer user_data)> cOnOfferReady =
               [this, holdSdp, sdpReady](GstPromise * _promise, gpointer user_data){

         DebugLog(<<"cOnOfferReady invoked");
         GstWebRTCSessionDescription *_gstwebrtcoffer = nullptr;

         if(gst_promise_wait(_promise) != GST_PROMISE_RESULT_REPLIED)
         {
            resip_assert(0);
         }

         // FIXME - overriding the const reply with a cast,
         // need to fix in Gstreamermm API
         Gst::Structure reply((GstStructure*)gst_promise_get_reply (_promise));
         gst_structure_get (reply.gobj(), "offer",
            GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &_gstwebrtcoffer, NULL);
         if(!_gstwebrtcoffer)
         {
            Glib::Error __error;
            reply.get_field<Glib::Error>("error", __error);
            ErrLog(<<"failed to get SDP offer: " << __error.what());
            gst_promise_unref (_promise);
            sdpReady(false, nullptr);
            return;
         }
         gst_promise_unref (_promise);

         setLocalDescription(_gstwebrtcoffer);

         /* Send offer to peer */
         std::shared_ptr<SdpContents> _offer(createSdpContentsFromGstreamer(_gstwebrtcoffer));
         gst_webrtc_session_description_free (_gstwebrtcoffer);
         //_offer->session().transformLocalHold(holdSdp);  // FIXME - tell Gstreamer to give us hold SDP

         setLocalSdpGathering(*_offer);
         mRtpSession->setLocalSdp(_offer);

         // FIXME - this should be done now if there is no ICE (non-WebRTC peer)
         //setProposedSdp(*_offer);
         //sdpReady(true, std::move(_offer));
      };

      std::function<void()> cConnected = [this, cOnOfferReady, sdpReady, isWebRTC]{
         if(isWebRTC)
         {
            // FIXME - can we tell Gst to use holdSdp?
            // We currently mangle the SDP after-the-fact in cOnOfferReady

            // From looking at the webrtcbin demo and related discussions in
            // bug trackers, it appears that all the elements should be added to
            // the pipeline and joined together before invoking create-offer.
            // The caps of the sink pads that have already been created and connected
            // on webrtcbin determine which media descriptors (audio, video)
            // will be present in the SDP offer.
            GstPromise *promise;
            std::function<void(GstPromise * _promise, gpointer user_data)> *_cOnOfferReady =
                     new std::function<void(GstPromise * _promise, gpointer user_data)>(cOnOfferReady);
            promise = gst_promise_new_with_change_func (promise_stub, _cOnOfferReady,
                     NULL);
            g_signal_emit_by_name (mRtpTransportElement->gobj(), "create-offer", NULL, promise);
         }
         else
         {
            bool sendAudio = true;
            bool sendVideo = true;
            std::unique_ptr<SdpContents> offer(dynamic_cast<SdpContents*>(mRtpSession->buildOffer(sendAudio, sendVideo)->clone()));
            mWaitingAnswer = true;

            DebugLog(<<"built offer: " << *offer);
            setProposedSdp(*offer);
            sdpReady(true, std::move(offer));
         }
      };

      if(firstUseEndpoint)
      {
         CallbackSdpReady _sdpReady = [this, sdpReady](bool sdpOk, std::unique_ptr<resip::SdpContents> sdp)
         {
            if(!sdpOk)
            {
               sdpReady(false, std::move(sdp));
               return;
            }
            setProposedSdp(*sdp);
            sdpReady(true, std::move(sdp));
         };

         createAndConnectElements(isWebRTC, cConnected, _sdpReady);
      }
      else
      {
         if(!useExistingSdp)
         {
            cConnected();
         }
         else
         {
            std::ostringstream offerMangledBuf;
            getLocalSdp()->session().transformLocalHold(isHolding());
            offerMangledBuf << *getLocalSdp();
            std::shared_ptr<std::string> offerMangledStr = std::make_shared<std::string>(offerMangledBuf.str());
            // FIXME cOnOfferReady(*offerMangledStr);
         }
      }
   }
   catch(exception& e)
   {
      ErrLog(<<"something went wrong: " << e.what());
      sdpReady(false, nullptr);
   }
}

bool
GstRemoteParticipant::onGstBusMessage(const RefPtr<Gst::Bus>&, const RefPtr<Gst::Message>& message)
{
   switch(message->get_message_type())
   {
      case Gst::MESSAGE_EOS:
         ErrLog(<< "End of stream");
         //main_loop->quit();  // FIXME - from echoTest
         return false;
      case Gst::MESSAGE_ERROR:
         {
            Glib::Error error;
            std::string debug_message;
            RefPtr<MessageError>::cast_static(message)->parse(error, debug_message);
            ErrLog(<< "Error: " << error.what() << std::endl << debug_message);
            // main_loop->quit(); // FIXME - from echoTest
            return false;
         }
      case Gst::MESSAGE_STATE_CHANGED:
         DebugLog(<< "state changed");
         return true;
      case Gst::MESSAGE_NEW_CLOCK:
         DebugLog(<< "new clock");
         return true;
      case Gst::MESSAGE_STREAM_STATUS:
         DebugLog(<< "stream status");
         return true;

      default:
         ErrLog(<<"unhandled Bus Message: " << message->get_message_type());
         return true;
   }

   return true;
}

void
GstRemoteParticipant::buildSdpAnswer(const SdpContents& offer, CallbackSdpReady sdpReady)
{
   bool requestSent = false;

   std::shared_ptr<SdpContents> offerMangled(dynamic_cast<SdpContents*>(offer.clone()));
   while(mRemoveExtraMediaDescriptors && offerMangled->session().media().size() > 2)
   {
      // FIXME hack to remove BFCP
      DebugLog(<<"more than 2 media descriptors, removing the last");
      offerMangled->session().media().pop_back();
   }

   try
   {
      // do some checks on the offer
      // check for video, check for WebRTC
      bool isWebRTC = offerMangled->session().isWebRTC();
      DebugLog(<<"peer is " << (isWebRTC ? "WebRTC":"not WebRTC"));

      initRtpSession(isWebRTC);

      if(!isWebRTC)
      {
         // RFC 4145 uses the attribute name "setup"
         // We override the attribute name and use the legacy name "direction"
         // from the drafts up to draft-ietf-mmusic-sdp-comedia-05.txt
         // Tested with Gst and Cisco EX90
         // https://datatracker.ietf.org/doc/html/draft-ietf-mmusic-sdp-comedia-05
         // https://datatracker.ietf.org/doc/html/rfc4145

         // FIXME was this only necessary for Kurento?
         //offerMangled->session().transformCOMedia("active", "direction");
      }

      std::shared_ptr<GstWebRTCSessionDescription> gstwebrtcoffer;
      if(isWebRTC)
      {
         gstwebrtcoffer = createGstWebRTCSessionDescriptionFromSdpContents(GST_WEBRTC_SDP_TYPE_OFFER, *offerMangled);
         if(!gstwebrtcoffer)
         {
            ErrLog(<<"failed to convert offer to GstWebRTCSessionDescription");
            sdpReady(false, nullptr);
         }
         mRtpSession->setRemoteSdp(offerMangled);
      }
      else
      {
         std::shared_ptr<SdpContents> answer = mRtpSession->buildAnswer(offerMangled);

         DebugLog(<<"built answer: " << *answer);
      }

      bool firstUseEndpoint = initEndpointIfRequired(isWebRTC);

      std::function<void(GstPromise* _promise, gpointer user_data)> cAnswerCreated =
               [this, sdpReady](GstPromise * _promise, gpointer user_data)
               {
         DebugLog(<<"cAnswerCreated invoked");

         if(gst_promise_wait(_promise) != GST_PROMISE_RESULT_REPLIED)
         {
            resip_assert(0);
         }

         GstWebRTCSessionDescription *_gstwebrtcanswer = NULL;
         // FIXME - overriding the const reply with a cast,
         // need to fix in Gstreamermm API
         Gst::Structure reply((GstStructure*)gst_promise_get_reply (_promise));
         // FIXME - need Gst::WebRTCSessionDescription
         //_reply.get_field<Gst::WebRTCSessionDescription>("answer",
         //         _gstwebrtcanswer);
         gst_structure_get (reply.gobj(), "answer",
            GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &_gstwebrtcanswer, NULL);
         if(!_gstwebrtcanswer)
         {
            Glib::Error __error;
            reply.get_field<Glib::Error>("error", __error);
            ErrLog(<<"failed to get SDP answer: " << __error.what());
            gst_promise_unref (_promise);
            sdpReady(false, nullptr);
            return;
         }
         gst_promise_unref (_promise);

         setLocalDescription(_gstwebrtcanswer);

         std::unique_ptr<SdpContents> _answer(createSdpContentsFromGstreamer(_gstwebrtcanswer));
         gst_webrtc_session_description_free (_gstwebrtcanswer);
         //_answer->session().transformLocalHold(isHolding());  // FIXME - Gstreamer can hold?
         setLocalSdpGathering(*_answer);

         /* FIXME
                  setLocalSdp(*_answer);
                  setRemoteSdp(*offerMangled);
                  sdpReady(true, std::move(_answer)); */
      };

      std::function<void()> cConnected = [this, isWebRTC, cAnswerCreated, gstwebrtcoffer, offerMangled, sdpReady]() {
         if(isWebRTC)
         {
            // FIXME - Gst::Promise
            DebugLog(<<"submitting SDP offer to Gstreamer element");
            GstPromise *_promise = nullptr;
            /* Set remote description on our pipeline */
            setRemoteDescription(gstwebrtcoffer.get());

            std::function<void(GstPromise * _promise, gpointer user_data)> *_cAnswerCreated =
                     new std::function<void(GstPromise * _promise, gpointer user_data)>(cAnswerCreated);
            _promise = gst_promise_new_with_change_func (promise_stub, _cAnswerCreated,
                     NULL);
            g_signal_emit_by_name (mRtpTransportElement->gobj(), "create-answer", NULL, _promise);
         }
         else
         {
            //cAnswerCreated
            std::unique_ptr<SdpContents> answer(new SdpContents(*mRtpSession->getLocalSdp()));
            setLocalSdp(*answer);
            setRemoteSdp(*offerMangled);
            mGstMediaStackAdapter.runMainLoop();
            sdpReady(true, std::move(answer));
         }
      };



      DebugLog(<<"request SDP answer from Gstreamer");



      CallbackSdpReady _sdpReady = [this, offerMangled, sdpReady](bool sdpOk, std::unique_ptr<resip::SdpContents> sdp)
      {
         if(!sdpOk)
         {
            sdpReady(false, std::move(sdp));
            return;
         }
         setLocalSdp(*sdp);
         setRemoteSdp(*offerMangled);
         sdpReady(true, std::move(sdp));
      };

      if(firstUseEndpoint)
      {
         createAndConnectElements(isWebRTC, cConnected, _sdpReady);
      }
      else
      {
         cConnected();
      }

      /*bool firstUseEndpoint = initEndpointIfRequired(isWebRTC);

      gstreamer::ContinuationString cOnAnswerReady = [this, offerMangled, isWebRTC, sdpReady](const std::string& answer){
         StackLog(<<"answer FROM Gst: " << answer);
         HeaderFieldValue hfv(answer.data(), answer.size());
         Mime type("application", "sdp");
         std::unique_ptr<SdpContents> _answer(new SdpContents(hfv, type));
         _answer->session().transformLocalHold(isHolding());
         setLocalSdp(*_answer);
         setRemoteSdp(*offerMangled);
         sdpReady(true, std::move(_answer));
      };

      gstreamer::ContinuationVoid cConnected = [this, offerMangled, offerMangledStr, isWebRTC, firstUseEndpoint, sdpReady, cOnAnswerReady]{
         if(!firstUseEndpoint && mReuseSdpAnswer)
         {
            // FIXME - Gst should handle hold/resume
            // but it fails with SDP_END_POINT_ALREADY_NEGOTIATED
            // if we call processOffer more than once
            std::ostringstream answerBuf;
            answerBuf << *getLocalSdp();
            std::shared_ptr<std::string> answerStr = std::make_shared<std::string>(answerBuf.str());
            cOnAnswerReady(*answerStr);
            return;
         }
         mEndpoint->processOffer([this, offerMangled, isWebRTC, sdpReady, cOnAnswerReady](const std::string& answer){
            if(isWebRTC)
            {
               if(mTrickleIcePermitted && offerMangled->session().isTrickleIceSupported())
               {
                  HeaderFieldValue hfv(answer.data(), answer.size());
                  Mime type("application", "sdp");
                  std::unique_ptr<SdpContents> _local(new SdpContents(hfv, type));
                  DebugLog(<<"storing incomplete webrtc answer");
                  setLocalSdp(*_local);
                  setRemoteSdp(*offerMangled);
                  ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(getInviteSessionHandle().get());
                  sis->provideAnswer(*_local);
                  sis->provisional(183, true);
                  //getDialogSet().provideAnswer(std::move(_local), getInviteSessionHandle(), false, true);
                  enableTrickleIce(); // now we are in early media phase, it is safe to send INFO

                  // FIXME - if we sent an SDP answer here,
                  //         make sure we don't call provideAnswer again on 200 OK
               }

               doIceGathering(cOnAnswerReady);
            }
            else
            {
               cOnAnswerReady(answer);
            }
         }, *offerMangledStr); // processOffer
      };

      if(firstUseEndpoint)
      {
         createAndConnectElements(cConnected);
      }
      else
      {
         cConnected();
      }*/

      requestSent = true;
   }
   catch(exception& e)
   {
      ErrLog(<<"something went wrong: " << e.what()); // FIXME - add try/catch to Continuation
      requestSent = false;
   }

   if(!requestSent)
   {
      sdpReady(false, nullptr);
   }
}

void
GstRemoteParticipant::adjustRTPStreams(bool sendingOffer)
{
   // FIXME Gst - implement, may need to break up this method into multiple parts
   StackLog(<<"adjustRTPStreams");

   std::shared_ptr<SdpContents> localSdp = sendingOffer ? getDialogSet().getProposedSdp() : getLocalSdp();
   resip_assert(localSdp);

   std::shared_ptr<SdpContents> remoteSdp = getRemoteSdp();
   bool remoteSdpChanged = remoteSdp.get() != mLastRemoteSdp; // FIXME - better way to do this?
   mLastRemoteSdp = remoteSdp.get();
   if(remoteSdp)
   {
      const SdpContents::Session::Direction& remoteDirection = remoteSdp->session().getDirection();
      if(!remoteDirection.recv())
      {
         setRemoteHold(true);
      }
      else
      {
         setRemoteHold(false);
      }
      if(remoteSdpChanged && mWaitingAnswer)
      {
         // FIXME - maybe this should not be in adjustRTPStreams
         DebugLog(<<"remoteSdp has changed, sending to Gstreamer");
         mWaitingAnswer = false;
         if(isWebRTCSession())
         {
            std::shared_ptr<GstWebRTCSessionDescription> gstwebrtcanswer =
                     createGstWebRTCSessionDescriptionFromSdpContents(GST_WEBRTC_SDP_TYPE_ANSWER, *remoteSdp);
            if(!gstwebrtcanswer)
            {
               ErrLog(<<"failed to convert offer to GstWebRTCSessionDescription");
               return;
            }
            mRtpSession->setRemoteSdp(remoteSdp);
            setRemoteDescription(gstwebrtcanswer.get());
         }
         else
         {
            mRtpSession->processAnswer(remoteSdp);
         }
         /*mEndpoint->processAnswer([this](const std::string updatedOffer){
            // FIXME - use updatedOffer
            WarningLog(<<"Gst has processed the peer's SDP answer");
            StackLog(<<"updatedOffer FROM Gst: " << updatedOffer);
            HeaderFieldValue hfv(updatedOffer.data(), updatedOffer.size());
            Mime type("application", "sdp");
            std::unique_ptr<SdpContents> _updatedOffer(new SdpContents(hfv, type));
            _updatedOffer->session().transformLocalHold(isHolding());
            setLocalSdp(*_updatedOffer);
            //c(true, std::move(_updatedOffer));
         }, answerBuf.str());*/
      }
      for(int i = 1000; i <= 5000; i+=1000)
      {
         std::chrono::milliseconds _i = std::chrono::milliseconds(i);
         std::chrono::milliseconds __i = std::chrono::milliseconds(i + 500);
         mConversationManager.requestKeyframe(mHandle, _i);
         mConversationManager.requestKeyframeFromPeer(mHandle, __i);
      }
   }
   debugGraph();
}

void
GstRemoteParticipant::addToConversation(Conversation *conversation, unsigned int inputGain, unsigned int outputGain)
{
   RemoteParticipant::addToConversation(conversation, inputGain, outputGain);
}

void
GstRemoteParticipant::removeFromConversation(Conversation *conversation)
{
   RemoteParticipant::removeFromConversation(conversation);
}

bool
GstRemoteParticipant::mediaStackPortAvailable()
{
   return true; // FIXME Gst - can we check with Gst somehow?
}

void
GstRemoteParticipant::waitingMode()
{
/*   std::shared_ptr<gstreamer::MediaElement> e = getWaitingModeElement();
   if(!e)
   {
      return;
   }
   e->connect([this]{
      DebugLog(<<"connected in waiting mode, waiting for peer");
      if(mWaitingModeVideo)
      {
         mPlayer->play([this]{}); // FIXME Gst async
      }
      else
      {
         mEndpoint->connect([this]{
            requestKeyframeFromPeer();
         }, *mPassThrough); // FIXME Gst async
      }
      requestKeyframeFromPeer();
   }, *mEndpoint);*/
}

/*std::shared_ptr<gstreamer::MediaElement>
GstRemoteParticipant::getWaitingModeElement()
{
   if(mWaitingModeVideo)
   {
      return dynamic_pointer_cast<gstreamer::Endpoint>(mPlayer);
   }
   else
   {
      return mPassThrough;
   }
}*/

bool
GstRemoteParticipant::onMediaControlEvent(MediaControlContents::MediaControl& mediaControl)
{
   if(mWSAcceptsKeyframeRequests)
   {
      auto now = std::chrono::steady_clock::now();
      if(now < (mLastLocalKeyframeRequest + getKeyframeRequestInterval()))
      {
         DebugLog(<<"keyframe request ignored, too soon");
         return false;
      }
      mLastLocalKeyframeRequest = now;
      InfoLog(<<"onMediaControlEvent: sending to Gst");
      // FIXME - check the content of the event
      //mEndpoint->sendPictureFastUpdate([this](){}); // FIXME Gst async, do we need to wait for Gst here?
      return true;
   }
   else
   {
      WarningLog(<<"rejecting MediaControlEvent due to config option mWSAcceptsKeyframeRequests");
      return false;
   }
}

void
GstRemoteParticipant::onRemoteIceCandidate(const resip::Data& candidate, unsigned int lineIndex, const resip::Data& mid)
{
   g_signal_emit_by_name (mRtpTransportElement->gobj(), "add-ice-candidate", lineIndex,
             candidate.c_str());
}

bool
GstRemoteParticipant::onTrickleIce(resip::TrickleIceContents& trickleIce)
{
   DebugLog(<<"onTrickleIce: sending to Gst");
   // FIXME - did we already receive a suitable SDP for trickle ICE and send it to Gstreamer?
   //         if not, Gstreamer is not ready for the candidates
   // FIXME - do we need to validate the ice-pwd password attribute here?
   for(auto m = trickleIce.media().cbegin(); m != trickleIce.media().cend(); m++)
   {
      if(m->exists("mid"))
      {
         const Data& mid = m->getValues("mid").front();
         unsigned int mLineIndex = mid.convertInt(); // FIXME - calculate from the full SDP
         if(m->exists("candidate"))
         {
            for(const auto& c : m->getValues("candidate"))
            {
               onRemoteIceCandidate(c, mLineIndex, mid);
            }
         }
      }
      else
      {
         WarningLog(<<"mid is missing for Medium in SDP fragment: " << trickleIce);
         return false;
      }
   }
   return true;
}

void
GstRemoteParticipant::requestKeyframe()
{
   DebugLog(<<"requestKeyframe from local encoder");

   Gst::Structure gSTForceKeyUnit("GstForceKeyUnit",
      "all-headers", true);

   Gst::Iterator<Gst::Pad> it = mRtpTransportElement->iterate_sink_pads();
   while(it.next())
   {
      RefPtr<Pad> pad = *it;
      if(pad)
      {
         StackLog(<<"examining pad " << pad->get_name());
         RefPtr<Pad> peer = pad->get_peer();
         RefPtr<Caps> caps = pad->get_current_caps();
         if(peer && caps)
         {
            if(caps->get_structure(0).has_field("media"))
            {
               string mediaName;
               caps->get_structure(0).get_field("media", mediaName);
               StackLog(<<"mediaName: " << mediaName);
               if(mediaName == "video")
               {
                  RefPtr<Gst::Event> _e = Glib::wrap(gst_event_new_custom(GST_EVENT_CUSTOM_UPSTREAM,
                           gSTForceKeyUnit.gobj_copy()));
                  peer->send_event(_e);
               }
            }
         }
         else
         {
            DebugLog(<<"no peer or no caps for this pad");
         }
      }
      else
      {
         WarningLog(<<"invalid pad");
      }
   }
}

/* ====================================================================

 Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2022, Daniel Pocock https://danielpocock.com
 Copyright (c) 2021, SIP Spectrum, Inc.
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
