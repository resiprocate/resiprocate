#include "resiprocate/SdpContents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"
#include "resiprocate/dum/ClientRegistration.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/DumShutdownHandler.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/RegistrationHandler.hxx"
#include "resiprocate/dum/ServerInviteSession.hxx"
#include "resiprocate/dum/ServerOutOfDialogReq.hxx"
#include "resiprocate/dum/OutOfDialogHandler.hxx"
#include "resiprocate/os/Log.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Random.hxx"

#include <sstream>
#include <time.h>
//#include <windows.h>

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;

void sleepSeconds(unsigned int seconds)
{
#ifdef WIN32
   Sleep(seconds*1000);
#else
   sleep(seconds);
#endif
}


void generateSdpSession(SdpContents& mMySdpOffer)
{
   Data myIP = "192.168.0.155";
   
   mMySdpOffer.session().version() = 0;
   mMySdpOffer.session().name() = "-";
   mMySdpOffer.session().origin() = SdpContents::Session::Origin(Data("dum"),Random::getRandom(), 
                                                                 Random::getRandom(), 
                                                                 SdpContents::IP4,
                                                                 myIP);
   mMySdpOffer.session().addTime(SdpContents::Session::Time(0,0));
   mMySdpOffer.session().connection() =
      SdpContents::Session::Connection(SdpContents::IP4, myIP, 0);
   SdpContents::Session::Medium medium(Symbols::audio, 0, 1, Symbols::RTP_AVP);
   SdpContents::Session::Codec codec("PCMU", 3, 8000);
   medium.addCodec(codec);
   mMySdpOffer.session().addMedium(medium);
   mMySdpOffer.session().origin().setAddress(myIP);
   mMySdpOffer.session().connection().setAddress(myIP);
   static int port = 9000;
   mMySdpOffer.session().media().front().port() = port++; // should be a random
}




//default, Debug outputs
class TestInviteSessionHandler : public InviteSessionHandler, public ClientRegistrationHandler
{
   public:
      Data name;
      bool registered;
      ClientRegistrationHandle registerHandle;
      
      TestInviteSessionHandler(const Data& n) : name(n), registered(false) 
      {
         InfoLog(  << "TestInviteSessionHandler::TestInviteSessionHandler(" << name << ")");         
      }

      virtual ~TestInviteSessionHandler()
      {
      }
      
      virtual void onSuccess(ClientRegistrationHandle h, const SipMessage& response)
      {         
         registerHandle = h;   
         assert(registerHandle.isValid());         
         {
            stringstream s;
            s << "Handle created: " << &h;
            //OutputDebugString(s.str().c_str());
         }
         InfoLog( << "###Register::onSuccess: ###" << name << endl);
         registered = true;
      }

      virtual void onFailure(ClientRegistrationHandle, const SipMessage& msg)
      {
         InfoLog( << "###Register::onFailure: ###" << name << endl);
         assert(0);
      }

      virtual void onRemoved(ClientRegistrationHandle)
      {
         InfoLog(  << "TestInviteSessionHandler::onRemoved(" << name << ")");
      }

      /// called when an initial INVITE arrives 
      virtual void onNewSession(ClientInviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)
      {
         assert(0);         
         InfoLog(  << "TestInviteSessionHandler::onNewSession(" << name << ")  "  << msg.brief());
      }
      
      virtual void onNewSession(ServerInviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)
      {
         assert(0);         
         InfoLog(  << "TestInviteSessionHandler::onNewSession " << msg.brief());
      }
      virtual void onFailure(ClientInviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onFailure " << msg.brief());
      }
      
      virtual void onEarlyMedia(ClientInviteSessionHandle, const SipMessage& msg, const SdpContents*)
      {
         InfoLog(  << "TestInviteSessionHandler::onEarlyMedia " << msg.brief());
      }

      virtual void onProvisional(ClientInviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onProvisional " << msg.brief());
      }

      virtual void onConnected(ClientInviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onConnected(Uac)" << msg.brief());
      }

      virtual void onConnected(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onConnected(Uas) " << msg.brief());
      }

      virtual void onStaleCallTimeout(ClientInviteSessionHandle)
      {
         InfoLog(  << "TestInviteSessionHandler::onStaleCallTimeout" );
      }

      virtual void onRedirected(ClientInviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onRedirected " << msg.brief());
      }

      virtual void onTerminated(InviteSessionHandle, const SipMessage& msg)
      {
         assert(0);         
         InfoLog(  << "TestInviteSessionHandler::onTerminated " << msg.brief());
      }

//       virtual void onReadyToSend(InviteSessionHandle, SipMessage& msg)
//       {
//          InfoLog(  << "TestInviteSessionHandler::onReadyToSend " << msg.brief());
//       }

