#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/ClientInviteSession.hxx"
#include "resip/dum/ServerInviteSession.hxx"
#include "resip/dum/ClientAuthManager.hxx"

#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/DumShutdownHandler.hxx"

#include "resip/dum/MasterProfile.hxx"
#include "resip/stack/ShutdownMessage.hxx"

#include "resip/stack/SdpContents.hxx"
#include "resip/stack/Pkcs7Contents.hxx"
#include "resip/stack/MultipartSignedContents.hxx"
#include "resip/stack/MultipartAlternativeContents.hxx"
#include "resip/stack/Mime.hxx"

#include "resip/stack/SecurityAttributes.hxx"
#include "resip/stack/Helper.hxx"

#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"

#ifdef WIN32
#include "resip/stack/WinSecurity.hxx"
#endif

#include "TestDumHandlers.hxx"

#include <time.h>
#include <sstream>

using namespace std;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST


class TestSMIMEInviteHandler : public TestClientRegistrationHandler,
                               public TestInviteSessionHandler,
                               public TestDumShutdownHandler
{
   public:
      
      TestSMIMEInviteHandler(Security *sec) : handles(0), security(sec), registered(0), done(0), dumShutDown(0), connected(0), callFailed(false)
      {
      }

      virtual ~TestSMIMEInviteHandler()
      {
      }

      void resetRegistered(void)
      {
         registered = 0;
      }

      bool isConnected(void)
      {
         return (connected == 2);
      }
      
      bool isDumShutDown(void)
      {
         return (dumShutDown == 2);
      }

      bool isRegistered(void)
      {
         return (registered == 2);
      }
      
      bool isDone(void)
      {
         return (done == 2);
         
      }

      bool isCallFailed(void)
      {
         return callFailed;
      }

      SdpContents* generateBody()
      {
         HeaderFieldValue* hfv;      
         Data* txt = new Data("v=0\r\n"
                              "o=1900 369696545 369696545 IN IP4 192.168.2.15\r\n"
                              "s=X-Lite\r\n"
                              "c=IN IP4 192.168.2.15\r\n"
                              "t=0 0\r\n"
                              "m=audio 8000 RTP/AVP 8 3 101\r\n"
                              "a=rtpmap:8 pcma/8000\r\n"
                              "a=rtpmap:3 gsm/8000\r\n"
                              "a=rtpmap:101 telephone-event/8000\r\n"
                              "a=fmtp:101 0-15\r\n");
         
         hfv = new HeaderFieldValue(txt->data(), txt->size());
         SdpContents *sdp = new SdpContents(hfv, Mime("application", "sdp"));
         return sdp;
      }
   

      virtual void onSuccess(ClientRegistrationHandle r,
                             const SipMessage& response)
      {
         InfoLog( << "ClientRegistrationHandler::onSuccess" );
         handles.push_back(r);
         registered++;
      }

      virtual void onConnected(ClientInviteSessionHandle,
                               const SipMessage& msg)
      {
         InfoLog( << "ClientInviteSessionHandler::onConnected" );
         connected++;
      }

      virtual void onFailure(ClientInviteSessionHandle,
                             const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHander::onFailure" );
         callFailed = true;
      }

      virtual void onNewSession(ServerInviteSessionHandle is,
                                InviteSession::OfferAnswerType oat,
                                const SipMessage& msg)
      {
         InfoLog( << "ServerInviteSessionHandler::onNewSession" );
         sis = is;
         is->provisional(180);
      }

      virtual void onConnected(InviteSessionHandle,
                               const SipMessage& msg)
      {
         InfoLog( << "InviteSessionHandler::onConnected()" );
         connected++;
      }

      virtual void onTerminated(InviteSessionHandle,
                                InviteSessionHandler::TerminatedReason reason,
                                const SipMessage* msg)
      {
         InfoLog( << "InviteSessionHandler::onTerminated");
         done++;
      }

      virtual void onOffer(InviteSessionHandle is,
                           const SipMessage& msg,
                           const SdpContents& sdp)
      {
         InfoLog( << "InviteSessionHandler::onOffer" );
         InfoLog( << "Server received SDP: " << sdp );

         const SecurityAttributes *attr = msg.getSecurityAttributes();
         if (attr)
         {
            InfoLog( << *attr );
         }
         else
         {
            InfoLog( << "no Security Attributes" );
         }

         if (sis.isValid())
         {
            NameAddr fromAor(msg.header(h_From).uri());
            NameAddr toAor(msg.header(h_To).uri());
            is->provideAnswer(*generateBody());
            sis->accept();
         }
      }
      
      virtual void onAnswer(InviteSessionHandle,
                            const SipMessage& msg,
                            const SdpContents& sdp,
                            InviteSessionHandler::AnswerReason reason)
      {
         InfoLog( << "InviteSessionHandler::onAnswer");
         InfoLog( << "Client received SDP: " << sdp );

         const SecurityAttributes *attr = msg.getSecurityAttributes();
         if (attr)
         {
            InfoLog( << *attr );
         }
         else
         {
            InfoLog( << "no Security Attributes" );
         }
         
      }

      virtual void onDumCanBeDeleted() 
      {
         InfoLog( << "DumShutDownHandler::onDumCanBeDeleted" );
         dumShutDown++;
      }

      virtual void onReferNoSub(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(<< "InviteSessionHandler::onReferNoSub(): " << msg.brief());
      }

   public:
      std::vector<ClientRegistrationHandle> handles;
      ServerInviteSessionHandle sis;

   private:
      Security *security;
      int registered;
      int done;
      int dumShutDown;
      int connected;
      bool callFailed;
      
};


