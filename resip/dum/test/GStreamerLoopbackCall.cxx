#include "resip/stack/SdpContents.hxx"
#include "resip/stack/PlainContents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/ShutdownMessage.hxx"
#include "resip/stack/SipStack.hxx"
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

#include <sstream>
#include <time.h>
#include <utility>

#include <gstreamermm.h>
#include <glibmm/main.h>

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST
/* #define NO_REGISTRATION 1 -- This is now run-time option */

using namespace resip;
using namespace std;

Data myIP;

class CodecConfig
{
   public:
      CodecConfig(const Data& name, const Data& decoder, const Data& encoder, const Data& depay, const Data&pay) :
         mName(name), mDecoder(decoder), mEncoder(encoder), mDepay(depay), mPay(pay) {}
      Data mName;
      Data mDecoder;
      Data mEncoder;
      Data mDepay;
      Data mPay;
};

CodecConfig h264("H264", "avdec_h264", "avenc_h264_omx", "rtph264depay", "rtph264pay");
//CodecConfig h264x("H264", "x264dec", "x264enc", "rtph264depay", "rtph264pay");
CodecConfig h264x("H264", "avdec_h264", "x264enc", "rtph264depay", "rtph264pay");
CodecConfig vp8("VP8", "vp8dec", "vp8enc", "rtpvp8depay", "rtpvp8pay");

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

      //RefPtr<Element> a_rtpssrcdemux, v_rtpssrcdemux;
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
  GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline->gobj()), GST_DEBUG_GRAPH_SHOW_ALL, "test-pipeline");
}

   public:

      GstThread(const CodecConfig& codecConfig, const Data& peerAddress, int localAudio, int peerAudio, int localVideo, int peerVideo)
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
  RefPtr<Element> a_rtpssrcdemux = ElementFactory::create_element("rtpssrcdemux");
  RefPtr<Element> v_rtpssrcdemux = ElementFactory::create_element("rtpssrcdemux");
  a_rtppcmadepay = ElementFactory::create_element("rtppcmadepay");
  v_depay = ElementFactory::create_element(codecConfig.mDepay.c_str());
  RefPtr<Element> a_queue = ElementFactory::create_element("queue");
  v_queue = ElementFactory::create_element("queue");
  RefPtr<Element> alawdec = ElementFactory::create_element("alawdec");
  RefPtr<Element> alawenc = ElementFactory::create_element("alawenc");
  RefPtr<Element> vdec = ElementFactory::create_element(codecConfig.mDecoder.c_str());
  RefPtr<Element> venc = ElementFactory::create_element(codecConfig.mEncoder.c_str());
  RefPtr<Element> a_rtppcmapay = ElementFactory::create_element("rtppcmapay");
  RefPtr<Element> v_pay = ElementFactory::create_element(codecConfig.mPay.c_str());

  if (!audio_source || !audio_source_rtcp || !video_source || !video_source_rtcp || !audio_sink || !audio_sink_rtcp || !video_sink || !video_sink_rtcp || !rtpbin || !a_rtpssrcdemux || !v_rtpssrcdemux || !a_rtppcmadepay || !v_depay || !a_queue || !v_queue || !a_rtppcmapay || !v_pay)
  {
    ErrLog(<< "One element could not be created.");
    throw;
  }

  DebugLog(<<"Creating caps");

Glib::RefPtr<Gst::Caps> a_caps = Gst::Caps::create_simple("application/x-rtp",
  "media", "audio",
  "clock-rate", 8000,
  "encoding-name", "PCMA",
  "payload", 8);

Glib::RefPtr<Gst::Caps> v_caps = Gst::Caps::create_simple("application/x-rtp",
  "media", "video",
  "clock-rate", 90000,
  "encoding-name", codecConfig.mName.c_str(),
  "payload", 97);