      virtual void onAnswer(InviteSessionHandle, const SipMessage& msg, const SdpContents*)
      {
         assert(0);
         InfoLog(  << "TestInviteSessionHandler::onAnswer" << msg.brief());
      }

      virtual void onOffer(InviteSessionHandle is, const SipMessage& msg, const SdpContents*)      
      {
         assert(0);
         InfoLog(  << "TestInviteSessionHandler::onOffer " << msg.brief());
      }
      
      virtual void onOfferRejected(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onOfferRejected " << msg.brief());
      }

      virtual void onDialogModified(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onDialogModified " << msg.brief());
      }

      virtual void onInfo(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onInfo" << msg.brief());
      }

      virtual void onRefer(InviteSessionHandle, ServerSubscriptionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onRefer " << msg.brief());
      }

      virtual void onReInvite(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onReInvite " << msg.brief());
      }

      /// called when an REFER message receives a failure response 
      virtual void onReferRejected(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onReferRejected " << msg.brief());
      }
};

class TestUac : public TestInviteSessionHandler
{
   public:
      bool done;
      SdpContents* sdp;     
      HeaderFieldValue* hfv;      
      Data* txt;      

      TestUac() 
         : TestInviteSessionHandler("UAC"), 
           done(false),
           sdp(0),
           hfv(0),
           txt(0)
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
         
         hfv = new HeaderFieldValue(txt->data(), txt->size());
         Mime type("application", "sdp");
         sdp = new SdpContents(hfv, type);
      }

      virtual ~TestUac()
      {
         delete sdp;
      }

      virtual void onNewSession(ClientInviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)
      {
         InfoLog(  << "TestUac::onNewSession(" << name << ")  "  << msg.brief());
      }

      virtual void onProvisional(ClientInviteSessionHandle, const SipMessage& msg)
      {
         InfoLog ( << "TestUac::onProvisional" << msg.brief());
      }      

      virtual void onConnected(ClientInviteSessionHandle cis, const SipMessage& msg)
      {
         InfoLog ( << "###TestUac::onConnected###" << msg.brief());
         cis->send(cis->ackConnection());
      }

      virtual void onAnswer(InviteSessionHandle, const SipMessage& msg, const SdpContents*)
      {
         InfoLog(  << "###TestUac::onAnswer###" << msg);
      }


      virtual void onTerminated(InviteSessionHandle is, const SipMessage& msg)
      {
         InfoLog ( << "###TestUac::onTerminated###" << msg.brief());
         done = true;
      }
};

class TestUas : public TestInviteSessionHandler
{

   public:
      bool done;
      time_t* pHangupAt;

      SdpContents* sdp;
      HeaderFieldValue* hfv;
      Data* txt;      

      TestUas(time_t* pH) 
         : TestInviteSessionHandler("UAS"), 
           done(false),
           pHangupAt(pH),
           hfv(0)
      { 
         pHangupAt = pHangupAt;

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
         
         hfv = new HeaderFieldValue(txt->data(), txt->size());
         Mime type("application", "sdp");
         sdp = new SdpContents(hfv, type);
      }

      ~TestUas()
      {
         delete sdp;
      }

      virtual void 
      onNewSession(ServerInviteSessionHandle sis, InviteSession::OfferAnswerType oat, const SipMessage& msg)
      {
         InfoLog(  << "###TestUas::onNewSession### " << msg.brief());
         mSis = sis;         
         sis->send(sis->provisional(180));
      }

      virtual void onFailure(ClientRegistrationHandle, const SipMessage& msg)
      {
         InfoLog( << "TestUas(Register)::onFailure: " << endl << msg );
         throw;
      }

      virtual void onTerminated(InviteSessionHandle is, const SipMessage& msg)
      {
         InfoLog ( << "###TestUas::onTerminated###" << msg.brief());
         done = true;
      }

      virtual void onOffer(InviteSessionHandle is, const SipMessage& msg, const SdpContents*)      
      {
         InfoLog ( << "###TestUas::onOffer###" << msg.brief());
         is->setAnswer(sdp);
         mSis->send(mSis->accept());
         *pHangupAt = time(NULL) + 5;
      }

      // Normal people wouldn't put this functionality in the handler
      // it's a kludge for this test for right now
      void hangup()
      {
         if (mSis.isValid())
         {
            InfoLog ( << "TestUas::hangup" );
            mSis->send(mSis->end());
         }
      }
   private:
      ServerInviteSessionHandle mSis;      
};

