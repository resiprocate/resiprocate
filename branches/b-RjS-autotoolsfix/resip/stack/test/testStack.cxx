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

#include <sys/types.h>
#include <iostream>
#include <memory>

#include "rutil/DnsUtil.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/DeprecatedDialog.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/Uri.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{

   char* logType = "cout";
   char* logLevel = "ALERT";
   char* proto = "tcp";
   char* bindAddr = 0;

   int runs = 10000;
   int window = 100;
   int seltime = 0;
   int v6 = 0;
   int invite=0;
   
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
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);
#endif
   Log::initialize(logType, logLevel, argv[0]);
   cout << "Performing " << runs << " runs." << endl;

   IpVersion version = (v6 ? V6 : V4);
   SipStack receiver;
   SipStack sender;
   
//   sender.addTransport(UDP, 25060, version); // !ah! just for debugging TransportSelector
//   sender.addTransport(TCP, 25060, version);

   int senderPort = 25070 + rand()& 0x7fff;   
   if (bindAddr)
   {
      InfoLog(<<"Binding to address: " << bindAddr);
      sender.addTransport(UDP, senderPort, version, StunDisabled, bindAddr);
      sender.addTransport(TCP, senderPort, version, StunDisabled, bindAddr);
   }
   else
   {
      sender.addTransport(UDP, senderPort, version);
      sender.addTransport(TCP, senderPort, version);
   }

   int registrarPort = 25080 + rand()& 0x7fff;   
   receiver.addTransport(UDP, registrarPort, version);
   receiver.addTransport(TCP, registrarPort, version);


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
   int outstanding=0;
   int count = 0;
   int sent = 0;

   while (count < runs)
   {
      //InfoLog (<< "count=" << count << " messages=" << messages.size());
      
      // load up the send window
      while (sent < runs && outstanding < window)
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
         
         next->header(h_Vias).front().sentPort() = senderPort;
         sender.send(*next);
         outstanding++;
         sent++;
         delete next;
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
         assert(request->isRequest());
         SipMessage response;
         switch (request->header(h_RequestLine).getMethod())
         {
            case INVITE:
            {
               DeprecatedDialog dlg(contact);
               dlg.makeResponse(*request, response, 180);
               receiver.send(response);               
               dlg.makeResponse(*request, response, 200);
               receiver.send(response);               
               break;
            }

            case ACK:
               break;

            case BYE:
               Helper::makeResponse(response, *request, 200);
               receiver.send(response);
               break;
               
            case REGISTER:
               Helper::makeResponse(response, *request, 200);
               receiver.send(response);
               break;
            default:
               assert(0);
               break;
         }
         delete request;
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
         
         delete response;
      }
   }
   InfoLog (<< "Finished " << count << " runs");
   
   UInt64 elapsed = Timer::getTimeMs() - startTime;
   if (!invite)
   {
      cout << runs << " registrations peformed in " << elapsed << " ms, a rate of " 
           << runs / ((float) elapsed / 1000.0) << " transactions per second.]" << endl;
   }
   else
   {
      cout << runs << " calls peformed in " << elapsed << " ms, a rate of " 
           << runs / ((float) elapsed / 1000.0) << " calls per second.]" << endl;
   }
   cout << "Note: this test runs both sides (client and server)" << endl;
   
#if defined(HAVE_POPT_H)
   poptFreeContext(context);
#endif
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
