#include "resip/stack/SdpContents.hxx"
#include "resip/stack/PlainContents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/ShutdownMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/HEPSipMessageLoggingHandler.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/ClientInviteSession.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/DumShutdownHandler.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/ServerInviteSession.hxx"
#include "resip/dum/ServerOutOfDialogReq.hxx"
#include "resip/dum/OutOfDialogHandler.hxx"
#include "resip/dum/AppDialog.hxx"
#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/AppDialogSetFactory.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "rutil/GStreamerUtils.hxx"
#include "rutil/ServerProcess.hxx"
#include "rutil/hep/HepAgent.hxx"

#include <sstream>
#include <time.h>
#include <utility>
#include <map>

#include <gstreamermm.h>
#include <glibmm/main.h>

#include "EchoTestConfig.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace echotest;
using namespace resip;
using namespace std;

Data myIP;

class CodecConfig
{
   public:
      CodecConfig(const Data& name,
                  const Data& decoder,
                  const Data& encoder,
                  const Data& h264Profile,
                  const Data& depay,
                  const Data& pay,
                  const Data& fmtp) :
         mName(name),
         mDecoder(decoder), mEncoder(encoder), mH264Profile(h264Profile),
         mDepay(depay), mPay(pay), mFmtp(fmtp) {}
      Data mName;
      Data mDecoder;
      Data mEncoder;
      Data mH264Profile;
      Data mDepay;
      Data mPay;
      Data mFmtp;
};

typedef std::map<Data,std::shared_ptr<CodecConfig>> Pipelines;
Pipelines mPipelines;

void
initPipelines()
{
   // Obtain a list of all H.264 plugins currently installed with
   // the command:
   //
   //   gst-inspect-1.0  | grep 264

   // OpenMAX IL https://www.phoronix.com/scan.php?page=news_item&px=Libav-OMX-H264-MPEG4
   // https://www.khronos.org/openmaxil
   // VA-API is more advanced
   mPipelines.insert({"h264omx", make_shared<CodecConfig>("H264", "avdec_h264", "avenc_h264_omx", "baseline", "rtph264depay", "rtph264pay", // not working
      "packetization-mode=0;profile-level-id=420016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16")});

   // Decode: libav
   // Encode: x264 (GPL 2) https://www.videolan.org/developers/x264.html
   //    (x264 doesn't provide a decoder)
   mPipelines.insert({"h264avx", make_shared<CodecConfig>("H264", "avdec_h264", "x264enc", "baseline", "rtph264depay", "rtph264pay", // works
      "packetization-mode=0;profile-level-id=420016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16")});

   // need to recompile the package gstreamer1.0-plugins-bad selecting the openh264 config option
   mPipelines.insert({"h264o", make_shared<CodecConfig>("H264", "openh264dec", "openh264enc", "baseline", "rtph264depay", "rtph264pay", // works for 30 seconds
      "packetization-mode=0;profile-level-id=420016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16")});

   // VA-API for AMD and Intel GPU hardware decoding / encoding
   // improves quality while using less CPU and giving real-time performance
   // sudo apt install gstreamer1.0-vaapi
   mPipelines.insert({"h264v", make_shared<CodecConfig>("H264", "vaapih264dec", "vaapih264enc", "constrained-baseline", "rtph264depay", "rtph264pay", // garbled video stream
      "packetization-mode=0;profile-level-id=420016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16")});

   // mix and match decoder and encoder
   //mPipelines.insert({"h264m", make_shared<CodecConfig>("H264", "vaapih264dec", "x264enc", "baseline", "rtph264depay", "rtph264pay", // works for a few seconds then replays
   //mPipelines.insert({"h264m", make_shared<CodecConfig>("H264", "openh264dec", "x264enc", "baseline", "rtph264depay", "rtph264pay", // not working
   mPipelines.insert({"h264m", make_shared<CodecConfig>("H264", "avdec_h264", "openh264enc", "baseline", "rtph264depay", "rtph264pay", // works
   //mPipelines.insert({"h264m", make_shared<CodecConfig>("H264", "vaapih264dec", "openh264enc", "baseline", "rtph264depay", "rtph264pay", // works
   //mPipelines.insert({"h264m", make_shared<CodecConfig>("H264", "avdec_h264", "vaapih264enc", "constrained-baseline", "rtph264depay", "rtph264pay", // not working
      "packetization-mode=0;profile-level-id=420016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16")});

   // VP8 with the open source reference implementation
   mPipelines.insert({"vp8", make_shared<CodecConfig>("VP8", "vp8dec", "vp8enc", "", "rtpvp8depay", "rtpvp8pay", "profile-level-id=HiP")});
}