class TestShutdownHandler : public DumShutdownHandler
{
   public:
      TestShutdownHandler()
         : dumShutDown(false)
      {
      }
      bool dumShutDown;
      virtual void onDumCanBeDeleted() 
      {
         InfoLog ( << "TestShutdownHandler::onDumCanBeDeleted" );         
         dumShutDown = true;
      }
};

class TestOutOfDialogHandler : public OutOfDialogHandler
{
   public:      
	// Out-of-Dialog Callbacks
    virtual void onSuccess(ClientOutOfDialogReqHandle, const SipMessage& successResponse)
	{
		InfoLog ( << "TestOutOfDialogHandler::onSuccess-ClientOutOfDialogReq: " << successResponse.brief());
	}
	virtual void onFailure(ClientOutOfDialogReqHandle, const SipMessage& errorResponse)
	{
		InfoLog ( << "TestOutOfDialogHandler::onFailure-ClientOutOfDialogReq: " << errorResponse.brief());
	}
    virtual void onReceivedRequest(ServerOutOfDialogReqHandle ood, const SipMessage& request)
	{
		InfoLog ( << "TestOutOfDialogHandler::onReceivedRequest-ServerOutOfDialogReq: " << request.brief());
		// Add SDP to response here if required
		ood->send(ood->answerOptions());
		//ood->send(ood->answerOptions(false));  // fIncludeAllows = false for proxy servers
	}
};


