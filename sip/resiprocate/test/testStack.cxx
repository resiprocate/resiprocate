#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif
#if defined (HAVE_POPT_H) || 1
#include <popt.h>
#else
#warning "will not work very well without libpopt"
#endif
#include <sys/types.h>
#include <iostream>
#include <memory>

#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/Dialog.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{

   char* logType = 0;
   char* logLevel = 0;
   char* proto = "tcp";
   char* bindAddr = 0;

   int runs = 100;
   int window = 10;
   int seltime = 100;
#if defined(HAVE_POPT_H) || 1
   struct poptOption table[] = {
      {"log-type",    'l', POPT_ARG_STRING, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",   'v', POPT_ARG_STRING, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
      {"num-runs",    'r', POPT_ARG_INT,    &runs,      0, "number of calls in test", 0},
      {"window-size", 'w', POPT_ARG_INT,    &window,    0, "number of registrations in test", 0},
      {"select-time", 's', POPT_ARG_INT,    &seltime,   0, "number of runs in test", 0},
      {"protocol",    'p', POPT_ARG_STRING, &proto,   0, "number of runs in test", 0},
      {"bind",        'b', POPT_ARG_STRING, &bindAddr, 0, "interface address to bind to",0},
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);
#endif
   Log::initialize(logType, logLevel, argv[0]);
   
   cout << "Performing " << runs << " runs." << endl;

   SipStack receiver;
   SipStack sender;

   sender.addTransport(UDP, 5060); // !ah! just for debugging TransportSelector
   sender.addTransport(TCP, 5060);
   if (bindAddr)
   {
      sender.addTransport(UDP, 5070,false,bindAddr);
      sender.addTransport(TCP, 5070,false,bindAddr);
   }
   else
   {
      sender.addTransport(UDP, 5070);
      sender.addTransport(TCP, 5070);
   }
   receiver.addTransport(UDP, 5080);
   receiver.addTransport(TCP, 5080);


   NameAddr target;
   target.uri().scheme() = "sip";
   target.uri().user() = "fluffy";
   target.uri().host() = DnsUtil::getLocalHostName();
   target.uri().port() = 5080;
   target.uri().param(p_transport) = proto;
   
   NameAddr from = target;
   from.uri().port() = 5070;
   

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
         SipMessage* next = Helper::makeRegister( target, from, from);
         next->header(h_Vias).front().sentPort() = 5070;
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
      if (request)
      {
         assert(request->isRequest());
         assert(request->header(h_RequestLine).getMethod() == REGISTER);

         SipMessage* response = Helper::makeResponse(*request, 200);
         receiver.send(*response);
         delete response;
         delete request;
      }
      
      SipMessage* response = sender.receive();
      if (response)
      {
         assert(response->isResponse());
         assert(response->header(h_CSeq).method() == REGISTER);
         outstanding--;
         count++;
         delete response;
      }
   }
   InfoLog (<< "Finished " << count << " runs");
   
   UInt64 elapsed = Timer::getTimeMs() - startTime;
   cout <<  runs * ( 1000.0 / (float) elapsed) * ( 1000.0 / (float)Timer::getCpuSpeedMhz() ) 
        << " half calls/s/GHz  ["
        << runs << " calls peformed in " << elapsed << " ms, a rate of " 
        << runs / ((float) elapsed / 1000.0) << " calls per second.]" << endl;
#if defined(HAVE_POPT_H) || 1
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
