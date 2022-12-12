
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <thread>
#include <time.h>
#include <utility>

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
#include "media/gstreamer/GStreamerUtils.hxx"
#include "media/gstreamer/GstRtpManager.hxx"
#include "media/gstreamer/GstRtpSession.hxx"
#include "rutil/ServerProcess.hxx"
#include "rutil/hep/HepAgent.hxx"

#include <gstreamermm.h>
#include <glibmm/main.h>

#include "EchoTestConfig.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace echotest;
//using namespace resipgst;
using namespace Gst;
using namespace Glib;
using namespace resip;
using namespace std;

Data myIP;

class GstThread
{
   public:
      GstThread()
      {
         main_loop = Glib::MainLoop::create();
         mThread = make_shared<thread>([this](){
            InfoLog(<<"GstThread starting");
            main_loop->run();
         });
      }

      virtual ~GstThread()
      {
         InfoLog(<<"GstThread stopping");
         if(main_loop)
         {
            main_loop->quit();
         }
         InfoLog(<<"GstThread destroyed");
      }

   private:
      Glib::RefPtr<Glib::MainLoop> main_loop;
      shared_ptr<thread> mThread;
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
class TestAppDialog : public AppDialog
{
public:
   TestAppDialog(HandleManager& ham, Data &SampleAppData, shared_ptr<resipgst::GstRtpManager> rTPManager)
    : AppDialog(ham),
      mSampleAppData(SampleAppData),
      mRtpSession(make_shared<resipgst::GstRtpSession>(*rTPManager, false))  // FIXME isWebRTC
   {  
      InfoLog(<< mSampleAppData << ": TestAppDialog: created.");
   }
   virtual ~TestAppDialog()
   { 
      InfoLog(<< mSampleAppData << ": TestAppDialog: destroyed.");
   }

   bool
   isWebRTCSession()
   {
      return false;
   }

   void
   onMediaSourceAdded(const RefPtr<Pad>& pad)
   {
      Glib::ustring padName = pad->get_name();
      DebugLog(<<"onMediaSourceAdded: on-pad-added, padName: " << padName << " stream ID: " << pad->get_stream_id().raw());
      if(pad->get_direction() == PAD_SRC)
      {
         RefPtr<Caps> caps = pad->get_current_caps();
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

               DebugLog(<<"completed connection from incoming to outgoing stream");
            }
            else
            {
               // FIXME - disconnect the test sources and do the loopback
               ErrLog(<<"not doing loopback for WebRTC");
            }
         }
      }
      else
      {
         StackLog(<<"not a source pad: " << padName);
      }
   }

   void prepareStream(unsigned int streamId, bool loopbackMode)
   {
      resipgst::GstRtpSession::CapsVector capsV = mRtpSession->getOutgoingCaps();
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


   Data mSampleAppData;
   shared_ptr<resipgst::GstRtpSession> mRtpSession;

   Glib::RefPtr<Gst::Pipeline> mPipeline;
   Glib::RefPtr<Gst::Element> mMediaBin;
   Glib::RefPtr<Gst::Element> mRtpTransportElement;

};

class TestAppDialogSet : public AppDialogSet
{
public:
   TestAppDialogSet(DialogUsageManager& dum, Data SampleAppData, shared_ptr<resipgst::GstRtpManager> rTPManager) : AppDialogSet(dum), mSampleAppData(SampleAppData), mRtpManager(rTPManager)
   {  
      InfoLog(<< mSampleAppData << ": TestAppDialogSet: created.");
   }
   virtual ~TestAppDialogSet() 
   {  
      InfoLog(<< mSampleAppData << ": TestAppDialogSet: destroyed.");
   }
   virtual AppDialog* createAppDialog(const SipMessage& msg) 
   {  
      return new TestAppDialog(mDum, mSampleAppData, mRtpManager);
   }
   virtual std::shared_ptr<UserProfile> selectUASUserProfile(const SipMessage& msg)
   { 
      InfoLog(<< mSampleAppData << ": TestAppDialogSet: UAS UserProfile requested for msg: " << msg.brief());
      return mDum.getMasterUserProfile(); 
   }
   Data mSampleAppData;
   shared_ptr<resipgst::GstRtpManager> mRtpManager;
};