/////////////////////////////////////////////////////////////////////////////////
//
// Use gstreamermm C++ bindings for GStreamer to create the GStreamer
// pipeline.
//
/////////////////////////////////////////////////////////////////////////////////
using namespace Gst;
using Glib::RefPtr;

class GstThread : public ThreadIf
{
   private:

      RefPtr<Pipeline> pipeline;

      RefPtr<Element> a_rtppcmadepay, v_depay;
      RefPtr<Element> v_queue;

      RefPtr<Glib::MainLoop> main_loop;

      bool on_bus_message(const RefPtr<Gst::Bus>&, const RefPtr<Gst::Message>& message)
      {
         switch(message->get_message_type())
         {
            case Gst::MESSAGE_EOS:
               DebugLog(<< "End of stream");
               main_loop->quit();
               return false;
            case Gst::MESSAGE_ERROR:
               {
                  Glib::Error error;
                  std::string debug_message;
                  RefPtr<MessageError>::cast_static(message)->parse(error, debug_message);
                  ErrLog(<< "Error: " << error.what() << std::endl << debug_message);
                  main_loop->quit();
                  return false;
               }
            default:
               break;
         }

         return true;
      }

      void on_demux_pad_added(const RefPtr<Pad>& newPad)
      {
         DebugLog(<<"Dynamic pad created. Linking demuxer/decoder " << newPad->get_name() );
         RefPtr<Pad> sinkPad;
         if(newPad->get_name().find("recv_rtp_src_0_") == 0)
         {
            DebugLog(<<"audio pad");
            sinkPad = a_rtppcmadepay->get_static_pad("sink");
         }
         else if(newPad->get_name().find("recv_rtp_src_1_") == 0)
         {
            DebugLog(<<"video pad");
            sinkPad = v_depay->get_static_pad("sink");
            //sinkPad = v_queue->get_static_pad("sink");
         }
         else
         {
            DebugLog(<<"pad not handled");
            return;
         }
         DebugLog(<<"found sinkPad");
         PadLinkReturn ret = newPad->link(sinkPad);

         if (ret != PAD_LINK_OK && ret != PAD_LINK_WAS_LINKED)
         {
            DebugLog(<< "Linking of pads " << newPad->get_name() << " and " <<
            sinkPad->get_name() << " failed.");
         }
         DebugLog(<<"linking done");
         GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline->gobj()),
                                   GST_DEBUG_GRAPH_SHOW_ALL,
                                   "test-pipeline");
      }

   public:

      GstThread(shared_ptr<CodecConfig> codecConfig,
                const Data& peerAddress,
                int localAudio, int peerAudio,
                int localVideo, int peerVideo)
      {

         pipeline = Pipeline::create("test-pipeline");
         main_loop = Glib::MainLoop::create();
         RefPtr<Bus> bus = pipeline->get_bus();
         bus->add_watch(sigc::mem_fun(*this, &GstThread::on_bus_message));

         DebugLog(<<"Creating elements");

         RefPtr<Element> audio_source = ElementFactory::create_element("udpsrc"),
            audio_source_rtcp = ElementFactory::create_element("udpsrc"),
            video_source = ElementFactory::create_element("udpsrc"),
            video_source_rtcp = ElementFactory::create_element("udpsrc"),
            audio_sink = ElementFactory::create_element("multiudpsink"),
            audio_sink_rtcp = ElementFactory::create_element("multiudpsink"),
            video_sink = ElementFactory::create_element("multiudpsink"),
            video_sink_rtcp = ElementFactory::create_element("multiudpsink");

         RefPtr<Element> rtpbin = ElementFactory::create_element("rtpbin");
         a_rtppcmadepay = ElementFactory::create_element("rtppcmadepay");
         v_depay = ElementFactory::create_element(codecConfig->mDepay.c_str());
         RefPtr<Element> v_parse;
         if(codecConfig->mName == "H264")
         {
            v_parse = ElementFactory::create_element("h264parse");
         }
         else if(codecConfig->mName == "VP8")
         {
            v_parse = ElementFactory::create_element("vp8parse"); // from gst-kurento-plugins
         }
         else
         {
            ErrLog(<<"v_parse: unsupported video codec " << codecConfig->mName);
            throw;
         }
         RefPtr<Element> a_queue = ElementFactory::create_element("queue");
         v_queue = ElementFactory::create_element("queue");
         RefPtr<Element> alawdec = ElementFactory::create_element("alawdec");
         RefPtr<Element> alawenc = ElementFactory::create_element("alawenc");
         DebugLog(<<"selected pipeline: decoder = " << codecConfig->mDecoder <<
                                      " encoder = " << codecConfig->mEncoder);
         RefPtr<Element> vdec = ElementFactory::create_element(codecConfig->mDecoder.c_str());
         RefPtr<Element> venc = ElementFactory::create_element(codecConfig->mEncoder.c_str());
         RefPtr<Element> a_rtppcmapay = ElementFactory::create_element("rtppcmapay");
         RefPtr<Element> v_pay = ElementFactory::create_element(codecConfig->mPay.c_str());

         if (!audio_source || !audio_source_rtcp || !video_source || !video_source_rtcp || !audio_sink || !audio_sink_rtcp || !video_sink || !video_sink_rtcp || !rtpbin || !a_rtppcmadepay || !v_depay || !a_queue || !v_queue || !a_rtppcmapay || !v_pay)
         {
            ErrLog(<< "One element could not be created.");
            throw;
         }

         DebugLog(<<"Creating caps");

         Glib::RefPtr<Gst::Caps> a_caps = Gst::Caps::create_simple(
            "application/x-rtp",
            "media", "audio",
            "clock-rate", 8000,
            "encoding-name", "PCMA",
            "payload", 8);

         Glib::RefPtr<Gst::Caps> v_caps = Gst::Caps::create_simple(
            "application/x-rtp",
            "media", "video",
            "clock-rate", 90000,
            "encoding-name", codecConfig->mName.c_str(),
            "payload", 97);

#define P_CLIENTS(a,p) (a + ":" + Data(p))

         Glib::RefPtr<Gst::Caps> rtcp_caps = Gst::Caps::create_simple(
            "application/x-rtcp");

         DebugLog(<<"setting properties");

         audio_source->set_property<Glib::ustring>("address", myIP.c_str());
         audio_source->set_property<gint32>("port", localAudio);

         audio_source_rtcp->set_property<Glib::ustring>("address", myIP.c_str());
         audio_source_rtcp->set_property<gint32>("port", localAudio+1);

         video_source->set_property<Glib::ustring>("address", myIP.c_str());
         video_source->set_property<gint32>("port", localVideo);

         video_source_rtcp->set_property<Glib::ustring>("address", myIP.c_str());
         video_source_rtcp->set_property<gint32>("port", localVideo+1);

         Data peerAudioClient = P_CLIENTS(peerAddress, peerAudio);
         audio_sink->set_property<Glib::ustring>("clients", peerAudioClient.c_str());
         DebugLog(<<"peerAudioClient = " << peerAudioClient);
         audio_sink->set_property("sync", false);
         audio_sink->set_property("async", false);

         Data peerAudioClientRtcp = P_CLIENTS(peerAddress, peerAudio+1);
         audio_sink_rtcp->set_property<Glib::ustring>("clients", peerAudioClientRtcp.c_str());
         audio_sink_rtcp->set_property("sync", false);
         audio_sink_rtcp->set_property("async", false);

         Data peerVideoClient = P_CLIENTS(peerAddress, peerVideo);
         video_sink->set_property<Glib::ustring>("clients", peerVideoClient.c_str());
         video_sink->set_property("sync", false);
         video_sink->set_property("async", false);

         Data peerVideoClientRtcp = P_CLIENTS(peerAddress, peerVideo+1);
         video_sink_rtcp->set_property<Glib::ustring>("clients", peerVideoClientRtcp.c_str());
         video_sink_rtcp->set_property("sync", false);
         video_sink_rtcp->set_property("async", false);

         v_pay->set_property<gint32>("pt", 97);

         // the names and values vary depending on which H.264 encoder is selected

         // openh264enc
         //venc->set_property("rate-control", 1); // bitrate
         //venc->set_property<guint32>("max-bitrate", 5000000);

         // vaapih264enc
         // venc->set_property<guint32>("bitrate", 5000);
         // venc->set_property<guint32>("quality-level", 7);

         DebugLog(<<"adding elements to pipeline");
  
         pipeline->add(audio_source)->
            add(audio_source_rtcp)->
            add(video_source)->
            add(video_source_rtcp)->
            add(audio_sink)->
            add(audio_sink_rtcp)->
            add(video_sink)->
            add(video_sink_rtcp)->
            add(rtpbin)->
            add(a_rtppcmadepay)->
            add(v_depay)->
            add(v_parse);
            pipeline->add(a_queue)->
            add(v_queue);
            pipeline->add(alawdec)->
            add(alawenc);
            pipeline->add(vdec);
            pipeline->add(venc);
            pipeline->add(a_rtppcmapay)->
            add(v_pay);

         DebugLog(<<"adding handlers");

         rtpbin->signal_pad_added().connect(sigc::mem_fun(*this, &GstThread::on_demux_pad_added));

         DebugLog(<<"linking pads, audio source");

         audio_source->link_pads("src", rtpbin, "recv_rtp_sink_0", a_caps);
         a_rtppcmadepay->link(alawdec);
         alawdec->link(a_queue);
         a_queue->link(alawenc);
         alawenc->link(a_rtppcmapay);
         a_rtppcmapay->link_pads("src", rtpbin, "send_rtp_sink_0");

         DebugLog(<<"linking pads, video source");

         video_source->link_pads("src", rtpbin, "recv_rtp_sink_1", v_caps);
         v_depay->link(v_parse);
         v_parse->link(vdec);
         vdec->link(v_queue);
         v_queue->link(venc);
         if(codecConfig->mH264Profile.empty())
         {
           venc->link(v_pay);
         }
         else
         {
           Glib::RefPtr<Gst::Caps> v_caps_h264 = Gst::Caps::create_simple("video/x-h264",
             "profile", codecConfig->mH264Profile.c_str());
           venc->link(v_pay, v_caps_h264);
         }
         //v_depay->link(v_queue);
         //v_queue->link(v_pay);
         v_pay->link_pads("src", rtpbin, "send_rtp_sink_1");
         //v_queue->link_pads("src", rtpbin, "send_rtp_sink_1");

         DebugLog(<<"linking pads, audio sink");

         rtpbin->link_pads("send_rtp_src_0", audio_sink, "sink");

         DebugLog(<<"linking pads, video sink");

         rtpbin->link_pads("send_rtp_src_1", video_sink, "sink");

         DebugLog(<<"Linking pads for RTCP");

         audio_source_rtcp->link_pads("src", rtpbin, "recv_rtcp_sink_0", rtcp_caps);
         rtpbin->link_pads("send_rtcp_src_0", audio_sink_rtcp, "sink", rtcp_caps);

         video_source_rtcp->link_pads("src", rtpbin, "recv_rtcp_sink_1", rtcp_caps);
         rtpbin->link_pads("send_rtcp_src_1", video_sink_rtcp, "sink", rtcp_caps);

         DebugLog(<<"setting state to play");

         pipeline->set_state(STATE_PLAYING);
      }

      ~GstThread()
      {
         shutdown();
         join();
      }

      void thread()
      {
         DebugLog(<<"storing a DOT file");
         GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline->gobj()), GST_DEBUG_GRAPH_SHOW_ALL, "test-pipeline-pre");

         DebugLog(<<"running the Gst main loop");
         main_loop->run();

         DebugLog(<<"done, storing a DOT file");
         GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline->gobj()), GST_DEBUG_GRAPH_SHOW_ALL, "test-pipeline-post");
         pipeline->set_state(STATE_NULL);
      }

      void
      shutdown() override
      {
         ThreadIf::shutdown();
         main_loop->quit();
      }
};

