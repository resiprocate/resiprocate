#include "resiprocate/SdpContents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"
#include "resiprocate/dum/ClientRegistration.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/RegistrationHandler.hxx"
#include "resiprocate/dum/ServerInviteSession.hxx"
#include "resiprocate/os/Log.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Random.hxx"

#include <time.h>

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
class TestInviteSessionHandler : public InviteSessionHandler
{
   public:
      Data name;
      
      TestInviteSessionHandler(const Data& n) : name(n) 
      {
         InfoLog(  << "TestInviteSessionHandler::TestInviteSessionHandler(" << name << ")");         
      }
      
      /// called when an initial INVITE arrives 
      virtual void onNewSession(ClientInviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onNewSession(" << name << ")  "  << msg.brief());
      }
      
      virtual void onNewSession(ServerInviteSessionHandle, const SipMessage& msg)
      {
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

      virtual void onTerminated(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onTerminated " << msg.brief());
      }

      virtual void onReadyToSend(InviteSessionHandle, SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onReadyToSend " << msg.brief());
      }

      virtual void onAnswer(InviteSessionHandle, const SipMessage& msg, const SdpContents*)
      {
         InfoLog(  << "TestInviteSessionHandler::onAnswer" << msg.brief());
      }

      virtual void onOffer(InviteSessionHandle is, const SipMessage& msg, const SdpContents*)      
      {
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

      virtual void onRefer(InviteSessionHandle, const SipMessage& msg) 
      {
         InfoLog(  << "TestInviteSessionHandler::onRefer " << msg.brief());
      }

      virtual void onReInvite(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onReInvite " << msg.brief());
      }
};

class TestUac : public TestInviteSessionHandler, public ClientRegistrationHandler
{
   public:
      bool done;
      bool registered; 
      SdpContents* sdp;     
      HeaderFieldValue* hfv;      
      Data* txt;      

      TestUac() 
         : TestInviteSessionHandler("UAC"), 
           done(false),
           registered(false),
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

      ~TestUac()
      {
         delete sdp;
      }

      virtual void onProvisional(ClientInviteSessionHandle, const SipMessage& msg)
      {
         InfoLog ( << "TestUac::onProvisional" << msg.brief());
      }      

      virtual void onConnected(ClientInviteSessionHandle cis, const SipMessage& msg)
      {
         InfoLog ( << "TestUac::onConnected" << msg.brief());
         cis->send(cis->ackConnection());
      }

      virtual void onTerminated(InviteSessionHandle is, const SipMessage& msg)
      {
         InfoLog ( << "TestUac::onTerminated" << msg.brief());
         done = true;
      }

            virtual void onSuccess(ClientRegistrationHandle h, const SipMessage& response)
      {
          InfoLog( << "TestUac(Register)::onSuccess: " << endl << response );
          registered = true;
      }

      virtual void onFailure(ClientRegistrationHandle, const SipMessage& msg)
      {
         InfoLog( << "TestUac(Register)::onFailure: " << endl << msg );
         throw;
      }

};

class TestUas : public TestInviteSessionHandler, public ClientRegistrationHandler
{

   public:
      bool done;
      time_t* pHangupAt;
      bool registered;
      SdpContents* sdp;
      HeaderFieldValue* hfv;
      Data* txt;      

      TestUas(time_t* pHangupAt) 
         : TestInviteSessionHandler("UAS"), 
           done(false),
           registered(false),
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
         InfoLog(  << "TestUas::onNewSession " << msg.brief());
         sis->send(sis->provisional(180));
         sis->send(sis->accept());
      }

      virtual void onSuccess(ClientRegistrationHandle h, const SipMessage& response)
      {
          InfoLog( << "TestUas(Register)::onSuccess: " << endl << response );
          registered = true;
      }

      virtual void onFailure(ClientRegistrationHandle, const SipMessage& msg)
      {
         InfoLog( << "TestUas(Register)::onFailure: " << endl << msg );
         throw;
      }

      virtual void onTerminated(InviteSessionHandle is, const SipMessage& msg)
      {
         InfoLog ( << "TestUas::onTerminated" << msg.brief());
         done = true;
      }

      void
      onOffer(ServerInviteSessionHandle sis, const SipMessage& msg )
      {
         InfoLog ( << "TestUas::onOffer" << msg.brief());
         sis->setAnswer(sdp);
         sis->send(sis->accept());
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


#define REMOTE 0
int 
main (int argc, char** argv)
{

   //set up UAC
   SipStack stackUac;
   stackUac.addTransport(UDP, 15060);
   DialogUsageManager dumUac(stackUac);

   Profile uacProfile;   
   ClientAuthManager uacAuth(uacProfile);
   dumUac.setProfile(&uacProfile);
   dumUac.setClientAuthManager(&uacAuth);

   TestUac uac;
   dumUac.setInviteSessionHandler(&uac);
   dumUac.setClientRegistrationHandler(&uac);
   
#if REMOTE
   //Vonage
//   NameAddr uacAor("sip:13015604287@sphone.vopr.vonage.net");
//   dumUac.getProfile()->addDigestCredential( "sphone.vopr.vonage.net", "13015604287", "jaRGRdkCQ9" );
   //FWD
//    NameAddr uacAor("sip:21186@fwd.pulver.com");
//    dumUac.getProfile()->addDigestCredential( "fwd.pulver.com", "21186", "111111" );
   //Purple
      NameAddr uacAor("sip:101@xten.gloo.net");
    dumUac.getProfile()->addDigestCredential( "xten.gloo.net", "derek@xten.gloo.net", "123456" );
    dumUac.getProfile()->setOutboundProxy(Uri("sip:64.124.66.33:9090"));    
#else
   //Local
   NameAddr uacAor("sip:101@internal.xten.net");
#endif
   dumUac.getProfile()->setDefaultAor(uacAor);
   dumUac.getProfile()->setDefaultRegistrationTime(70);

   //set up UAS
   SipStack stackUas;
   stackUas.addTransport(UDP, 15070);
   DialogUsageManager dumUas(stackUas);
   
   Profile uasProfile;   
   ClientAuthManager uasAuth(uasProfile);
   dumUas.setProfile(&uasProfile);
   dumUas.setClientAuthManager(&uasAuth);
   
#if REMOTE
//    Vonage
//    NameAddr uasAor("sip:13015604286@sphone.vopr.vonage.net");
//    dumUas.getProfile()->addDigestCredential( "sphone.vopr.vonage.net", "13015604286", "9E5aI9L9h9" );
   //Purple
      NameAddr uasAor("sip:105@xten.gloo.net");
    dumUas.getProfile()->addDigestCredential( "xten.gloo.net", "derek@xten.gloo.net", "123456" );
    dumUas.getProfile()->setOutboundProxy(Uri("sip:64.124.66.33:9090"));    
#else
   //Local
   NameAddr uasAor("sip:105@internal.xten.net");
#endif
   dumUas.getProfile()->setDefaultRegistrationTime(70);
   dumUas.getProfile()->setDefaultAor(uasAor);


   time_t bHangupAt = 0;
   TestUas uas(&bHangupAt);
   dumUas.setClientRegistrationHandler(&uas);
   dumUas.setInviteSessionHandler(&uas);

//   NameAddr contact;
//   contact.uri().user() = "13015604286";   
//   regMessage.header(h_Contacts).push_back(contact);   
   {
      SipMessage& regMessage = dumUas.makeRegistration(uasAor);
      InfoLog( << regMessage << "Generated register for Uas: " << endl << regMessage );
      dumUas.send(regMessage);
   }
   {
      SipMessage& regMessage = dumUac.makeRegistration(uacAor);
      InfoLog( << regMessage << "Generated register for Uac: " << endl << regMessage );
      dumUac.send(regMessage);
   }
   bool startedCallFlow = false;
   
   while ( (!uas.done) || (!uac.done) )
   {
     FdSet fdset;
     dumUac.buildFdSet(fdset);
     dumUas.buildFdSet(fdset);
     int err = fdset.selectMilliSeconds(100);
     assert ( err != -1 );
     dumUac.process(fdset);
     dumUas.process(fdset);
     if (uas.registered && uac.registered && !startedCallFlow)
     if (!startedCallFlow)
     {
        startedCallFlow = true;
        dumUac.send(dumUac.makeInviteSession(uasAor.uri(), uac.sdp));
     }

     if (bHangupAt!=0)
     {
       if (time(NULL)>bHangupAt)
       {
         uas.hangup();
       }
     }
   }   
   // How do I turn these things off? For now, we just blow
   // out with all the wheels turning...try graceful shutdown in this test soon.
}