#define NO_REGISTRATION 1
int 
main (int argc, char** argv)
{
//    int level=(int)Log::Info;
//    if (argc >1 ) level = atoi(argv[1]);
   
//    Log::initialize(Log::Cout, (resip::Log::Level)level, argv[0]);

    Log::initialize(Log::Cout, resip::Log::Info, argv[0]);


   //set up UAC
   SipStack stackUac;
   stackUac.addTransport(UDP, 12005);
   DialogUsageManager* dumUac = new DialogUsageManager(stackUac);

   Profile uacProfile;      
   auto_ptr<ClientAuthManager> uacAuth(new ClientAuthManager(uacProfile));
   dumUac->setProfile(&uacProfile);
   dumUac->setClientAuthManager(uacAuth);

   TestUac uac;
   dumUac->setInviteSessionHandler(&uac);
   dumUac->setClientRegistrationHandler(&uac);

   TestOutOfDialogHandler uac_ood;
   dumUac->addOutOfDialogHandler(OPTIONS, &uac_ood);

#if !defined(NO_REGISTRATION)
   //your aor, credentials, etc here
   NameAddr uacAor("sip:101@foo.net");
   dumUac->getProfile()->addDigestCredential( "foo.net", "derek@foo.net", "pass6" );
//   dumUac->getProfile()->setOutboundProxy(Uri("sip:64.125.66.33:9090"));    
   dumUac->getProfile()->setOutboundProxy(Uri("sip:209.134.58.33:9090"));    
#else
   NameAddr uacAor("sip:UAC@127.0.0.1:12005");
#endif

   dumUac->getProfile()->setDefaultFrom(uacAor);
   dumUac->getProfile()->setDefaultRegistrationTime(70);
   dumUac->getProfile()->addSupportedLanguage(Token("en"));
   dumUac->getProfile()->addSupportedMimeType(Mime("application", "sdp"));
   dumUac->getProfile()->addSupportedMethod(INVITE);
   dumUac->getProfile()->addSupportedMethod(ACK);
   dumUac->getProfile()->addSupportedMethod(CANCEL);
   dumUac->getProfile()->addSupportedMethod(OPTIONS);
   dumUac->getProfile()->addSupportedMethod(BYE);
   dumUac->getProfile()->addSupportedMethod(REGISTER);
   //dumUac->getProfile()->addSupportedOptionTags();  

   //set up UAS
   SipStack stackUas;
   stackUas.addTransport(UDP, 12010);
   DialogUsageManager* dumUas = new DialogUsageManager(stackUas);
   
   Profile uasProfile;   
   std::auto_ptr<ClientAuthManager> uasAuth(new ClientAuthManager(uasProfile));
   dumUas->setProfile(&uasProfile);
   dumUas->setClientAuthManager(uasAuth);

#if !defined(NO_REGISTRATION)
   //your aor, credentials, etc here
   NameAddr uasAor("sip:105@foo.net");
   dumUas->getProfile()->addDigestCredential( "foo.net", "derek@foo.net", "pass6" );
//   dumUas->getProfile()->setOutboundProxy(Uri("sip:64.125.66.33:9090"));    
   dumUas->getProfile()->setOutboundProxy(Uri("sip:209.134.58.33:9090"));    
#else
   NameAddr uasAor("sip:UAS@127.0.0.1:12010");
#endif

   dumUas->getProfile()->setDefaultRegistrationTime(70);
   dumUas->getProfile()->setDefaultFrom(uasAor);
   dumUas->getProfile()->addSupportedLanguage(Token("en"));
   dumUas->getProfile()->addSupportedMimeType(Mime("application", "sdp"));
   dumUas->getProfile()->addSupportedMethod(INVITE);
   dumUas->getProfile()->addSupportedMethod(ACK);
   dumUas->getProfile()->addSupportedMethod(CANCEL);
   dumUas->getProfile()->addSupportedMethod(OPTIONS);
   dumUas->getProfile()->addSupportedMethod(BYE);
   dumUas->getProfile()->addSupportedMethod(REGISTER);
   //dumUas->getProfile()->addSupportedOptionTags();  

   time_t bHangupAt = 0;
   TestUas uas(&bHangupAt);
   dumUas->setClientRegistrationHandler(&uas);
   dumUas->setInviteSessionHandler(&uas);

   TestOutOfDialogHandler uas_ood;
   dumUas->addOutOfDialogHandler(OPTIONS, &uas_ood);

#if !defined(NO_REGISTRATION)
//   NameAddr contact;
//   contact.uri().user() = "13015604286";   
//   regMessage.header(h_Contacts).push_back(contact);   
   {
      SipMessage& regMessage = dumUas->makeRegistration(uasAor);
      InfoLog( << regMessage << "Generated register for Uas: " << endl << regMessage );
      dumUas->send(regMessage);
   }
   {
      SipMessage& regMessage = dumUac->makeRegistration(uacAor);
      InfoLog( << regMessage << "Generated register for Uac: " << endl << regMessage );
      dumUac->send(regMessage);
   }
#else
   uac.registered = true;
   uas.registered = true;
#endif

   bool finishedTest = false;
   bool stoppedRegistering = false;
   bool startedCallFlow = false;
   bool hungup = false;   
   TestShutdownHandler uasShutdownHandler;   
   TestShutdownHandler uacShutdownHandler;   

   while (!(uasShutdownHandler.dumShutDown && uacShutdownHandler.dumShutDown))
   {
     if (!uacShutdownHandler.dumShutDown)
     {
        FdSet fdset;
        dumUac->buildFdSet(fdset);
        int err = fdset.selectMilliSeconds(dumUac->getTimeTillNextProcessMS());
        assert ( err != -1 );
        dumUac->process(fdset);
     }
     if (!uasShutdownHandler.dumShutDown)
     {
        FdSet fdset;
        dumUas->buildFdSet(fdset);
        int err = fdset.selectMilliSeconds(dumUas->getTimeTillNextProcessMS());
        assert ( err != -1 );
        dumUas->process(fdset);
     }

     if (!(uas.done && uac.done))
     {
        if (uas.registered && uac.registered && !startedCallFlow)
           if (!startedCallFlow)
           {
              startedCallFlow = true;
#if !defined(NO_REGISTRATION)
              InfoLog( << "!!!!!!!!!!!!!!!! Registered !!!!!!!!!!!!!!!! " );        
#endif

              InfoLog( << "#### Sending Options Request ####");
			  dumUac->send(dumUac->makeOutOfDialogRequest(uasAor, uacAor, OPTIONS));  // Should probably add Allow, Accept, Accept-Encoding, Accept-Language and Supported headers - but this is fine for testing/demonstration

              InfoLog( << "#### Sending Invite ####");
              dumUac->send(dumUac->makeInviteSession(uasAor, uacAor, uac.sdp));
           }

        if (bHangupAt!=0)
        {
           if (time(NULL)>bHangupAt && !hungup)
           {
              hungup = true;
              uas.hangup();
           }
        }
     }
     else
     {
        if (!stoppedRegistering)
        {
           finishedTest = true;
           stoppedRegistering = true;
           dumUas->shutdown(&uasShutdownHandler);
           dumUac->shutdown(&uacShutdownHandler);
#if !defined(NO_REGISTRATION)
           uas.registerHandle->stopRegistering();
           uac.registerHandle->stopRegistering();
#endif
        }
     }
   }
   // OK to delete DUM objects now
   delete dumUac; 
   delete dumUas;
   InfoLog ( << "!!!!!!!!!!!!!!!!!! Somewhat successful !!!!!!!!!! " );
}