/////////////////////////////////////////////////////////////////////////////////
//
// Classes that provide the mapping between Application Data and DUM 
// dialogs/dialogsets
//  										
// The DUM layer creates an AppDialog/AppDialogSet object for inbound/outbound
// SIP Request that results in Dialog creation.
//  										
/////////////////////////////////////////////////////////////////////////////////
class testAppDialog : public AppDialog
{
public:
   testAppDialog(HandleManager& ham, Data &SampleAppData) : AppDialog(ham), mSampleAppData(SampleAppData)
   {  
      InfoLog(<< mSampleAppData << ": testAppDialog: created.");
   }
   virtual ~testAppDialog() 
   { 
      InfoLog(<< mSampleAppData << ": testAppDialog: destroyed.");
   }
   Data mSampleAppData;
};

class testAppDialogSet : public AppDialogSet
{
public:
   testAppDialogSet(DialogUsageManager& dum, Data SampleAppData) : AppDialogSet(dum), mSampleAppData(SampleAppData)
   {  
      InfoLog(<< mSampleAppData << ": testAppDialogSet: created.");
   }
   virtual ~testAppDialogSet() 
   {  
      InfoLog(<< mSampleAppData << ": testAppDialogSet: destroyed.");
   }
   virtual AppDialog* createAppDialog(const SipMessage& msg) 
   {  
      return new testAppDialog(mDum, mSampleAppData);  
   }
   virtual std::shared_ptr<UserProfile> selectUASUserProfile(const SipMessage& msg)
   { 
      InfoLog(<< mSampleAppData << ": testAppDialogSet: UAS UserProfile requested for msg: " << msg.brief());
      return mDum.getMasterUserProfile(); 
   }
   Data mSampleAppData;
};