#define P_CLIENTS(a,p) (a + ":" + Data(p))
Glib::RefPtr<Gst::Caps> rtcp_caps = Gst::Caps::create_simple("application/x-rtcp");

  DebugLog(<<"setting properties");

  audio_source->set_property<Glib::ustring>("address", myIP.c_str());
  audio_source->set_property<gint32>("port", localAudio);

  audio_source_rtcp->set_property<Glib::ustring>("address", myIP.c_str());
  audio_source_rtcp->set_property<gint32>("port", localAudio+1);

  video_source->set_property<Glib::ustring>("address", myIP.c_str());
  video_source->set_property<gint32>("port", localVideo);

  video_source_rtcp->set_property<Glib::ustring>("address", myIP.c_str());
  video_source_rtcp->set_property<gint32>("port", localVideo+1);

  //audio_sink->set_property<Glib::ustring>("host", peerAddress.c_str());
  //audio_sink->set_property<gint32>("port", peerAudio);
  Data peerAudioClient = P_CLIENTS(peerAddress, peerAudio);
  audio_sink->set_property<Glib::ustring>("clients", peerAudioClient.c_str());
  DebugLog(<<"peerAudioClient = " << peerAudioClient);
  audio_sink->set_property("sync", false);
  audio_sink->set_property("async", false);

  //audio_sink_rtcp->set_property<Glib::ustring>("host", peerAddress.c_str());
  //audio_sink_rtcp->set_property<gint32>("port", peerAudio+1);
  Data peerAudioClientRtcp = P_CLIENTS(peerAddress, peerAudio+1);
  audio_sink_rtcp->set_property<Glib::ustring>("clients", peerAudioClientRtcp.c_str());
  audio_sink_rtcp->set_property("sync", false);
  audio_sink_rtcp->set_property("async", false);

  //video_sink->set_property<Glib::ustring>("host", peerAddress.c_str());
  //video_sink->set_property<gint32>("port", peerVideo);
  Data peerVideoClient = P_CLIENTS(peerAddress, peerVideo);
  video_sink->set_property<Glib::ustring>("clients", peerVideoClient.c_str());
  video_sink->set_property("sync", false);
  video_sink->set_property("async", false);

  //video_sink_rtcp->set_property<Glib::ustring>("host", peerAddress.c_str());
  //video_sink_rtcp->set_property<gint32>("port", peerVideo+1);
  Data peerVideoClientRtcp = P_CLIENTS(peerAddress, peerVideo+1);
  video_sink_rtcp->set_property<Glib::ustring>("clients", peerVideoClientRtcp.c_str());
  video_sink_rtcp->set_property("sync", false);
  video_sink_rtcp->set_property("async", false);

  //v_pay->set_property<gint32>("payload", 97);
  v_pay->set_property<gint32>("pt", 97);

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
            add(a_rtpssrcdemux)->
            add(v_rtpssrcdemux)->
            add(a_rtppcmadepay)->
            add(v_depay);
            pipeline->add(a_queue)->
            add(v_queue);
            pipeline->add(alawdec)->
            add(alawenc);
            pipeline->add(vdec);
            pipeline->add(venc);
            pipeline->add(a_rtppcmapay)->
            add(v_pay);

  DebugLog(<<"adding handlers");

  //a_rtpssrcdemux->signal_pad_added().connect(sigc::mem_fun(*this, &GstThread::on_demux_pad_added_a));
  //v_rtpssrcdemux->signal_pad_added().connect(sigc::mem_fun(*this, &GstThread::on_demux_pad_added_v));

  rtpbin->signal_pad_added().connect(sigc::mem_fun(*this, &GstThread::on_demux_pad_added));
  //rtpbin->signal_pad_added().connect(sigc::mem_fun(*this, &GstThread::on_demux_pad_added_v));

  DebugLog(<<"linking pads, audio source");

  audio_source->link_pads("src", rtpbin, "recv_rtp_sink_0", a_caps);
  //rtpbin->link_pads("recv_rtp_src_0", a_rtpssrcdemux, "sink");
  a_rtppcmadepay->link(alawdec);
  alawdec->link(a_queue);
  a_queue->link(alawenc);
  alawenc->link(a_rtppcmapay);
  a_rtppcmapay->link_pads("src", rtpbin, "send_rtp_sink_0");

  DebugLog(<<"linking pads, video source");

  video_source->link_pads("src", rtpbin, "recv_rtp_sink_1", v_caps);
  //rtpbin->link_pads("recv_rtp_src_1", v_rtpssrcdemux, "sink");
  v_depay->link(vdec);
  vdec->link(v_queue);
  v_queue->link(venc);
  venc->link(v_pay);
  /*v_depay->link(v_queue);
  v_queue->link(v_pay);*/
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

      ~GstThread() { shutdown(); join(); }

      void thread()
      {

         DebugLog(<<"storing a DOT file");
         GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline->gobj()), GST_DEBUG_GRAPH_SHOW_ALL, "test-pipeline");

  DebugLog(<<"running the Gst main loop");
  main_loop->run();

  DebugLog(<<"done, storing a DOT file");
         GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline->gobj()), GST_DEBUG_GRAPH_SHOW_ALL, "test-pipeline");
  pipeline->set_state(STATE_NULL);
      }


};