class TestAppDialogSetFactory : public AppDialogSetFactory
{
public:
   TestAppDialogSetFactory(shared_ptr<resipgst::GstRtpManager> rTPManager) : mRtpManager(rTPManager) {}

   virtual AppDialogSet* createAppDialogSet(DialogUsageManager& dum, const SipMessage& msg) 
   {  return new TestAppDialogSet(dum, Data("UAS") + Data("(") + getMethodName(msg.header(h_RequestLine).getMethod()) + Data(")"), mRtpManager);  }
   // For a UAS the testAppDialogSet will be created by DUM using this function.  If you want to set 
   // Application Data, then one approach is to wait for onNewSession(ServerInviteSessionHandle ...) 
   // to be called, then use the ServerInviteSessionHandle to get at the AppDialogSet or AppDialog,
   // then cast to your derived class and set the desired application data.

private:
   shared_ptr<resipgst::GstRtpManager> mRtpManager;
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
         resip_assert(registerHandle.isValid());         
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
         resip_assert(0); // This is overriden in UAS and UAC specific handlers
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

      typedef std::map<DialogId, shared_ptr<GstThread>> MediaThreads;
      MediaThreads mMediaThreads;

      std::shared_ptr<HepAgent> mHepAgent;

      TestUas(const Data& pipelineId, std::shared_ptr<HepAgent> agent)
         : TestInviteSessionHandler("UAS"),
           done(false),
           requestedOffer(false),
           mHepAgent(agent)
      { 
      }

      ~TestUas()
      {
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
         mMediaThreads.erase(is->getDialogId());
         done = true;
      }


      bool
      onGstBusMessage(const RefPtr<Gst::Bus>&, const RefPtr<Gst::Message>& message)
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

      virtual void onOffer(InviteSessionHandle is, const SipMessage& msg, const SdpContents& sdp)      
      {
         InfoLog(<< name << ": InviteSession-onOffer(SDP)");
         TestAppDialog* d = dynamic_cast<TestAppDialog*>(is->getAppDialog().get());
         const Data& callee = msg.header(h_RequestLine).uri().user();
         InfoLog(<< name << ": call is for: " << callee);
         std::shared_ptr<SdpContents> offer(dynamic_cast<SdpContents*>(sdp.clone()));
         shared_ptr<SdpContents> mSdp = d->mRtpSession->buildAnswer(offer);
         //sdp->encode(cout);
         InfoLog(<< name << ": starting GStreamer media thread");
         d->mRtpSession->initHomer(msg.header(h_CallId).value(), mHepAgent);

         if(d->mPipeline)
         {
            DebugLog(<<"mPipeline already exists");
         }
         else
         {
            d->mPipeline = Gst::Pipeline::create();
            d->mMediaBin = d->mRtpSession->getMediaBin();
            resip_assert(d->mMediaBin);
            d->mRtpTransportElement = d->mRtpSession->getRtpTransportBin();
            resip_assert(d->mRtpTransportElement);
            d->mPipeline->add(d->mMediaBin);
            d->mRtpSession->initOutgoingBins();
            bool isWebRTC = d->isWebRTCSession();
            bool loopbackMode = !isWebRTC;

            unsigned int streamCount = d->mRtpSession->getOutgoingCaps().size();
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
                  d->prepareStream(streamId, loopbackMode);
               }
            }
            // FIXME
            RefPtr<Bus> bus = d->mPipeline->get_bus();
            bus->add_watch(sigc::mem_fun(*this, &TestUas::onGstBusMessage));

            DebugLog(<<"mPipeline and mRtpTransportElement created");

            DebugLog(<<"going to STATE_READY");
            d->mPipeline->set_state(STATE_READY);

            d->mMediaBin->signal_pad_added().connect(sigc::mem_fun(*d, &TestAppDialog::onMediaSourceAdded));

            DebugLog(<<"going to STATE_PLAYING");
            StateChangeReturn ret = d->mPipeline->set_state(STATE_PLAYING);
            if (ret == STATE_CHANGE_FAILURE)
            {
               ErrLog(<<"pipeline fail");
               d->mPipeline.reset();
               is->reject(606);
               return;
            }