class testAppDialogSetFactory : public AppDialogSetFactory
{
public:
   virtual AppDialogSet* createAppDialogSet(DialogUsageManager& dum, const SipMessage& msg) 
   {  return new testAppDialogSet(dum, Data("UAS") + Data("(") + getMethodName(msg.header(h_RequestLine).getMethod()) + Data(")"));  }
   // For a UAS the testAppDialogSet will be created by DUM using this function.  If you want to set 
   // Application Data, then one approach is to wait for onNewSession(ServerInviteSessionHandle ...) 
   // to be called, then use the ServerInviteSessionHandle to get at the AppDialogSet or AppDialog,
   // then cast to your derived class and set the desired application data.
};


// Generic InviteSessionHandler
class TestInviteSessionHandler : public InviteSessionHandler, public ClientRegistrationHandler, public OutOfDialogHandler
{
   public:
      Data name;
      bool registered;
      ClientRegistrationHandle registerHandle;
      
      TestInviteSessionHandler(const Data& n) : name(n), registered(false) 
      {
      }

      virtual ~TestInviteSessionHandler()
      {
      }
      
      virtual void onSuccess(ClientRegistrationHandle h, const SipMessage& response)
      {         
         registerHandle = h;   
         assert(registerHandle.isValid());         
         InfoLog(<< name << ": ClientRegistration-onSuccess - " << response.brief());
         registered = true;
      }