static Log::Level
gst_debug_level_to_severity_level (GstDebugLevel level)
{
  switch (level) {
  case GST_LEVEL_ERROR:   return Log::Err;
  case GST_LEVEL_WARNING: return Log::Warning;
  case GST_LEVEL_FIXME:   return Log::Info;
  case GST_LEVEL_INFO:    return Log::Info;
  case GST_LEVEL_DEBUG:   return Log::Debug;
  case GST_LEVEL_LOG:     return Log::Stack;
  case GST_LEVEL_TRACE:   return Log::Stack;
  default:                return Log::None;
  }
}

static void
resip_log_function (GstDebugCategory *category, GstDebugLevel level,
                  const gchar *file,
                  const gchar *function, gint line, GObject *object,
                  GstDebugMessage *message, gpointer user_data) G_GNUC_NO_INSTRUMENT;

static void
resip_log_function (GstDebugCategory *category, GstDebugLevel level,
                  const gchar *file,
                  const gchar *function, gint line, GObject *object,
                  GstDebugMessage *message, gpointer user_data)
{
  if (level > gst_debug_category_get_threshold (category) ) {
    return;
  }

  Log::Level level_ = gst_debug_level_to_severity_level (level);

  if (level_ == Log::None) {
    return;
  }

   Subsystem& system_ = Subsystem::APP;
   do
   {
      if (genericLogCheckLevel(level_, system_))
      {
         resip::Log::Guard _resip_log_guard(level_, system_, file, line, function);
         _resip_log_guard.asStream() << "[" << category->name << "]: ";
         // FIXME - include the GObject *object with debug_object (object)
         _resip_log_guard.asStream() << gst_debug_message_get (message);
      }
   }
   while(false);
}


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

class TestUac : public TestInviteSessionHandler
{
   public:
      bool done;
      SdpContents* mSdp;     
      HeaderFieldValue* hfv;      
      Data* txt;
      int mNumExpectedMessages;

      TestUac() 
         : TestInviteSessionHandler("UAC"), 
           done(false),
           mSdp(0),
           hfv(0),
           txt(0),
           mNumExpectedMessages(2)
      {
         txt = new Data("v=0\r\n"
                        "o=1900 369696545 369696545 IN IP4 192.168.2.15\r\n"
                        "s=X-Lite\r\n"
                        "c=IN IP4 192.168.2.15\r\n"
                        "t=0 0\r\n"
                        "m=audio 8000 RTP/AVP 8 3 98 97 101\r\n"
                        "a=rtpmap:8 pcma/8000\r\n"
                        "a=rtpmap:3 gsm/8000\r\n"
                        "a=rtpmap:98 iLBC\r\n"
                        "a=rtpmap:97 speex/8000\r\n"
                        "a=rtpmap:101 telephone-event/8000\r\n"
                        "a=fmtp:101 0-15\r\n");
         
         hfv = new HeaderFieldValue(txt->data(), (unsigned int)txt->size());
         Mime type("application", "sdp");
         mSdp = new SdpContents(*hfv, type);
      }

      virtual ~TestUac()
      {
         assert(mNumExpectedMessages == 0);
         delete mSdp;
         delete txt;
         delete hfv;
      }

      virtual void onOffer(InviteSessionHandle is, const SipMessage& msg, const SdpContents& sdp)      
      {
         InfoLog(<< name << ": InviteSession-onOffer(SDP)");
         //sdp->encode(cout);
         is->provideAnswer(sdp);
      }

      using TestInviteSessionHandler::onConnected;
      virtual void onConnected(ClientInviteSessionHandle is, const SipMessage& msg)
      {
         InfoLog(<< name << ": ClientInviteSession-onConnected - " << msg.brief());
         InfoLog(<< "Connected now - requestingOffer from UAS");
         is->requestOffer();

         // At this point no NIT should have been sent
         assert(!is->getLastSentNITRequest());

         // Send a first MESSAGE from UAC with some contents (we use a fake PlainContents contents here for
         // simplicity)
         PlainContents contents("Hi there!!!");
         is->message(contents);

         // Immediately send another one, which will end up queued on the
         // InviteSession's NIT queue
         PlainContents contentsOther("Hi again!!!");
         is->message(contentsOther);
      }

