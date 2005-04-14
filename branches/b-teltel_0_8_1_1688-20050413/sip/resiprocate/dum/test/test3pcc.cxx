#include "resiprocate/SdpContents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/ShutdownMessage.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"
#include "resiprocate/dum/ClientRegistration.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/DumShutdownHandler.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/MasterProfile.hxx"
#include "resiprocate/dum/RegistrationHandler.hxx"
#include "resiprocate/dum/ServerInviteSession.hxx"
#include "resiprocate/dum/ServerOutOfDialogReq.hxx"
#include "resiprocate/dum/OutOfDialogHandler.hxx"
#include "resiprocate/dum/AppDialog.hxx"
#include "resiprocate/dum/AppDialogSet.hxx"
#include "resiprocate/dum/AppDialogSetFactory.hxx"
#include "resiprocate/os/Log.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Random.hxx"

#include <sstream>
#include <time.h>

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;
using namespace std;

class Controller : public InviteSessionHandler
{
   public:
      virtual void onNewSession(ClientInviteSessionHandle, InviteSession::OfferAnswerType oat, 
                                const SipMessage& msg)
      {
      }
      
      virtual void onNewSession(ServerInviteSessionHandle, InviteSession::OfferAnswerType oat, 
                                const SipMessage& msg)
      {
      }

      virtual void onFailure(ClientInviteSessionHandle, const SipMessage& msg)
      {
      }
      
      virtual void onProvisional(ClientInviteSessionHandle h, const SipMessage& msg)
      {
      }

      virtual void onConnected(ClientInviteSessionHandle, const SipMessage& msg)
      {
      }

      virtual void onStaleCallTimeout(ClientInviteSessionHandle)
      {
      }

      virtual void onConnected(InviteSessionHandle, const SipMessage& msg)
      {
      }

      virtual void onRedirected(ClientInviteSessionHandle, const SipMessage& msg)
      {
      }

      virtual void onTerminated(InviteSessionHandle, InviteSessionHandler::TerminatedReason reason, 
                                const SipMessage* msg)
      {
      }

      virtual void onAnswer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
      {
         h->end();         
      }

      virtual void onOffer(InviteSessionHandle is, const SipMessage& msg, const SdpContents& sdp)      
      {
      }

      virtual void onEarlyMedia(ClientInviteSessionHandle, const SipMessage& msg, const SdpContents& sdp)
      {
      }

      virtual void onOfferRequired(InviteSessionHandle, const SipMessage& msg)
      {
      }

      virtual void onOfferRejected(InviteSessionHandle, const SipMessage& msg)
      {
      }

      virtual void onDialogModified(InviteSessionHandle, InviteSession::OfferAnswerType oat, 
                                    const SipMessage& msg)
      {
      }

      virtual void onInfo(InviteSessionHandle, const SipMessage& msg)
      {
      }

      virtual void onRefer(InviteSessionHandle, ServerSubscriptionHandle, const SipMessage& msg)
      {
      }

      virtual void onReferAccepted(InviteSessionHandle, ClientSubscriptionHandle, const SipMessage& msg)
      {
      }

      virtual void onReferRejected(InviteSessionHandle, const SipMessage& msg)
      {
      }

      virtual void onInfoSuccess(InviteSessionHandle, const SipMessage& msg)
      {
      }

      virtual void onInfoFailure(InviteSessionHandle, const SipMessage& msg)
      {
      }

      virtual void onForkDestroyed(ClientInviteSessionHandle)
	  {
	  }
};


int 
main (int argc, char** argv)
{
   Log::initialize(Log::Cout, resip::Log::Warning, argv[0]);

   DialogUsageManager controller;
   controller.addTransport(UDP, 12005);

   MasterProfile uacMasterProfile;      
   auto_ptr<ClientAuthManager> uacAuth(new ClientAuthManager);
   controller.setMasterProfile(&uacMasterProfile);
   controller.setClientAuthManager(uacAuth);

   Controller invSessionHandler;
   controller.setInviteSessionHandler(&invSessionHandler);

   NameAddr controllerAor("sip:controller@internal.xten.net");
   controller.getMasterProfile()->setDefaultFrom(controllerAor);
   NameAddr target("sip:rohan@internal.xten.net");

   Data txt("v=0\r\n"
            "o=- 327064655 327074279 IN IP4 192.168.0.129\r\n"
            "s=3pcc test\r\n"
            "c=IN IP4 0.0.0.0\r\n"
            "t=0 0\r\n"
//            "m=audio 7154 RTP/AVP 0\r\n"
//            "a=sendrecv\r\n"
      );
            
   HeaderFieldValue hfv(txt.data(), txt.size());
   Mime type("application", "sdp");
   SdpContents sdp(&hfv, type);
   
   controller.send(controller.makeInviteSession(target, &sdp));
   
   
   int n = 0;
   while (true)
   {
     FdSet fdset;
     // Should these be buildFdSet on the DUM?
     controller.buildFdSet(fdset);
     int err = fdset.selectMilliSeconds(100);
     assert ( err != -1 );
     controller.process(fdset);
     if (!(n++ % 10)) cerr << "|/-\\"[(n/10)%4] << '\b';
   }   
   return 0;
}