      virtual void onFailure(ClientRegistrationHandle, const SipMessage& msg)
      {
         InfoLog(<< name << ": ClientRegistration-onFailure - " << msg.brief());
         throw;  // Ungracefully end
      }

      virtual void onRemoved(ClientRegistrationHandle, const SipMessage& response)
      {
          InfoLog(<< name << ": ClientRegistration-onRemoved");
      }

      virtual int onRequestRetry(ClientRegistrationHandle, int retrySeconds, const SipMessage& response)
      {
          InfoLog(<< name << ": ClientRegistration-onRequestRetry (" << retrySeconds << ") - " << response.brief());
          return -1;
      }

      virtual void onNewSession(ClientInviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)
      {
         InfoLog(<< name << ": ClientInviteSession-onNewSession - " << msg.brief());
      }
      
      virtual void onNewSession(ServerInviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)
      {
         InfoLog(<< name << ": ServerInviteSession-onNewSession - " << msg.brief());
      }

      virtual void onFailure(ClientInviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(<< name << ": ClientInviteSession-onFailure - " << msg.brief());
      }
      
      virtual void onProvisional(ClientInviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(<< name << ": ClientInviteSession-onProvisional - " << msg.brief());
      }

      virtual void onConnected(ClientInviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(<< name << ": ClientInviteSession-onConnected - " << msg.brief());
      }

      virtual void onStaleCallTimeout(ClientInviteSessionHandle handle)
      {
         InfoLog(<< name << ": ClientInviteSession-onStaleCallTimeout");
      }

      virtual void onConnected(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onConnected - " << msg.brief());
      }

      virtual void onRedirected(ClientInviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(<< name << ": ClientInviteSession-onRedirected - " << msg.brief());
      }

      virtual void onTerminated(InviteSessionHandle, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg)
      {
         InfoLog(<< name << ": InviteSession-onTerminated - " << msg->brief());
         assert(0); // This is overriden in UAS and UAC specific handlers
      }

      virtual void onAnswer(InviteSessionHandle, const SipMessage& msg, const SdpContents& sdp)
      {
         InfoLog(<< name << ": InviteSession-onAnswer(SDP)");
         //sdp->encode(cout);
      }

      virtual void onOffer(InviteSessionHandle is, const SipMessage& msg, const SdpContents& sdp)      
      {
         InfoLog(<< name << ": InviteSession-onOffer(SDP)");
         //sdp->encode(cout);
      }

      virtual void onEarlyMedia(ClientInviteSessionHandle, const SipMessage& msg, const SdpContents& sdp)
      {
         InfoLog(<< name << ": InviteSession-onEarlyMedia(SDP)");
         //sdp->encode(cout);
      }

      virtual void onOfferRequired(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onOfferRequired - " << msg.brief());
      }

      virtual void onOfferRejected(InviteSessionHandle, const SipMessage* msg)
      {
         InfoLog(<< name << ": InviteSession-onOfferRejected");
      }

      virtual void onRefer(InviteSessionHandle, ServerSubscriptionHandle, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onRefer - " << msg.brief());
      }

      virtual void onReferAccepted(InviteSessionHandle, ClientSubscriptionHandle, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onReferAccepted - " << msg.brief());
      }

      virtual void onReferRejected(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onReferRejected - " << msg.brief());
      }

      virtual void onReferNoSub(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onReferNoSub - " << msg.brief());
      }

      virtual void onInfo(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onInfo - " << msg.brief());
      }

      virtual void onInfoSuccess(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onInfoSuccess - " << msg.brief());
      }

      virtual void onInfoFailure(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onInfoFailure - " << msg.brief());
      }

      virtual void onMessage(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onMessage - " << msg.brief());
      }

      virtual void onMessageSuccess(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onMessageSuccess - " << msg.brief());
      }

      virtual void onMessageFailure(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onMessageFailure - " << msg.brief());
      }