int 
main (int argc, char** argv)
{
   if ( argc < 5 ) {
      cout << "usage: " << argv[0] << " sip:user1 passwd1 sip:user2 passwd2" << endl;
      return 1;
   }

   Log::initialize(Log::Cout, Log::Debug, argv[0]);

   NameAddr clientAor(argv[1]);
   Data clientPasswd(argv[2]);
   NameAddr serverAor(argv[3]);
   Data serverPasswd(argv[4]);

#ifdef WIN32
   Security* security = new WinSecurity;
#else
   Security* security = new Security;
#endif

   TestSMIMEInviteHandler handler(security);

   // set up UAC
   SipStack clientStack(security);
   DialogUsageManager clientDum(clientStack);
   srand(time(NULL));
   clientDum.addTransport(UDP, 10000 + rand()&0x7fff, V4);
   clientDum.addTransport(TCP, 10000 + rand()&0x7fff, V4);
   clientDum.addTransport(TLS, 10000 + rand()&0x7fff, V4);
#ifdef USE_IPV6
   clientDum.addTransport(UDP, 10000 + rand()&0x7fff, V6);
   clientDum.addTransport(TCP, 10000 + rand()&0x7fff, V6);
   clientDum.addTransport(TLS, 10000 + rand()&0x7fff, V6);
#endif

   SharedPtr<MasterProfile> clientProfile(new MasterProfile);   
   auto_ptr<ClientAuthManager> clientAuth(new ClientAuthManager());   

   clientDum.setClientAuthManager(clientAuth);
   clientDum.setClientRegistrationHandler(&handler);
   clientDum.setInviteSessionHandler(&handler);

   clientProfile->setDefaultFrom(clientAor);
   clientProfile->setDigestCredential(clientAor.uri().host(),clientAor.uri().user(),clientPasswd);
   clientProfile->setDefaultRegistrationTime(60);
   clientProfile->addSupportedMethod(INVITE);
   clientProfile->addSupportedMimeType(INVITE, Mime("application", "pkcs7-mime"));
   clientProfile->addSupportedMimeType(INVITE, Mime("multipart", "signed"));
   clientProfile->addSupportedMimeType(INVITE, Mime("multipart", "alternative"));
   clientDum.setMasterProfile(clientProfile);

   //set up UAS
   SipStack serverStack(security);
   DialogUsageManager serverDum(serverStack);
   //serverDum.addTransport(UDP, 10000 + rand()&0x7fff, V4);
   serverDum.addTransport(TCP, 10000 + rand()&0x7fff, V4);
   //serverDum.addTransport(TLS, 10000 + rand()&0x7fff, V4);

   SharedPtr<MasterProfile> serverProfile(new MasterProfile);
   std::auto_ptr<ClientAuthManager> serverAuth(new ClientAuthManager);

   serverDum.setClientAuthManager(serverAuth);
   serverDum.setClientRegistrationHandler(&handler);
   serverDum.setInviteSessionHandler(&handler);

   serverProfile->setDefaultFrom(serverAor);
   serverProfile->setDigestCredential(serverAor.uri().host(),serverAor.uri().user(),serverPasswd);
   serverProfile->setDefaultRegistrationTime(60);
   serverProfile->addSupportedMethod(INVITE);
   serverProfile->addSupportedMimeType(INVITE, Mime("application", "pkcs7-mime"));
   serverProfile->addSupportedMimeType(INVITE, Mime("multipart", "signed"));
   serverProfile->addSupportedMimeType(INVITE, Mime("multipart", "alternative"));
   serverDum.setMasterProfile(serverProfile);

   enum
      {
         Registering,
         Inviting,
         Waiting,
         HangingUp,
         Unregistering,
         ShuttingDown,
         Finished
      } state;
   time_t endTime;

   // register client and server
   SharedPtr<SipMessage> clientRegMessage = clientDum.makeRegistration(clientAor);
   clientDum.send(clientRegMessage);
   SharedPtr<SipMessage> serverRegMessage = serverDum.makeRegistration(serverAor);
   serverDum.send(serverRegMessage);
   state = Registering;

   while (state != Finished)
   {
      FdSet fdset;

      clientStack.buildFdSet(fdset);
      serverStack.buildFdSet(fdset);
      
      int err = fdset.selectMilliSeconds(resipMin((int)clientStack.getTimeTillNextProcessMS(), 50));
      assert ( err != -1 );

      clientStack.process(fdset);
      serverStack.process(fdset);
      while(clientDum.process() || serverDum.process());

      switch (state)
      {
         case Registering:
         {
            if (handler.isRegistered())
            {
               InfoLog( << "Sending INVITE request" );
               clientDum.send(clientDum.makeInviteSession(serverAor,
                                                          handler.generateBody()));
               state = Inviting;
            }
            break;
         }
         
         case Inviting:
         {
            if (handler.isConnected())
            {
               InfoLog( << "Starting timer, waiting for 5 seconds" );
               endTime = time(NULL) + 5;
               state = Waiting;
            }
            break;
         }
         
         case Waiting:
         {
            if (handler.isCallFailed())
            {
               InfoLog( << "Call Failed" );
               for (std::vector<ClientRegistrationHandle>::iterator it = handler.handles.begin();
                    it != handler.handles.end(); it++)
               {
                  (*it)->end();
               }
               state = Unregistering;
            }

            if (time(NULL) > endTime)
            {
               InfoLog( << "Timer expired, hanging up" );
               handler.sis->end();
               state = HangingUp;
            }
            break;
         }

         case HangingUp:
         {
            if (handler.isDone())
            {
               for (std::vector<ClientRegistrationHandle>::iterator it = handler.handles.begin();
                       it != handler.handles.end(); it++)
               {
                  (*it)->end();
               }
               state = Unregistering;
            }
            break;
         }
         
         case Unregistering:
         {
            if (handler.isRegistered())
            {
               InfoLog( << "Shutting down" );
               serverDum.shutdown(&handler);
               clientDum.shutdown(&handler);
               state = ShuttingDown;
            }
            break;
         }
         
         case ShuttingDown:
         {
            if (handler.isDumShutDown()) 
            {
               InfoLog( << "Finished" );
               state = Finished;
            }
            
            break;
         }

         default:
         {
            InfoLog( << "Unrecognised state" );
            assert(0);
         }
      }
      
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
