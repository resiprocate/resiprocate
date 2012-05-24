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
#include <unistd.h> // for sleep 
#include <iostream>
#include <memory>
#include <sys/ioctl.h>
#include <signal.h>

#ifdef __MINGW32__
#define sleep(x) Sleep((x)*1000)
#endif

#include "rutil/DnsUtil.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/ShutdownMessage.hxx"

#include "resip/stack/ssl/Security.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{
#ifdef USE_SSL
   const char *logType = "cerr";
   const char *logLevel = "DEBUG";
   const char *certPath = ".";
   int  clientPort = 15001;
   int  serverPort = 5061;
   const char *clientDomain = "example.net";
   const char *serverDomain = "example.com";

#if defined(HAVE_POPT_H)
   struct poptOption table[] = {
      {"log-type",     'l', POPT_ARG_STRING, &logType,       0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",    'v', POPT_ARG_STRING, &logLevel,      0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
      {"cert-path",    'p', POPT_ARG_STRING, &certPath,      0, "path to the server certificates", "."},
      {"client-port",  'f', POPT_ARG_INT,    &clientPort,    0, "client port (send request from)", "15001"},
      {"server-port",  't', POPT_ARG_INT,    &serverPort,    0, "server port (send request to)", "5061"},
      {"client-domain",'c', POPT_ARG_STRING, &clientDomain,  0, "client domain", "example.net"},
      {"server-domain",'s', POPT_ARG_STRING, &serverDomain,  0, "server domain", "example.com"},
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);
#endif

   Log::initialize(logType, logLevel, argv[0]);

   DebugLog(<< "Logs initialized");
   DebugLog(<< "logLevel = " << logLevel);
   DebugLog(<< "certPath = " << certPath);

   DebugLog(<< "Starting ssldump");
   
   int dumpPid = fork();

   if (dumpPid==-1)
   {
     DebugLog(<< "Can't fork to run ssldump"); exit(-1);
   }

   if (dumpPid==0)
   {
     // todo - make this something that can come on the command line
     const char *args[] = { "ssldump","-A","-i","lo0",NULL };
     // .kw. execvp takes a 'char* const *' -- for some reason it doesn't
     // promise to not modify the strings. Thus shouldn't use static string
     // initializers above!
     int ret = execvp("ssldump",const_cast<char**>(args));
     ErrLog(<< "Can't execvp: return is " << ret <<" errno is " << errno);
     exit(-1);
   }

   DebugLog(<< "ssldump's pid is " << dumpPid);
 
   IpVersion version = V4;
   Data bindInterface;
   //bindInterface = Data( "127.0.0.1" );
         
   SipStack* clientStack;
   SipStack* serverStack;

   Security* clientSecurity = new Security(certPath);
   Security* serverSecurity = new Security(certPath);
   clientStack = new SipStack(clientSecurity);
   serverStack = new SipStack(serverSecurity);

   clientStack->addTransport(TLS, clientPort, version, StunDisabled, bindInterface, clientDomain);
   serverStack->addTransport(TLS, serverPort, version, StunDisabled, bindInterface, serverDomain);
 
   NameAddr target;
   target.uri().scheme() = "sip";
   target.uri().user() = "fluffy";
   target.uri().host() = serverDomain;
   target.uri().port() = serverPort;
   target.uri().param(p_transport) = "tls";

   NameAddr contact;
   contact.uri().scheme() = "sip";
   contact.uri().user() = "fluffy";
   contact.uri().host() = clientDomain;
   contact.uri().port() = clientPort;

   NameAddr from = target;
   from.uri().port() = clientPort;
   
   InfoLog (<< "Starting" );

   SipMessage* msg = Helper::makeRegister( target, from, contact);
   clientStack->send(*msg);
   delete msg;
      
   bool done=false;
   int rundown = 0;
   while ( (!done) || (rundown-- > 0) )
   {

      FdSet fdset; 

      clientStack->buildFdSet(fdset);
      serverStack->buildFdSet(fdset);

      fdset.selectMilliSeconds(100); 


      serverStack->process(fdset);
      SipMessage *msg = serverStack->receive();
      if ( !msg ) 
      {
          clientStack->process(fdset);
          msg = clientStack->receive();
      }
   
      if ( msg )
      {
         if ( msg->isRequest() )
         {  
            assert(msg->isRequest());
            assert(msg->header(h_RequestLine).getMethod() == REGISTER);
            
            SipMessage* response = Helper::makeResponse(*msg, 200);
            serverStack->send(*response);
            delete response;
            delete msg;
         }
         else
         { 
            assert(msg->isResponse());
            assert(msg->header(h_CSeq).method() == REGISTER);
            if (msg->header(h_StatusLine).statusCode() < 200)
            {
              DebugLog(<<"Provisional response to a REGISTER - you're kidding me, right?");
            }
            else
            {
              done = true;
              rundown = 100;
            }
            delete msg;
         }
      }
   }
   InfoLog (<< "Finished " );
   
#if defined(HAVE_POPT_H)
   poptFreeContext(context);
#endif
   sleep(2);
   delete serverStack; //btw - this deletes the security object
   delete clientStack;
   sleep(5);

   if (dumpPid>0)

   {
     DebugLog( << "Trying to kill process id " << dumpPid);
     int ret = kill(dumpPid, SIGINT); // ssldump catches INT and does an fflush
     DebugLog( << "Kill returned: " << ret);
   }


#endif
   return 0;
}
/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2008 Robert Sparks.  All rights reserved.
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
 */