      virtual void onForkDestroyed(ClientInviteSessionHandle)
	  {
         InfoLog(<< name << ": ClientInviteSession-onForkDestroyed");
	  }

      // Out-of-Dialog Callbacks
      virtual void onSuccess(ClientOutOfDialogReqHandle, const SipMessage& successResponse)
      {
          InfoLog(<< name << ": ClientOutOfDialogReq-onSuccess - " << successResponse.brief());
      }
      virtual void onFailure(ClientOutOfDialogReqHandle, const SipMessage& errorResponse)
      {
          InfoLog(<< name << ": ClientOutOfDialogReq-onFailure - " << errorResponse.brief());
      }
      virtual void onReceivedRequest(ServerOutOfDialogReqHandle ood, const SipMessage& request)
      {
          InfoLog(<< name << ": ServerOutOfDialogReq-onReceivedRequest - " << request.brief());
          // Add SDP to response here if required
          InfoLog(<< name << ": Sending 200 response to OPTIONS.");
          ood->send(ood->answerOptions());
      }
};

class TestUas : public TestInviteSessionHandler
{

   public:
      bool done;
      bool requestedOffer;

      SdpContents* mSdp;
      HeaderFieldValue* hfv;
      Data* txt;      

      shared_ptr<CodecConfig> mDefaultCodecConfig;

      int mAudioPort;
      int mVideoPort;

      typedef std::map<DialogId, shared_ptr<GstThread>> MediaThreads;
      MediaThreads mMediaThreads;

      TestUas(const Data& pipelineId, int audioPort, int videoPort)
         : TestInviteSessionHandler("UAS"), 
           done(false),
           requestedOffer(false),
           hfv(0),
           mDefaultCodecConfig(mPipelines[pipelineId]),
           mAudioPort(audioPort),
           mVideoPort(videoPort)
      { 
         txt = new Data("v=0\r\n"
                        "o=- 3838180699 3838180699 IN IP4 " + myIP + "\r\n"
                        "s=reSIProcate GStreamer Echo Test\r\n"
                        "c=IN IP4 " + myIP + "\r\n"
                        "t=0 0\r\n"
                        "a=ice-pwd:da9801364d7cd7d3a87f7f2f\r\n"
                        "a=ice-ufrag:91f7ed7e\r\n"
                        "a=rtcp-xr:rcvr-rtt=all:10000 stat-summary=loss,dup,jitt,TTL voip-metrics\r\n"
                        "m=audio " + Data(mAudioPort) + " RTP/AVP 8\r\n"
                        "a=sendrecv\r\n"
                        "a=rtcp:" + Data(mAudioPort+1) + "\r\n"
                        //"a=ssrc:2337389544 cname:user269660271@host-9999cdcf\r\n"
                        "m=video " + Data(mVideoPort) + " RTP/AVP 97\r\n"
                        "a=sendrecv\r\n"
                        "a=rtcp:" + Data(mVideoPort+1) + "\r\n"
                        "a=rtpmap:97 " + mDefaultCodecConfig->mName + "/90000\r\n"
                        "a=fmtp:97 " + mDefaultCodecConfig->mFmtp + "\r\n"
                        //"a=ssrc:2005192486 cname:user269660271@host-9999cdcf\r\n"
                        );
         
         hfv = new HeaderFieldValue(txt->data(), (unsigned int)txt->size());
         Mime type("application", "sdp");
         mSdp = new SdpContents(*hfv, type);
      }

      ~TestUas()
      {
         delete mSdp;
         delete txt;
         delete hfv;
      }

      shared_ptr<CodecConfig>
      updateSdp(const Data& pipelineId)
      {
         shared_ptr<CodecConfig> codecConfig(mDefaultCodecConfig);
         Pipelines::const_iterator it = mPipelines.find(pipelineId);
         if(it != mPipelines.end())
         {
            codecConfig = it->second;
         }

         txt = new Data("v=0\r\n"
                        "o=- 3838180699 3838180699 IN IP4 " + myIP + "\r\n"
                        "s=reSIProcate GStreamer Echo Test\r\n"
                        "c=IN IP4 " + myIP + "\r\n"
                        "t=0 0\r\n"
                        "a=ice-pwd:da9801364d7cd7d3a87f7f2f\r\n"
                        "a=ice-ufrag:91f7ed7e\r\n"
                        "a=rtcp-xr:rcvr-rtt=all:10000 stat-summary=loss,dup,jitt,TTL voip-metrics\r\n"
                        "m=audio " + Data(mAudioPort) + " RTP/AVP 8\r\n"
                        "a=sendrecv\r\n"
                        "a=rtcp:" + Data(mAudioPort+1) + "\r\n"
                        //"a=ssrc:2337389544 cname:user269660271@host-9999cdcf\r\n"
                        "m=video " + Data(mVideoPort) + " RTP/AVP 97\r\n"
                        "a=sendrecv\r\n"
                        "a=rtcp:" + Data(mVideoPort+1) + "\r\n"
                        "a=rtpmap:97 " + codecConfig->mName + "/90000\r\n"
                        "a=fmtp:97 " + codecConfig->mFmtp + "\r\n"
                        //"a=ssrc:2005192486 cname:user269660271@host-9999cdcf\r\n"
                        );

         hfv = new HeaderFieldValue(txt->data(), (unsigned int)txt->size());
         Mime type("application", "sdp");
         mSdp = new SdpContents(*hfv, type);
         return codecConfig;
      }

