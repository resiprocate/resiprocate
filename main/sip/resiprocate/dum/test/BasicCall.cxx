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
#include <time.h>

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;

//default, Debug outputs
class TestInviteSessionHandler : public InviteSessionHandler
{
   public:
      /// called when an initial INVITE arrives 
      virtual void onNewSession(ClientInviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onNewSession " << msg.brief());
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

      virtual void onOffer(InviteSessionHandle, const SipMessage& msg, const SdpContents*)      
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

class TestUac : public TestInviteSessionHandler
{
   public:
      bool done;

      TestUac() 
         : done(false)
      {}

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
};

class TestUas : public TestInviteSessionHandler, public ClientRegistrationHandler
{

   public:
      bool done;
      time_t* pHangupAt;
      bool registered;      

      TestUas(time_t* pHangupAt) 
         : done(false),
           registered(false)
      { 
         pHangupAt = pHangupAt;
      }

      void 
      onNewSession(ServerInviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)
      {
         InfoLog(  << "TestUas::onNewSession " << msg.brief());
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
         sis->setAnswer(new SdpContents());
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
   NameAddr uacAor("sip:test@192.168.0.156");
   dumUac.getProfile()->setDefaultAor(uacAor);

   //set up UAS
   SipStack stackUas;
   stackUas.addTransport(UDP, 15070);
   DialogUsageManager dumUas(stackUas);

   Profile uasProfile;   
   ClientAuthManager uasAuth(uasProfile);
   dumUas.setProfile(&uasProfile);
   dumUas.setClientAuthManager(&uasAuth);
   NameAddr uasAor("sip:13015604286@sphone.vopr.vonage.net");
   dumUas.getProfile()->setDefaultRegistrationTime(70);
   dumUas.getProfile()->setDefaultAor(uasAor);
   dumUas.getProfile()->addDigestCredential( "sphone.vopr.vonage.net", "13015604286", "" );


   time_t bHangupAt = 0;
   TestUas uas(&bHangupAt);
   dumUas.setClientRegistrationHandler(&uas);
   dumUas.setInviteSessionHandler(&uas);

   SipMessage& regMessage = dumUas.makeRegistration(uasAor);
   NameAddr contact;
   contact.uri().user() = "13015604286";   
//   regMessage.header(h_Contacts).push_back(contact);   
   InfoLog( << regMessage << "Generated register: " << endl << regMessage );
   dumUas.send(regMessage);

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

     if (uas.registered && !startedCallFlow)
     {
        startedCallFlow = true;
        dumUac.send(dumUac.makeInviteSession(uasAor.uri(), new SdpContents()));
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
