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

#include <sys/types.h>
#include <iostream>
#include <signal.h>
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

static bool finished = false;

static void
signalHandler(int signo)
{
   std::cerr << "Shutting down" << endl;
   finished = true;
}

int
main(int argc, char* argv[])
{

#ifndef _WIN32
   if ( signal( SIGPIPE, SIG_IGN) == SIG_ERR)
   {
      cerr << "Couldn't install signal handler for SIGPIPE" << endl;
      exit(-1);
   }
#endif

   if ( signal( SIGINT, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGINT" << endl;
      exit( -1 );
   }

   if ( signal( SIGTERM, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGTERM" << endl;
      exit( -1 );
   }

   const char* logType = "cout";
   const char* logLevel = "ALERT";
   const char* proto = "tcp";
   const char* bindAddr = "";

   int seltime = 0;
   
#if defined(HAVE_POPT_H)
   struct poptOption table[] = {
      {"log-type",    'l', POPT_ARG_STRING, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",   'v', POPT_ARG_STRING, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
      { "select-time", 's', POPT_ARG_INT,    &seltime,   0, "number of runs in test", 0},
      {"protocol",    'p', POPT_ARG_STRING, &proto,     0, "protocol to use (tcp | udp)", 0},
      {"bind",        'b', POPT_ARG_STRING, &bindAddr,  0, "interface address to bind to",0},
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);
#endif
   Log::initialize(logType, logLevel, argv[0]);

   SipStack receiver;
   
   int port = 5060;
   receiver.addTransport(UDP, port, V4);
   receiver.addTransport(TCP, port, V4);


   NameAddr contact;
   contact.uri().scheme() = "sip";
   contact.uri().user() = "fluffy";
   contact.uri().host() = bindAddr;
   
   while (!finished)
   {

      try
      {
         receiver.process(seltime);
      }
      catch(BaseException& e)
      {
         CritLog(<< "Uncaught Exception: " << e);
         assert(0);
      }
      
      SipMessage* request = receiver.receive();
      static NameAddr contact;

      if (request)
      {
         if(request->isRequest())
         {
            SipMessage response;
            try
            {
            switch (request->header(h_RequestLine).getMethod())
            {
               case ACK:
                  break;
               default:
                  Helper::makeResponse(response,*request,200,contact);
                  receiver.send(response);
                  break;
            }
            }
            catch(BaseException& e)
            {
               CritLog(<< "Uncaught Exception: " << e );
               assert(0);
            }
         }
         delete request;
      }
   }      
   
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