      using TestInviteSessionHandler::onNewSession;
      virtual void 
      onNewSession(ServerInviteSessionHandle sis, InviteSession::OfferAnswerType oat, const SipMessage& msg)
      {
         InfoLog(<< name << ": ServerInviteSession-onNewSession - " << msg.brief());
         InfoLog(<< name << ": Sending 180 response.");
         mSis = sis;         
         sis->provisional(180);
      }

      virtual void onTerminated(InviteSessionHandle is, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg)
      {
         InfoLog(<< name << ": InviteSession-onTerminated - ");
         MediaThreads::const_iterator it = mMediaThreads.find(is->getDialogId());
         if(it != mMediaThreads.end())
         {
            shared_ptr<GstThread> t = it->second;
            t->shutdown();
         }
         done = true;
      }

      virtual void onOffer(InviteSessionHandle is, const SipMessage& msg, const SdpContents& sdp)      
      {
         InfoLog(<< name << ": InviteSession-onOffer(SDP)");
         const Data& callee = msg.header(h_RequestLine).uri().user();
         InfoLog(<< name << ": call is for: " << callee);
         shared_ptr<CodecConfig> codecConfig = updateSdp(callee);
         //sdp->encode(cout);
         InfoLog(<< name << ": starting GStreamer media thread");
         const Data& peerIp = sdp.session().connection().getAddress();
         const int& peerAudio = sdp.session().media().begin()->port();
         const int& peerVideo = (++sdp.session().media().begin())->port();
         // FIXME shared_ptr
         shared_ptr<GstThread> t = make_shared<GstThread>(codecConfig, peerIp, mAudioPort, peerAudio, mVideoPort, peerVideo);
         mMediaThreads[is->getDialogId()] = t;
         t->run();
         InfoLog(<< name << ": Sending 200 response with SDP answer.");
         is->provideAnswer(*mSdp);
         mSis->accept();
      }

      virtual void onOfferRequired(InviteSessionHandle is, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onOfferRequired - " << msg.brief());
         is->provideOffer(*mSdp);
      }

      virtual void onConnected(InviteSessionHandle is, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onConnected - " << msg.brief());
         
         // At this point no NIT should have been sent
         assert(!is->getLastSentNITRequest());
      }

      virtual void onInfoSuccess(InviteSessionHandle is, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onInfoSuccess - " << msg.brief());
      }

      virtual void onMessage(InviteSessionHandle is, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onMessage - " << msg.brief());

         is->acceptNIT();
      }

      // Normal people wouldn't put this functionality in the handler
      // it's a kludge for this test for right now
      void hangup()
      {
         if (mSis.isValid())
         {
            InfoLog(<< name << ": Sending BYE.");
            mSis->end();
            done = true;
         }
      }
   private:
      ServerInviteSessionHandle mSis;      
};

class TestShutdownHandler : public DumShutdownHandler
{
   public:
      TestShutdownHandler(const Data& n) : name(n), dumShutDown(false) { }
      virtual ~TestShutdownHandler(){}

      Data name;
      bool dumShutDown;
      virtual void onDumCanBeDeleted() 
      {
         InfoLog(<< name << ": onDumCanBeDeleted.");
         dumShutDown = true;
      }
};

class EchoTestServer : public resip::ServerProcess
{
   private:
      SipStack* stackUas;
      DialogUsageManager* dumUas;

   public:
      EchoTestServer()
      {
      };

      ~EchoTestServer()
      {
         delete dumUas;
         delete stackUas;
      };

