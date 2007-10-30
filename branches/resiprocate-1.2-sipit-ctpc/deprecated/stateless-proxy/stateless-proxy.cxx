#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <sys/types.h>
#include <iostream>
#include <memory>

#if defined (HAVE_POPT_H) 
#include <popt.h>
#endif

#include "rutil/Log.hxx"
#include "StatelessProxy.hxx"

using resip::Log;

int
main(int argc, char* argv[])
{
   char* proxyHost=0;
   int proxyPort=5060;
   char* targetHost=0;
   int targetPort=5060;
   char* logType=0;
   char* logLevel=0;
#if defined (HAVE_POPT_H) 
   struct poptOption table[] = {
      {"host", 'h', POPT_ARG_STRING,  &proxyHost, 0, "this hostname", 0},
      {"port", 'p', POPT_ARG_INT,  &proxyPort, 0, "port to listen on", 0},
      {"target", 't', POPT_ARG_STRING,  &targetHost, 0, "target of stateless proxy", 0},
      {"target-port", 'o', POPT_ARG_INT,  &targetPort, 0, "target port of stateless proxy", 0},
      {"log-type", 'l', POPT_ARG_STRING, &logType, 0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level", 'v', POPT_ARG_STRING, &logLevel, 0, "specify the default log level", "LOG_DEBUG|LOG_INFO|LOG_WARNING|LOG_ALERT"},
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);
#endif

   Log::initialize(Log::toType(logType), Log::toLevel(logLevel), argv[0]);

   resip::StatelessProxy proxy(proxyHost, proxyPort, targetHost, targetPort);
   proxy.run();
   proxy.join();
   
#if defined (HAVE_POPT_H) 
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
 * This software consists of voluntary contributions made by PurpleComm,
 * Inc. and many individuals on behalf of PurpleComm, Inc. Inc.
 *
 */
