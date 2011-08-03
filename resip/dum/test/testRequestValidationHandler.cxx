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
#include "resip/dum/RequestValidationHandler.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/ServerInviteSession.hxx"
#include "resip/dum/ServerOutOfDialogReq.hxx"
#include "resip/dum/OutOfDialogHandler.hxx"
#include "resip/dum/AppDialog.hxx"
#include "resip/dum/AppDialogSet.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "rutil/WinLeakCheck.hxx"

#include <sstream>
#include <time.h>

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;
using namespace std;

class TestInviteSessionHandler : public InviteSessionHandler, public RequestValidationHandler
{
   public:
      Data name;
      bool registered;
      ClientRegistrationHandle registerHandle;

      int mInvalidMethod, mInvalidScheme, mInvalidRequiredOptions, m100RelNotSupportedByRemote, mInvalidContentType, mInvalidContentEncoding;
      int mInvalidContentLanguage, mInvalidAccept;


      TestInviteSessionHandler(const Data& n) : name(n), registered(false) 
      {
         mInvalidMethod = 0;
         mInvalidScheme = 0;
         mInvalidRequiredOptions = 0;
         m100RelNotSupportedByRemote = 0;
         mInvalidContentType = 0;
         mInvalidContentEncoding = 0;
         mInvalidContentLanguage = 0;
         mInvalidAccept = 0;
      }

      virtual ~TestInviteSessionHandler()
      {
      }
      
      virtual void onNewSession(ClientInviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)
      {
         cout << name << ": ClientInviteSession-onNewSession - " << msg.brief() << endl;
      }
      
      virtual void onNewSession(ServerInviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)
      {
         cout << name << ": ServerInviteSession-onNewSession - " << msg.brief() << endl;
      }

      virtual void onFailure(ClientInviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": ClientInviteSession-onFailure - " << msg.brief() << endl;
      }
      
      virtual void onProvisional(ClientInviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": ClientInviteSession-onProvisional - " << msg.brief() << endl;
      }

      virtual void onConnected(ClientInviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": ClientInviteSession-onConnected - " << msg.brief() << endl;
      }

      virtual void onStaleCallTimeout(ClientInviteSessionHandle handle)
      {
         cout << name << ": ClientInviteSession-onStaleCallTimeout" << endl;
      }

      virtual void onConnected(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onConnected - " << msg.brief() << endl;
      }

      virtual void onRedirected(ClientInviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": ClientInviteSession-onRedirected - " << msg.brief() << endl;
      }

      virtual void onTerminated(InviteSessionHandle, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg)
      {
         cout << name << ": InviteSession-onTerminated - " << msg->brief() << endl;
      }

      virtual void onAnswer(InviteSessionHandle, const SipMessage& msg, const SdpContents& sdp)
      {
         cout << name << ": InviteSession-onAnswer(SDP)" << endl;
      }

      virtual void onOffer(InviteSessionHandle is, const SipMessage& msg, const SdpContents& sdp)      
      {
         cout << name << ": InviteSession-onOffer(SDP)" << endl;
      }

      virtual void onEarlyMedia(ClientInviteSessionHandle, const SipMessage& msg, const SdpContents& sdp)
      {
         cout << name << ": InviteSession-onEarlyMedia(SDP)" << endl;
      }

      virtual void onOfferRequired(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onOfferRequired - " << msg.brief() << endl;
      }

      virtual void onOfferRejected(InviteSessionHandle, const SipMessage* msg)
      {
         cout << name << ": InviteSession-onOfferRejected" << endl;
      }

      virtual void onRefer(InviteSessionHandle, ServerSubscriptionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onRefer - " << msg.brief() << endl;
      }

      virtual void onReferAccepted(InviteSessionHandle, ClientSubscriptionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onReferAccepted - " << msg.brief() << endl;
      }

      virtual void onReferRejected(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onReferRejected - " << msg.brief() << endl;
      }

      virtual void onReferNoSub(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onReferNoSub - " << msg.brief() << endl;
      }

      virtual void onInfo(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onInfo - " << msg.brief() << endl;
      }

      virtual void onInfoSuccess(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onInfoSuccess - " << msg.brief() << endl;
      }

      virtual void onInfoFailure(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onInfoFailure - " << msg.brief() << endl;
      }

      virtual void onMessage(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onMessage - " << msg.brief() << endl;
      }

      virtual void onMessageSuccess(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onMessageSuccess - " << msg.brief() << endl;
      }

      virtual void onMessageFailure(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onMessageFailure - " << msg.brief() << endl;
      }

      virtual void onForkDestroyed(ClientInviteSessionHandle)
      {
         cout << name << ": ClientInviteSession-onForkDestroyed" << endl;
      }

      void onInvalidMethod(const resip::SipMessage&)
      {
         cout << name << ": onInvalidMethod" << endl;
         mInvalidMethod++;
      }

      void onInvalidScheme(const resip::SipMessage&)
      {
         cout << name << ": onInvalidSchema" << endl;
         mInvalidScheme++;
      }

      void onInvalidRequiredOptions(const resip::SipMessage&)
      {
         cout << name << ": onInvalidRequiredOptions" << endl;
         mInvalidRequiredOptions++;
      }

      void on100RelNotSupportedByRemote(const resip::SipMessage&)
      {
         cout << name << ": on100RelNotSupportedByRemote" << endl;
         m100RelNotSupportedByRemote++;
      }