      int
      main (int argc, char** argv)
      {
         installSignalHandler();

         Data defaultConfigFilename("echoTest.config");
         EchoTestConfig echoTestConfig;
         try
         {
            echoTestConfig.parseConfig(argc, argv, defaultConfigFilename);
         }
         catch(std::exception& e)
         {
            ErrLog(<< "Exception parsing configuration: " << e.what());
            return -1;
         }

         Log::initialize(echoTestConfig, argv[0]);

         Data captureHost = echoTestConfig.getConfigData("CaptureHost", "");
         int capturePort = echoTestConfig.getConfigInt("CapturePort", 9060);
         int captureAgentID = echoTestConfig.getConfigInt("CaptureAgentID", 2002);

         // For GStreamer
         const char* _argv[] =  { argv[0], "--gst-debug", "5", NULL };
         int _argc = 3;
         char** __argv = (char**)_argv;
         // FIXME: are there C++ equivalents of these functions?
         gst_debug_remove_log_function (gst_debug_log_default);
         gst_debug_add_log_function(gst2resip_log_function, nullptr, nullptr);
         Gst::init(_argc, __argv);

         initPipelines();

#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
         FindMemoryLeaks fml;
         {
#endif
         bool doReg = echoTestConfig.getConfigBool("Register", false);
         NameAddr uasAor;
         Uri uasContact;
         Data uasPasswd;
         int uasUdpPort = echoTestConfig.getConfigInt("UDPPort", 12010);
         int uasTcpPort = echoTestConfig.getConfigInt("TCPPort", 12010);
         Uri outboundUri = echoTestConfig.getConfigUri("OutboundProxyUri", Uri());
         bool useOutbound = (outboundUri != Uri());
         myIP = echoTestConfig.getConfigData("IPAddress", "127.0.0.1");

         uasAor = echoTestConfig.getConfigNameAddr("SIPUri", NameAddr("sip:UAS@" + myIP + ":" + Data(uasUdpPort)));
         uasContact = Uri("sip:" + myIP + ":" + Data(uasUdpPort));
         uasPasswd = echoTestConfig.getConfigData("Password", Data::Empty);

         //set up UAS
         stackUas = new SipStack();
         dumUas = new DialogUsageManager(*stackUas);
         stackUas->addTransport(UDP, uasUdpPort);
         stackUas->addTransport(TCP, uasTcpPort);
         
         auto uasMasterProfile = std::make_shared<MasterProfile>();
         std::unique_ptr<ClientAuthManager> uasAuth(new ClientAuthManager);
         dumUas->setMasterProfile(uasMasterProfile);
         dumUas->setClientAuthManager(std::move(uasAuth));
         uasMasterProfile->setOverrideHostAndPort(uasContact);

         if(doReg) 
         {
            dumUas->getMasterProfile()->setDigestCredential(uasAor.uri().host(), uasAor.uri().user(), uasPasswd);
         }
         if(useOutbound) 
         {
            dumUas->getMasterProfile()->setOutboundProxy(outboundUri);
            dumUas->getMasterProfile()->addSupportedOptionTag(Token(Symbols::Outbound));
         }

         dumUas->getMasterProfile()->setDefaultFrom(uasAor);
         dumUas->getMasterProfile()->addSupportedMethod(INFO);
         dumUas->getMasterProfile()->addSupportedMethod(MESSAGE);
         dumUas->getMasterProfile()->addSupportedMimeType(INFO, PlainContents::getStaticType());
         dumUas->getMasterProfile()->addSupportedMimeType(MESSAGE, PlainContents::getStaticType());
         dumUas->getMasterProfile()->setDefaultRegistrationTime(70);

         Data pipelineId = echoTestConfig.getConfigData("DefaultPipelineId", "h264avx");
         int mediaPortStart = echoTestConfig.getConfigInt("MediaPortStart", 8002);
         TestUas uas(pipelineId, mediaPortStart, mediaPortStart+2);
         dumUas->setClientRegistrationHandler(&uas);
         dumUas->setInviteSessionHandler(&uas);
         dumUas->addOutOfDialogHandler(OPTIONS, &uas);

         std::unique_ptr<AppDialogSetFactory> uas_dsf(new testAppDialogSetFactory);
         dumUas->setAppDialogSetFactory(std::move(uas_dsf));

         if (!doReg) 
         {
            uas.registered = true;
         }

         if(!captureHost.empty())
         {
            const auto agent = std::make_shared<HepAgent>(captureHost, capturePort, captureAgentID);
            stackUas->setTransportSipMessageLoggingHandler(std::make_shared<HEPSipMessageLoggingHandler>(agent));
            // FIXME - need to integrate GStreamer RTCP code with HEP:
            // setRTCPEventLoggingHandler(std::make_shared<HEPRTCPEventLoggingHandler>(agent));
         }

         TestShutdownHandler uasShutdownHandler("UAS");   

         mainLoop();

         // FIXME: shutdown the DUM and SipStack

#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK)
         } // FindMemoryLeaks fml
#endif
         return 0;
      }

      void
      doWait()
      {
         stackUas->process(50);
      }

      void
      onLoop()
      {
         while(dumUas->process());
      }

      void
      onReload()
      {
      }

};

int main(int argc, char** argv)
{
   EchoTestServer proc;
   return proc.main(argc, argv);
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2021 Daniel Pocock, https://danielpocock.com
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more snformation on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
