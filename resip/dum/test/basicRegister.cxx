#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "resip/stack/SipStack.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "rutil/Time.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Subsystem.hxx"
#include "resip/dum/KeepAliveManager.hxx"

#if defined (USE_SSL)
#if defined(WIN32) 
#include "resip/stack/ssl/WinSecurity.hxx"
#else
#include "resip/stack/ssl/Security.hxx"
#endif
#endif

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;
using namespace std;

class ClientHandler : public ClientRegistrationHandler
{
   public:
      ClientHandler() : done(false) {}

      virtual void onSuccess(ClientRegistrationHandle h, const SipMessage& response)
      {
         InfoLog( << "ClientHandler::onSuccess: " << endl );

         resipCerr << "Pausing before unregister" << endl;
         
         sleepMs(2000);
         h->removeAll();
      }

      virtual void onRemoved(ClientRegistrationHandle, const SipMessage& response)
      {
         InfoLog ( << "ClientHandler::onRemoved ");
         done = true;
      }

      virtual void onFailure(ClientRegistrationHandle, const SipMessage& response)
      {
         InfoLog ( << "ClientHandler::onFailure: " << response );
      }

      virtual int onRequestRetry(ClientRegistrationHandle, int retrySeconds, const SipMessage& response)
      {
         InfoLog ( << "ClientHandler:onRequestRetry");
         return -1;
      }
      
      bool done;
};



int 
main (int argc, char** argv)
{

   if ( argc < 3 ) {
      resipCout << "usage: " << argv[0] << " sip:user passwd\n";
      return 0;
   }

   Log::initialize(Log::Cout, Log::Stack, argv[0]);

   NameAddr userAor(argv[1]);
   Data passwd(argv[2]);

#ifdef USE_SSL
#ifdef WIN32
   Security* security = new WinSecurity;
#else
   Security* security = new Security;
#endif
   SipStack stack(security);
#else
   SipStack stack;
#endif

   DialogUsageManager clientDum(stack);
   SharedPtr<MasterProfile> profile(new MasterProfile);
   auto_ptr<ClientAuthManager> clientAuth(new ClientAuthManager);   
   ClientHandler clientHandler;

   stack.addTransport(UDP, 0, V4);
   // stack.addTransport(UDP, 0, V6);
   stack.addTransport(TCP, 0, V4);
   // stack.addTransport(TCP, 0, V6);
#ifdef USE_SSL
   stack.addTransport(TLS, 0, V4);
   // stack.addTransport(TLS, 0, V6);
#endif
   clientDum.setMasterProfile(profile);
   clientDum.setClientRegistrationHandler(&clientHandler);
   clientDum.setClientAuthManager(clientAuth);
   clientDum.getMasterProfile()->setDefaultRegistrationTime(70);

   // keep alive test.
   auto_ptr<KeepAliveManager> keepAlive(new KeepAliveManager);
   clientDum.setKeepAliveManager(keepAlive);

   clientDum.getMasterProfile()->setDefaultFrom(userAor);
   profile->setDigestCredential(userAor.uri().host(),
                                     userAor.uri().user(),
                                     passwd);

   SharedPtr<SipMessage> regMessage = clientDum.makeRegistration(userAor);
   NameAddr contact;

   clientDum.send( regMessage );

   int n = 0;
   while ( !clientHandler.done )
   {
      stack.process(100);
      while(clientDum.process());

      if (n == 1000) clientHandler.done = true;

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
