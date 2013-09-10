#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#if defined (HAVE_POPT_H) 
#include <popt.h>
#else
#ifndef WIN32
#warning "will not work very well without libpopt"
#endif
#endif

#include <iostream>

#include "resip/stack/SipStack.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/DataStream.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{
   const char* logType = "cout";
   const char* logLevel = "DEBUG";
   int optLoop = 0;
   int optPort = 0;
   int optTcp = 0;
   int optUdp = 0;
   char *optBindAddr = 0;
   char *optBindTcpAddr = 0;
   char *optBindUdpAddr = 0;

#if defined(HAVE_POPT_H)
   struct poptOption table[] = {
      {"log-type",    'l', POPT_ARG_STRING, &logType,   0,
       "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",   'v', POPT_ARG_STRING, &logLevel,  0,
       "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
      {"loop",    'l', POPT_ARG_NONE,    &optLoop,      0, "loop endlessly", 0 },
      {"port", 'p', POPT_ARG_INT, &optPort, 0, "port to listen on", 0},
      {"tcp", 't', POPT_ARG_NONE, &optTcp, 0, "do not use TCP", 0},
      {"udp", 'u', POPT_ARG_NONE, &optUdp, 0, "do not use UDP", 0},
      {"bind", 'b', POPT_ARG_STRING, &optBindAddr, 0, "bind to this address globally", 0},
      {"bind-tcp", 0, POPT_ARG_STRING, &optBindTcpAddr, 0, "bind TCP Transport to this address", 0},
      {"bind-udp", 0, POPT_ARG_STRING, &optBindUdpAddr, 0, "bind UDP Transport to this address", 0},
      POPT_AUTOHELP
      { 0 }
   };
   
   poptContext context = poptGetContext(0, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);
#endif

  Log::initialize(logType, logLevel, argv[0]);

   optUdp = !optUdp;
   optTcp = !optTcp;
   if (!optPort) optPort = 5060;


   
   auto_ptr<SipStack> stack(new SipStack());
   if (optBindAddr)
   {
     const char *addr = optBindUdpAddr?optBindUdpAddr:optBindAddr;
     stack->addTransport(UDP, optPort, DnsUtil::isIpV6Address(addr)?V6:V4, StunDisabled, addr);
     addr = optBindTcpAddr?optBindTcpAddr:optBindAddr;
     stack->addTransport(TCP, optPort, DnsUtil::isIpV6Address(addr)?V6:V4, StunDisabled, addr);
   }
   else
   {
     stack->addTransport(UDP, optPort);
     stack->addTransport(TCP, optPort);
   }

   int count=1;
   bool needToProcessSend = false;
   while (optLoop || count > 0 || needToProcessSend)
   {
      FdSet fdset; 
      stack->buildFdSet(fdset);

      stack->process(fdset);
      needToProcessSend = false;

      Message* msg = stack->receive();

      if (msg)
      {
         SipMessage* received = dynamic_cast<SipMessage*>(msg);
         if (received)
         {
           --count;
           received->encode(resipCout);
           if (received->isRequest())
           {
             SipMessage* resp = Helper::makeResponse(*received, 606);
             WarningCategory warn;
             warn.text() = "Simple test reply from "__FILE__" driver.";
             resp->header(h_Warnings).push_back(warn);
             stack->send(*resp);
             needToProcessSend = true;
           }
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