      void onInvalidContentType(const resip::SipMessage&)
      {
         cout << name << ": onInvalidContentType" << endl;
         mInvalidContentType++;
      }

      void onInvalidContentEncoding(const resip::SipMessage&)
      {
         cout << name << ": onInvalidContentEncoding" << endl;
         mInvalidContentEncoding++;
      }

      void onInvalidContentLanguage(const resip::SipMessage&)
      {
         cout << name << ": onInvalidContentLanguage" << endl;
         mInvalidContentLanguage++;
      }

      void onInvalidAccept(const resip::SipMessage&)
      {
         cout << name << ": onInvalidAccept" << endl;
         mInvalidAccept++;
      }

};

int 
main (int argc, char** argv)
{
   Log::initialize(Log::Cout, resip::Log::Debug, argv[0]);

   NameAddr uacAor("sip:127.0.0.1:17298");
   NameAddr uasAor("sip:127.0.0.1:17299");

   SdpContents* dummySdp;
   HeaderFieldValue* hfv;
   Data* txt;

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
   dummySdp = new SdpContents(*hfv, type);

   //set up UAC
   SipStack stackUac;
   DialogUsageManager* dumUac = new DialogUsageManager(stackUac);
   dumUac->addTransport(UDP, 17298);
   dumUac->addTransport(TCP, 17298);

   SharedPtr<MasterProfile> uacMasterProfile(new MasterProfile);
   auto_ptr<ClientAuthManager> uacAuth(new ClientAuthManager);
   dumUac->setMasterProfile(uacMasterProfile);
   dumUac->setClientAuthManager(uacAuth);

   TestInviteSessionHandler uac("UAC");
   dumUac->setInviteSessionHandler(&uac);
   dumUac->setRequestValidationHandler(&uac);

   dumUac->getMasterProfile()->setDefaultFrom(uacAor);

   //set up UAS
   SipStack stackUas;
   DialogUsageManager* dumUas = new DialogUsageManager(stackUas);
   dumUas->addTransport(UDP, 17299);
   dumUas->addTransport(TCP, 17299);
   
   SharedPtr<MasterProfile> uasMasterProfile(new MasterProfile);
   std::auto_ptr<ClientAuthManager> uasAuth(new ClientAuthManager);
   dumUas->setMasterProfile(uasMasterProfile);
   dumUas->setClientAuthManager(uasAuth);

   dumUas->getMasterProfile()->setDefaultFrom(uasAor);

   TestInviteSessionHandler uas("UAS");
   dumUas->setInviteSessionHandler(&uas);
   dumUas->setRequestValidationHandler(&uas);

   // First test: invalid method
   dumUac->send(dumUac->makeOutOfDialogRequest(uasAor, MESSAGE));

   // Second test: invalid scheme
   SharedPtr<SipMessage> invalidSchemeMsg = dumUac->makeOutOfDialogRequest(uasAor, OPTIONS);
   invalidSchemeMsg->header(h_RequestLine).uri().scheme() = "tel";
   dumUac->send(invalidSchemeMsg);

   // !fjoanis! TODO: Add more tests

   assert(uas.mInvalidMethod == 0);
   assert(uas.mInvalidScheme == 0);
   assert(uas.mInvalidRequiredOptions == 0);
   assert(uas.m100RelNotSupportedByRemote == 0);
   assert(uas.mInvalidContentType == 0);
   assert(uas.mInvalidContentEncoding == 0);
   assert(uas.mInvalidContentLanguage == 0);
   assert(uas.mInvalidAccept == 0);

   assert(uac.mInvalidMethod == 0);
   assert(uac.mInvalidScheme == 0);
   assert(uac.mInvalidRequiredOptions == 0);
   assert(uac.m100RelNotSupportedByRemote == 0);
   assert(uac.mInvalidContentType == 0);
   assert(uac.mInvalidContentEncoding == 0);
   assert(uac.mInvalidContentLanguage == 0);
   assert(uac.mInvalidAccept == 0);  
 
   // Now give them some cycles
   for(int i=0;i<10;i++)
   {
     {
        stackUac.process(50);
        while(dumUac->process());
     }
     {
        stackUas.process(50);
        while(dumUas->process());
     }
   }

   assert(uas.mInvalidMethod == 1);
   assert(uas.mInvalidScheme == 1);
   assert(uas.mInvalidRequiredOptions == 0);
   assert(uas.m100RelNotSupportedByRemote == 0);
   assert(uas.mInvalidContentType == 0);
   assert(uas.mInvalidContentEncoding == 0);
   assert(uas.mInvalidContentLanguage == 0);
   assert(uas.mInvalidAccept == 0);

   assert(uac.mInvalidMethod == 0);
   assert(uac.mInvalidScheme == 0);
   assert(uac.mInvalidRequiredOptions == 0);
   assert(uac.m100RelNotSupportedByRemote == 0);
   assert(uac.mInvalidContentType == 0);
   assert(uac.mInvalidContentEncoding == 0);
   assert(uac.mInvalidContentLanguage == 0);
   assert(uac.mInvalidAccept == 0);

   delete dumUac; 
   delete dumUas;
   delete dummySdp;
   delete txt;
   delete hfv;

   cout << "!!!!!!!!!!!!!!!!!! Successful !!!!!!!!!! " << endl;
#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   }
#endif

}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
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
