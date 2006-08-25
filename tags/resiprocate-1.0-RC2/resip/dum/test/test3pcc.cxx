#include "resip/stack/SdpContents.hxx"
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

   SharedPtr<MasterProfile> uacMasterProfile(new MasterProfile);      
   auto_ptr<ClientAuthManager> uacAuth(new ClientAuthManager);
   controller.setMasterProfile(uacMasterProfile);
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
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
