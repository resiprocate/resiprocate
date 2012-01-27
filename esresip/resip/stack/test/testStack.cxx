/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#if defined (HAVE_POPT_H) 
#include <popt.h>
#else
#ifndef WIN32
#warning "will not work very well without libpopt"
#endif
#endif

#include "resip/stack/ApiCheckList.hxx"
#include <sys/types.h>
#include <iostream>
#include <memory>

#ifdef RESIP_USE_SCTP
#include <netinet/sctp.h>
#endif

#include "rutil/DnsUtil.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/EsLogger.hxx"
#include "resip/stack/DeprecatedDialog.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/TransportThread.hxx"
#include "resip/stack/Uri.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

int
main(int argc, char* argv[])
{

   const char* logType = "cout";
   const char* logLevel = "ALERT";
   const char* proto = "tcp";
   const char* bindAddr = "127.0.0.1";

   int runs = 50000;
   int window = 100;
   int seltime = 0;
   int v6 = 0;
   int invite=0;
   int singleThreaded=0;
   unsigned int loopDepth=0;
   unsigned int extraVias=0;
   int quiet=0;
   
#if defined(HAVE_POPT_H)
   struct poptOption table[] = {
      {"log-type",    'l', POPT_ARG_STRING, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",   'v', POPT_ARG_STRING, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
      {"num-runs",    'r', POPT_ARG_INT,    &runs,      0, "number of calls in test", 0},
      {"window-size", 'w', POPT_ARG_INT,    &window,    0, "number of concurrent transactions", 0},
      { "select-time", 's', POPT_ARG_INT,    &seltime,   0, "number of runs in test", 0},
      {"protocol",    'p', POPT_ARG_STRING, &proto,     0, "protocol to use (tcp | udp)", 0},
      {"bind",        'b', POPT_ARG_STRING, &bindAddr,  0, "interface address to bind to",0},
      {"v6",          '6', POPT_ARG_NONE,   &v6     ,   0, "ipv6", 0},
      {"invite",      'i', POPT_ARG_NONE,   &invite     ,   0, "send INVITE/BYE instead of REGISTER", 0},
      {"single-threaded",      'u', POPT_ARG_NONE,   &singleThreaded     ,   0, "run in old single-threaded mode", 0},
      {"loop-depth",      'd', POPT_ARG_INT,   &loopDepth     ,   0, "allow requests to loop a given number of times", 0},
      {"extra-vias",      'e', POPT_ARG_INT,   &extraVias     ,   0, "add extra Via headers to the request", 0},
      {"quiet",      'q', POPT_ARG_NONE,   &quiet     ,   0, "minimal output (only write transactions per second)", 0},
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);
#endif
   if(loopDepth>70)
   {
      loopDepth=70;
   }
   estacado::EsLogger::init("", argc, argv, log4cplus::WARN_LOG_LEVEL);
   if(!quiet)
   {
      cout << "Performing " << runs << " runs." << endl;
   }

   IpVersion version = (v6 ? V6 : V4);
   SipStack receiver(0, (singleThreaded==0));
   SipStack sender(0,(singleThreaded==0));
   
//   sender.addTransport(UDP, 25060, version); // !ah! just for debugging TransportSelector
//   sender.addTransport(TCP, 25060, version);

   int senderPort = 25070 + (rand()& 0x7fff);   
   std::auto_ptr<TransportThread> udpClientThread;
   std::auto_ptr<TransportThread> tcpClientThread;
   std::auto_ptr<TransportThread> sctpClientThread;
   std::auto_ptr<TransportThread> udpServerThread;
   std::auto_ptr<TransportThread> tcpServerThread;
   std::auto_ptr<TransportThread> sctpServerThread;
   if (bindAddr)
   {
      InfoLog(<<"Binding to address: " << bindAddr);
      udpClientThread.reset(new TransportThread(*sender.addTransport(UDP, senderPort, version, StunDisabled, bindAddr, resip::Data::Empty, resip::Data::Empty, SecurityTypes::TLSv1, 0, (singleThreaded==0))));
      tcpClientThread.reset(new TransportThread(*sender.addTransport(TCP, senderPort, version, StunDisabled, bindAddr, resip::Data::Empty, resip::Data::Empty, SecurityTypes::TLSv1, 0, (singleThreaded==0))));
#ifdef HAVE_SCTP
      sctpClientThread.reset(new TransportThread(*sender.addTransport(SCTP, senderPort, version, StunDisabled, bindAddr, resip::Data::Empty, resip::Data::Empty, SecurityTypes::TLSv1, 0, (singleThreaded==0))));
#endif
   }
   else
   {
      udpClientThread.reset(new TransportThread(*sender.addTransport(UDP, senderPort, version, StunDisabled, resip::Data::Empty, resip::Data::Empty, resip::Data::Empty, SecurityTypes::TLSv1, 0, (singleThreaded==0))));
      tcpClientThread.reset(new TransportThread(*sender.addTransport(TCP, senderPort, version, StunDisabled, resip::Data::Empty, resip::Data::Empty, resip::Data::Empty, SecurityTypes::TLSv1, 0, (singleThreaded==0))));
#ifdef HAVE_SCTP
      sctpClientThread.reset(new TransportThread(*sender.addTransport(SCTP, senderPort, version, StunDisabled, resip::Data::Empty, resip::Data::Empty, resip::Data::Empty, SecurityTypes::TLSv1, 0, (singleThreaded==0))));
#endif
   }

   int registrarPort = 25080 + (rand()& 0x7fff);   
   udpServerThread.reset(new TransportThread(*receiver.addTransport(UDP, registrarPort, version, StunDisabled, bindAddr, resip::Data::Empty, resip::Data::Empty, SecurityTypes::TLSv1, 0, (singleThreaded==0))));
   tcpServerThread.reset(new TransportThread(*receiver.addTransport(TCP, registrarPort, version, StunDisabled, bindAddr, resip::Data::Empty, resip::Data::Empty, SecurityTypes::TLSv1, 0, (singleThreaded==0))));
#ifdef HAVE_SCTP
   sctpServerThread.reset(new TransportThread(*receiver.addTransport(SCTP, registrarPort, version, StunDisabled, bindAddr, resip::Data::Empty, resip::Data::Empty, SecurityTypes::TLSv1, 0, (singleThreaded==0))));
#endif


   NameAddr target;
   target.uri().scheme() = "sip";
   target.uri().user() = "fluffy";
   target.uri().host() = bindAddr ? bindAddr :DnsUtil::getLocalHostName();
   target.uri().port() = registrarPort;
   target.uri().param(p_transport) = proto;
  
   NameAddr contact;
   contact.uri().scheme() = "sip";
   contact.uri().user() = "fluffy";

#ifdef WIN32
     target.uri().host() = Data("127.0.0.1");
#endif

   NameAddr from = target;
   from.uri().port() = senderPort;
   
   UInt64 startTime = Timer::getTimeMs();
   UInt64 lastTime = startTime;
   int outstanding=0;
   int count = 0;
   int sent = 0;
   WarningLog(<< "Total runs/total time:last x runs/time");

   if(singleThreaded==0)
   {
      udpClientThread->run();
      tcpClientThread->run();
      udpServerThread->run();
      tcpServerThread->run();
#ifdef HAVE_SCTP
      sctpClientThread->run();
      sctpServerThread->run();
#endif
   }
   sender.run();
   receiver.run();

   int nextLog=1000;
   while (count < runs)
   {
      int smallCount=100;
      // load up the send window
      while (sent < runs && outstanding < window && smallCount!=0)
      {
         DebugLog (<< "Sending " << count << " / " << runs << " (" << outstanding << ")");
         target.uri().port() = registrarPort; // +(sent%window);

         SipMessage* next=0;
         if (invite)
         {
            next = Helper::makeInvite( target, from, contact);            
         }
         else
         {
            next = Helper::makeRegister( target, from, contact);
         }
         for(unsigned int v=0; v<extraVias; ++v)
         {
            next->header(h_Vias).push_back(Via());
         }
         next->header(h_Vias).front().sentPort() = senderPort;
         sender.send(std::auto_ptr<SipMessage>(next));
         outstanding++;
         sent++;
         smallCount--;
      }

      FdSet fdset; 
      receiver.buildFdSet(fdset);
      sender.buildFdSet(fdset);
      fdset.selectMilliSeconds(seltime); 
      receiver.process(fdset);
      sender.process(fdset);
      
      SipMessage* request = receiver.receive();
      static NameAddr contact;

      if (request)
      {
         if(request->isRequest())
         {
            switch (request->header(h_RequestLine).getMethod())
            {
               case INVITE:
               if(loopDepth==0 ||
                     request->header(h_MaxForwards).value() == (70-loopDepth))
               {
                  std::auto_ptr<SipMessage> response(new SipMessage);
                  DeprecatedDialog dlg(contact);
                  dlg.makeResponse(*response, *request, 180);
                  receiver.send(response);
                  response.reset(new SipMessage);
                  dlg.makeResponse(*response, *request, 200);
                  receiver.send(response);               
               }
               else
               {
                  --(request->header(h_MaxForwards).value());
                  request->header(h_Vias).push_front(Via());
                  receiver.send(std::auto_ptr<SipMessage>(request));
                  request=0;
               }
               break;
   
               case ACK:
                  break;
   
               case BYE:
                  if(loopDepth==0 ||
                     request->header(h_MaxForwards).value() == (70-loopDepth))
                  {
                  Helper::makeInPlaceResponse(*request, 200);
                  std::auto_ptr<SipMessage> response(request);
                  request=0;
                  receiver.send(response);
                  }
                  else
                  {
                     --(request->header(h_MaxForwards).value());
                     request->header(h_Vias).push_front(Via());
                     receiver.send(std::auto_ptr<SipMessage>(request));
                     request=0;
                  }
                  break;
                  
               case REGISTER:
                  if(loopDepth==0 ||
                     request->header(h_MaxForwards).value() == (70-loopDepth))
                  {
                     Helper::makeInPlaceResponse(*request, 200);
                     std::auto_ptr<SipMessage> response(request);
                     request=0;
                     receiver.send(response);
                  }
                  else
                  {
                     --(request->header(h_MaxForwards).value());
                     request->header(h_Vias).push_front(Via());
                     receiver.send(std::auto_ptr<SipMessage>(request));
                     request=0;
                  }
                  break;
               default:
                  assert(0);
                  break;
            }
            delete request;
         }
         else
         {
            assert(loopDepth);
            std::auto_ptr<SipMessage> response(request);
            response->header(h_Vias).pop_front();
            receiver.send(response);
         }
      }
      
      SipMessage* response = sender.receive();
      if (response)
      {
         assert(response->isResponse());
         switch(response->header(h_CSeq).method())
         {
            case REGISTER:
               outstanding--;
               count++;
               break;
               
            case INVITE:
               if (response->header(h_StatusLine).statusCode() == 200)
               {
                  outstanding--;
                  count++;

                  DeprecatedDialog dlg(contact);
                  dlg.createDialogAsUAC(*response);
                  SipMessage* ack = dlg.makeAck();
                  sender.send(*ack);
                  delete ack;

                  SipMessage* bye = dlg.makeBye();
                  sender.send(*bye);
                  delete bye;
               }
               break;

            case BYE:
               break;
               
            default:
               assert(0);
               break;
         }

         if(count==nextLog)
         {
            UInt64 time=Timer::getTimeMs();
            WarningLog(<< count << "/" << time - startTime<< ":" << 1000 << "/" << time-lastTime);
            lastTime=time;
            nextLog=count+1000;
         }
         
         delete response;
      }
   }
   InfoLog (<< "Finished " << count << " runs");
   
   UInt64 elapsed = Timer::getTimeMs() - startTime;
   if (!invite)
   {
      if(quiet)
      {
         cout << runs / ((float) elapsed / 1000.0) << endl;
      }
      else
      {
         cout << runs << " registrations peformed in " << elapsed << " ms, a rate of " 
              << runs / ((float) elapsed / 1000.0) << " transactions per second.]" << endl;
      }
   }
   else
   {
      if(quiet)
      {
         cout << runs / ((float) elapsed / 1000.0) << endl;
      }
      else
      {
         cout << runs << " calls peformed in " << elapsed << " ms, a rate of " 
              << runs / ((float) elapsed / 1000.0) << " calls per second.]" << endl;
      }
   }
   
   if(!quiet)
   {
      cout << "Note: this test runs both sides (client and server)" << endl;
   }
   
#if defined(HAVE_POPT_H)
   poptFreeContext(context);
#endif

   if(singleThreaded==0)
   {
      udpClientThread->shutdown();
      tcpClientThread->shutdown();
      udpServerThread->shutdown();
      tcpServerThread->shutdown();
#ifdef HAVE_SCTP
      sctpClientThread->shutdown();
      sctpServerThread->shutdown();
#endif

      udpClientThread->join();
      tcpClientThread->join();
      udpServerThread->join();
      tcpServerThread->join();
#ifdef HAVE_SCTP
      sctpClientThread->join();
      sctpServerThread->join();
#endif
   }
   return 0;
}
/* Copyright 2007 Estacado Systems */

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
