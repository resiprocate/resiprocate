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
#include <unistd.h> // for sleep
#include <iostream>
#include <memory>

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

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{
#ifdef USE_SSL
   char logType[] = "cout";
   char logLevel[] = "DEBUG";

   int window = 5*20;
   int seltime = 100;

   const int MaxStacks=100;
   int numStacks=20;

   //logLevel = "ALERT";
   //logLevel = "INFO";

#if defined(HAVE_POPT_H)
   struct poptOption table[] = {
      {"log-type",    'l', POPT_ARG_STRING, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",   'v', POPT_ARG_STRING, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
      {"num-stacks",    'r', POPT_ARG_INT,  &numStacks,      0, "number of calls in test", 0},
      {"window-size", 'w', POPT_ARG_INT,    &window,    0, "number of concurrent transactions", 0},
      {"select-time", 's', POPT_ARG_INT,    &seltime,   0, "number of runs in test", 0},
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);
#endif

   int runs = 3*numStacks*numStacks;

   runs = 1;
   
   Log::initialize(logType, logLevel, argv[0]);
   cout << "Performing " << runs << " runs." << endl;

   IpVersion version = V4;
   Data bindInterface;
   //bindInterface = Data( "127.0.0.1" );
         
   SipStack* stack[MaxStacks];
   for ( int s=0; s<numStacks; s++)
   {
      stack[s] = new SipStack;

      Data domain = Data("example") + Data(s) +".com";
      
#ifdef USE_DTLS
      stack[s]->addTransport(DTLS, 25000+s,version, StunDisabled, bindInterface, domain);
#else
      stack[s]->addTransport(TLS,  25000+s,version, StunDisabled, bindInterface, domain);
#endif
   }
   
   NameAddr target;
   target.uri().scheme() = "sip";
   target.uri().user() = "fluffy";
   target.uri().host() = Data("127.0.0.1");
   target.uri().port() = 25000;
#ifdef USE_DTLS
   target.uri().param(p_transport) = "dtls";
#else
   target.uri().param(p_transport) = "tls";
#endif  

   NameAddr contact;
   contact.uri().scheme() = "sip";
   contact.uri().user() = "fluffy";
   contact.uri().host() = Data("127.0.0.1");
   contact.uri().port() = 25000;

   NameAddr from = target;
   from.uri().port() = 25000;
   
   UInt64 startTime = Timer::getTimeMs();
   int outstanding=0;
   int count = 0;
   int sent = 0;
   int msgMod=0;
   
   while (count < runs)
   {
      //InfoLog (<< "count=" << count << " messages=" << messages.size());
      
      // load up the send window
      while (sent < runs && outstanding < window)
      {
         DebugLog (<< "Sending " << count << " / " << runs << " (" << outstanding << ")");

         // send from stack s to to stack r 
         int s = msgMod%numStacks;
         int r = (msgMod/numStacks)%numStacks;
         msgMod++;
         if ( s == r ) 
         {
            continue;
         }
         
         target.uri().port() = 25000+r;
         from.uri().port() = 25000+s;
         contact.uri().port() = 25000+s;
   
         SipMessage* msg = Helper::makeRegister( target, from, contact);
         msg->header(h_Vias).front().sentPort() = 25000+s;
         stack[s]->send(*msg);
         outstanding++;
         sent++;
         delete msg;
      }
      
      FdSet fdset; 
      for ( int s=0; s<numStacks; s++)
      {
         stack[s]->buildFdSet(fdset);
      }
      fdset.selectMilliSeconds(seltime); 
      for ( int s=0; s<numStacks; s++)
      {
         stack[s]->process(fdset);
      }
       
      for ( int s=0; s<numStacks; s++)
      { 
         SipMessage* msg = stack[s]->receive();
         
         if ( msg )
         {
            if ( msg->isRequest() )
            {  
               assert(msg->isRequest());
               assert(msg->header(h_RequestLine).getMethod() == REGISTER);
               
               SipMessage* response = Helper::makeResponse(*msg, 200);
               stack[s]->send(*response);
               delete response;
               delete msg;
            }
            else
            { 
               assert(msg->isResponse());
               assert(msg->header(h_CSeq).method() == REGISTER);
               assert(msg->header(h_StatusLine).statusCode() == 200);
               outstanding--;
               count++;
               delete msg;
            }
         }
      }
   }
   InfoLog (<< "Finished " << count << " runs");
   
   UInt64 elapsed = Timer::getTimeMs() - startTime;
   cout << runs << " registrations peformed in " << elapsed << " ms, a rate of " 
        << runs / ((float) elapsed / 1000.0) << " transactions per second.]" << endl;
#if defined(HAVE_POPT_H)
   poptFreeContext(context);
#endif

   while (true)
   {
      sleep(10);
   }
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