      virtual void onMessageSuccess(InviteSessionHandle is, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onMessageSuccess - " << msg.brief());

         assert(is->getLastSentNITRequest());
         PlainContents* pContents = dynamic_cast<PlainContents*>(is->getLastSentNITRequest()->getContents());
         assert(pContents != NULL);

         if(mNumExpectedMessages == 2)
         {
            assert(pContents->text() == Data("Hi there!!!"));
            mNumExpectedMessages--;
         }
         else if(mNumExpectedMessages == 1)
         {
            assert(pContents->text() == Data("Hi again!!!"));
            mNumExpectedMessages--;
         }
      }

      virtual void onInfo(InviteSessionHandle is, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onInfo - " << msg.brief());
         is->acceptNIT();
      }

      virtual void onTerminated(InviteSessionHandle, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg)
      {
         if(msg)
         {
            InfoLog(<< name << ": InviteSession-onTerminated - " << msg->brief());
         }
         else
         {
            InfoLog(<< name << ": InviteSession-onTerminated");
         }
         done = true;
      }
};

class TestUas : public TestInviteSessionHandler
{

   public:
      bool done;
      bool requestedOffer;
      time_t* pHangupAt;

      SdpContents* mSdp;
      HeaderFieldValue* hfv;
      Data* txt;      

      int mNumExpectedInfos;

      CodecConfig mCodecConfig;

      TestUas(time_t* pH) 
         : TestInviteSessionHandler("UAS"), 
           done(false),
           requestedOffer(false),
           pHangupAt(pH),
           hfv(0),
           mNumExpectedInfos(2),
           mCodecConfig(h264x)
      { 
         txt = new Data("v=0\r\n"
                        "o=- 3838180699 3838180699 IN IP4 " + myIP + "\r\n"
                        "s=Kurento Media Server\r\n"
                        "c=IN IP4 " + myIP + "\r\n"
                        "t=0 0\r\n"
                        "a=ice-pwd:da9801364d7cd7d3a87f7f2f\r\n"
                        "a=ice-ufrag:91f7ed7e\r\n"
                        "a=rtcp-xr:rcvr-rtt=all:10000 stat-summary=loss,dup,jitt,TTL voip-metrics\r\n"
                        "m=audio 8002 RTP/AVP 8\r\n"
                        "a=sendrecv\r\n"
                        "a=rtcp:8003\r\n"
                        //"a=ssrc:2337389544 cname:user269660271@host-9999cdcf\r\n"
                        "m=video 8004 RTP/AVP 97\r\n"
                        "a=sendrecv\r\n"
                        "a=rtcp:8005\r\n"
                        "a=rtpmap:97 " + mCodecConfig.mName + "/90000\r\n"
                        //"a=fmtp:97 profile-level-id=HiP\r\n"
                        "a=fmtp:97 packetization-mode=0;profile-level-id=420016;max-br=5000;max-mbps=245000;max-fs=9000;max-smbps=245000;max-fps=6000;max-rcmd-nalu-size=3456000;sar-supported=16\r\n"
                        //"a=ssrc:2005192486 cname:user269660271@host-9999cdcf\r\n"
                        );
         
         hfv = new HeaderFieldValue(txt->data(), (unsigned int)txt->size());
         Mime type("application", "sdp");
         mSdp = new SdpContents(*hfv, type);
      }