            d->mRtpSession->onPlaying(); // FIXME - use a signal in GstRtpSession

            mSdp = make_shared<SdpContents>(*d->mRtpSession->getLocalSdp());

            mMediaThreads[is->getDialogId()] = make_shared<GstThread>();
         }


         InfoLog(<< name << ": Sending 200 response with SDP answer.");
         is->provideAnswer(*mSdp);
         mSis->accept();
      }

      virtual void onOfferRequired(InviteSessionHandle is, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onOfferRequired - " << msg.brief());
         shared_ptr<SdpContents> mSdp;
         if(!mSdp)
         {
            TestAppDialog* d = dynamic_cast<TestAppDialog*>(is->getAppDialog().get());
            mSdp = d->mRtpSession->buildOffer(true, true);
         }
         is->provideOffer(*mSdp);
      }

      virtual void onConnected(InviteSessionHandle is, const SipMessage& msg)
      {
         InfoLog(<< name << ": InviteSession-onConnected - " << msg.brief());
         
         // At this point no NIT should have been sent
         resip_assert(!is->getLastSentNITRequest());
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

      std::shared_ptr<resipgst::GstRtpManager> mRtpManager;

   public:
      EchoTestServer() :
         stackUas(nullptr),
         dumUas(nullptr)
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

#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
         FindMemoryLeaks fml;
         {
#endif
         bool doReg = echoTestConfig.getConfigBool("Register", false);
         Data uasPasswd;
         int uasUdpPort = echoTestConfig.getConfigInt("UDPPort", 12010);
         int uasTcpPort = echoTestConfig.getConfigInt("TCPPort", 12010);
         Uri outboundUri = echoTestConfig.getConfigUri("OutboundProxyUri", Uri());
         bool useOutbound = (outboundUri != Uri());
         myIP = echoTestConfig.getConfigData("IPAddress", "127.0.0.1");

         uasPasswd = echoTestConfig.getConfigData("Password", Data::Empty);

         //set up UAS
         stackUas = new SipStack();
         std::shared_ptr<HepAgent> agent;
         if(!captureHost.empty())
         {
            agent = std::make_shared<HepAgent>(captureHost, capturePort, captureAgentID);
            stackUas->setTransportSipMessageLoggingHandler(std::make_shared<HEPSipMessageLoggingHandler>(agent));
         }

         dumUas = new DialogUsageManager(*stackUas);
         NameAddr uasAor;
         Uri uasContact;
         if(uasUdpPort)
         {
            stackUas->addTransport(UDP, uasUdpPort);
            uasContact = Uri("sip:" + myIP + ":" + Data(uasUdpPort));
         }
         if(uasTcpPort)
         {
            stackUas->addTransport(TCP, uasTcpPort);
            if(uasContact.port() == 0)
            {
               uasContact = Uri("sip:" + myIP + ":" + Data(uasTcpPort) + ";transport=tcp");
            }
         }
         if(uasContact.port() == 0)
         {
            uasContact = Uri("sip:" + myIP);
            ErrLog(<<"no port for uasContact");
            resip_assert(0);
         }
         uasAor = echoTestConfig.getConfigNameAddr("SIPUri", NameAddr("sip:UAS@" + myIP + ":" + Data(uasContact.port())));

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

         // FIXME - IPv6 needed
         mRtpManager = make_shared<resipgst::GstRtpManager>(myIP, mediaPortStart, mediaPortStart + 100);
         mRtpManager->setApplicationName("reSIProcate GStreamer Echo Test");

         TestUas uas(pipelineId, agent);
         dumUas->setClientRegistrationHandler(&uas);
         dumUas->setInviteSessionHandler(&uas);
         dumUas->addOutOfDialogHandler(OPTIONS, &uas);

         std::unique_ptr<AppDialogSetFactory> uas_dsf(new TestAppDialogSetFactory(mRtpManager));
         dumUas->setAppDialogSetFactory(std::move(uas_dsf));

         if (!doReg) 
         {
            uas.registered = true;
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
 * Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 * Copyright (c) 2021-2022, Daniel Pocock https://danielpocock.com
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