      ~TestUas()
      {
         assert(mNumExpectedInfos == 0);
         delete mSdp;
         delete txt;
         delete hfv;
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

      virtual void onTerminated(InviteSessionHandle, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg)
      {
         InfoLog(<< name << ": InviteSession-onTerminated - ");
         done = true;
      }

      virtual void onOffer(InviteSessionHandle is, const SipMessage& msg, const SdpContents& sdp)      
      {
         InfoLog(<< name << ": InviteSession-onOffer(SDP)");
         //sdp->encode(cout);
         InfoLog(<< name << ": starting GStreamer media thread");
         const Data& peerIp = sdp.session().connection().getAddress();
         const int& peerAudio = sdp.session().media().begin()->port();
         const int& peerVideo = (++sdp.session().media().begin())->port();
         // FIXME shared_ptr
         GstThread* t = new GstThread(mCodecConfig, peerIp, 8002, peerAudio, 8004, peerVideo);
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

         // Send a first INFO from UAS with some contents (we use a fake PlainContents contents here for
         // simplicity)
         //PlainContents contents("Hello there!!!");
         //is->info(contents);

         // Immediately send another one, which will end up queued on the
         // InviteSession's NIT queue
         //PlainContents contentsOther("Hello again!!!");
         //is->info(contentsOther);
      }

      virtual void onInfoSuccess(InviteSessionHandle is, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onInfoSuccess - " << msg.brief());

         assert(is->getLastSentNITRequest());
         PlainContents* pContents = dynamic_cast<PlainContents*>(is->getLastSentNITRequest()->getContents());
         assert(pContents != NULL);

         if(mNumExpectedInfos == 2)
         {
            assert(pContents->text() == Data("Hello there!!!"));
            mNumExpectedInfos--;
         }
         else if(mNumExpectedInfos == 1)
         {
            assert(pContents->text() == Data("Hello again!!!"));
            mNumExpectedInfos--;

            // Schedule a BYE in 5 seconds
            if(*pHangupAt == 0)
            {
               *pHangupAt = time(NULL) + 5;
            }
         }
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


int 
main (int argc, char** argv)
{
   //Log::initialize(Log::Cout, resip::Log::Warning, argv[0]);
   //Log::initialize(Log::Cout, resip::Log::Debug, argv[0]);
   //Log::initialize(Log::Cout, resip::Log::Stack, argv[0]);
   //Log::initialize(Log::Cout, resip::Log::Debug, argv[0]);
   Log::initialize(Log::File, resip::Log::Stack, argv[0], "testing.log");

  // For GStreamer
  const char* _argv[] =  { argv[0], "--gst-debug", "5", NULL };
  int _argc = 3;
  char** __argv = (char**)_argv;
  //Gst::init(_argc, __argv);
  gst_debug_remove_log_function (gst_debug_log_default);
  gst_debug_add_log_function(resip_log_function, nullptr, nullptr);
  Gst::init(_argc, __argv);


#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   FindMemoryLeaks fml;
   {
#endif
   bool doReg = false;
   NameAddr uacAor;
   NameAddr uasAor;
   Uri uasContact;
   Data uacPasswd;
   Data uasPasswd;
   bool useOutbound = false;
   Uri outboundUri;

   if ( argc == 1 ) 
   {
      uacAor = NameAddr("sip:UAC@127.0.0.1:12005");
      uasAor = NameAddr("sip:UAS@127.0.0.1:12010");
      InfoLog(<< "Skipping registration (no arguments).");
   } 
   else if ( argc == 2 )
   {
      myIP = Data(argv[1]);
      uacAor = NameAddr("sip:UAC@" + myIP + ":12005");
      uasAor = NameAddr("sip:UAS@" + myIP + ":12010");
      uasContact = Uri("sip:" + myIP + ":12010");
      InfoLog(<< "Skipping registration (no arguments).");
   }
   else 
   {
      if ( argc < 5 ) 
      {
         cerr << "usage: " << argv[0] <<
                 " sip:user1 passwd1 sip:user2 passwd2 [outbound]" << endl;
         return 1;
      }
      doReg = true;
      uacAor = NameAddr(argv[1]);
      uacPasswd = Data(argv[2]);
      uasAor = NameAddr(argv[3]);
      uasPasswd = Data(argv[4]);
      if ( argc >= 6 ) 
      {
         useOutbound = true;
         outboundUri = Uri(Data(argv[5]));
      }
   }

   //set up UAC
   SipStack stackUac;
   DialogUsageManager* dumUac = new DialogUsageManager(stackUac);
   stackUac.addTransport(UDP, 12005);
   stackUac.addTransport(TCP, 12005);

   auto uacMasterProfile = std::make_shared<MasterProfile>();
   std::unique_ptr<ClientAuthManager> uacAuth(new ClientAuthManager);
   dumUac->setMasterProfile(uacMasterProfile);
   dumUac->setClientAuthManager(std::move(uacAuth));

   TestUac uac;
   dumUac->setInviteSessionHandler(&uac);
   dumUac->setClientRegistrationHandler(&uac);
   dumUac->addOutOfDialogHandler(OPTIONS, &uac);

   unique_ptr<AppDialogSetFactory> uac_dsf(new testAppDialogSetFactory);
   dumUac->setAppDialogSetFactory(std::move(uac_dsf));

   if ( doReg ) 
   {
      dumUac->getMasterProfile()->setDigestCredential(uacAor.uri().host(), uacAor.uri().user(), uacPasswd);
   }
   if (useOutbound) 
   {
       dumUac->getMasterProfile()->setOutboundProxy(outboundUri);
       dumUac->getMasterProfile()->addSupportedOptionTag(Token(Symbols::Outbound));
   }

   dumUac->getMasterProfile()->setDefaultFrom(uacAor);
   dumUac->getMasterProfile()->addSupportedMethod(INFO);
   dumUac->getMasterProfile()->addSupportedMethod(MESSAGE);
   dumUac->getMasterProfile()->addSupportedMimeType(INFO, PlainContents::getStaticType());
   dumUac->getMasterProfile()->addSupportedMimeType(MESSAGE, PlainContents::getStaticType());
   dumUac->getMasterProfile()->setDefaultRegistrationTime(70);

   //set up UAS
   SipStack stackUas;
   DialogUsageManager* dumUas = new DialogUsageManager(stackUas);
   stackUas.addTransport(UDP, 12010);
   stackUas.addTransport(TCP, 12010);
   
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

   time_t bHangupAt = 0;
   TestUas uas(&bHangupAt);
   dumUas->setClientRegistrationHandler(&uas);
   dumUas->setInviteSessionHandler(&uas);
   dumUas->addOutOfDialogHandler(OPTIONS, &uas);

   std::unique_ptr<AppDialogSetFactory> uas_dsf(new testAppDialogSetFactory);
   dumUas->setAppDialogSetFactory(std::move(uas_dsf));

   if (doReg) 
   {
      auto regMessage = dumUas->makeRegistration(uasAor, new testAppDialogSet(*dumUac, "UAS(Registration)"));
      InfoLog(<< "Sending register for Uas: " << endl << regMessage);
      dumUas->send(std::move(regMessage));
   } 
   else 
   {
      uas.registered = true;
   }
   if (doReg) 
   {
      auto regMessage = dumUac->makeRegistration(uacAor, new testAppDialogSet(*dumUac, "UAC(Registration)"));
      InfoLog(<< "Sending register for Uac: " << endl << regMessage);
      dumUac->send(std::move(regMessage));
   } 
   else 
   {
      uac.registered = true;
   }

   bool stoppedRegistering = false;
   bool startedCallFlow = false;
   bool hungup = false;   
   TestShutdownHandler uasShutdownHandler("UAS");   
   TestShutdownHandler uacShutdownHandler("UAC");   

   while (!(uasShutdownHandler.dumShutDown && uacShutdownHandler.dumShutDown))
   {
     if (!uacShutdownHandler.dumShutDown)
     {
        stackUac.process(50);
        while(dumUac->process());
     }
     if (!uasShutdownHandler.dumShutDown)
     {
        stackUas.process(50);
        while(dumUas->process());
     }

     if (!(uas.done && uac.done))
     {
        if (uas.registered && uac.registered && !startedCallFlow)
        {
           if (!startedCallFlow)
           {
              startedCallFlow = true;
              if ( doReg ) {
                 InfoLog(<< "!!!!!!!!!!!!!!!! Registered !!!!!!!!!!!!!!!! ");
              }

              // Kick off call flow by sending an OPTIONS request then an INVITE request from the UAC to the UAS
              //cout << "UAC: Sending Options Request to UAS." << endl;
              //dumUac->send(dumUac->makeOutOfDialogRequest(uasAor, OPTIONS, new testAppDialogSet(*dumUac, "UAC(OPTIONS)")));  // Should probably add Allow, Accept, Accept-Encoding, Accept-Language and Supported headers - but this is fine for testing/demonstration

              //cout << "UAC: Sending Invite Request to UAS." << endl;
              //dumUac->send(dumUac->makeInviteSession(uasAor, uac.mSdp, new testAppDialogSet(*dumUac, "UAC(INVITE)")));
           }
        }

        // Check if we should hangup yet
        if (bHangupAt!=0)
        {
           if (time(NULL)>bHangupAt && !hungup)
           {
              //hungup = true;
              //uas.hangup();
           }
        }
     }
     else
     {
        if (!stoppedRegistering)
        {
           stoppedRegistering = true;
           dumUas->shutdown(&uasShutdownHandler);
           dumUac->shutdown(&uacShutdownHandler);
            if ( doReg ) 
            {
              uas.registerHandle->stopRegistering();
              uac.registerHandle->stopRegistering();
            }
        }
     }
   }

   // OK to delete DUM objects now
   delete dumUac; 
   delete dumUas;

   InfoLog(<< "!!!!!!!!!!!!!!!!!! Successful !!!!!!!!!! ");
#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   }
#endif

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
