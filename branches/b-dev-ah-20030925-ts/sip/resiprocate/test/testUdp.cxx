#include <iostream>
#include <popt.h>

#include "resiprocate/UdpTransport.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/DataStream.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{
   char* logType = 0;
   char* logLevel = 0;
   int runs = 100;
   int window = 10;
   int seltime = 100;

   struct poptOption table[] = {
      {"log-type",    'l', POPT_ARG_STRING, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",   'v', POPT_ARG_STRING, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
      {"num-runs",    'r', POPT_ARG_INT,    &runs,      0, "number of calls in test", 0},
      {"window-size", 'w', POPT_ARG_INT,    &window,    0, "number of registrations in test", 0},
      {"select-time", 's', POPT_ARG_INT,    &seltime,   0, "number of runs in test", 0},
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);
   Log::initialize(logType, logLevel, argv[0]);

   cout << "Performing " << runs << " runs." << endl;
   
   Fifo<Message> txFifo;
   UdpTransport* sender = new UdpTransport(txFifo, 5070, Data::Empty);

   Fifo<Message> rxFifo;
   UdpTransport* receiver = new UdpTransport(rxFifo, 5080, Data::Empty);

   NameAddr target;
   target.uri().scheme() = "sip";
   target.uri().user() = "fluffy";
   target.uri().host() = "localhost";
   target.uri().port() = 5080;
   target.uri().param(p_transport) = "udp";
   
   NameAddr from = target;
   from.uri().port() = 5070;

   InfoLog (<< "Creating messages");

   list<SipMessage*> messages;
   {
      UInt64 startTime = Timer::getTimeMs();
      for (int i=0; i<runs; i++)
      {
         SipMessage* m = Helper::makeInvite( target, from, from);      
         m->header(h_Vias).front().transport() = Tuple::toData(sender->transport());
         m->header(h_Vias).front().sentHost() = "localhost";
         m->header(h_Vias).front().sentPort() = sender->port();
      
         messages.push_back(m);
      }

      UInt64 elapsed = Timer::getTimeMs() - startTime;
      cout <<  runs * ( 1000.0 / (float) elapsed) * ( 1000.0 / (float)Timer::getCpuSpeedMhz() ) 
           << " half calls/s/GHz  ["
           << runs << " calls peformed in " << elapsed << " ms, a rate of " 
           << runs / ((float) elapsed / 1000.0) << " calls per second.]" << endl;
      
      InfoLog (<< "Messages created");
   }
   
   
   in_addr in;
   DnsUtil::inet_pton("127.0.0.1", in);
   Tuple dest(in, target.uri().port(), UDP);
   InfoLog (<< "Sending to " << dest);
   
   UInt64 startTime = Timer::getTimeMs();

   int tid=1;
   int outstanding=0;
   int count=0;
   while (!messages.empty())
   {
      if (count > 500) exit(0);
      // load up the send window
      while (outstanding < window)
      {
         Data encoded;
         {
            DataStream strm(encoded);
            SipMessage* next = messages.front();
            messages.pop_front();
            next->encode(strm);
            outstanding++;
         }
         sender->send(dest, encoded, Data(tid++));
      }

      FdSet fdset; 
      receiver->buildFdSet(fdset);
      //sender->buildFdSet(fdset);
      int err = fdset.selectMilliSeconds(seltime);

      receiver->process(fdset);
      sender->process(fdset);
      
      Message* msg;
      if (rxFifo.messageAvailable())
      {
         msg = rxFifo.getNext();
         SipMessage* received = dynamic_cast<SipMessage*>(msg);
         if (received)
         {
            //DebugLog (<< "got: " << received->brief());
            outstanding--;
         
            assert (received->header(h_RequestLine).uri().host() == "localhost");
            assert (received->header(h_To).uri().host() == "localhost");
            assert (received->header(h_From).uri().host() == "localhost");
            assert (!received->header(h_Vias).begin()->sentHost().empty());
            assert (received->header(h_Contacts).begin()->uri().host() == "localhost");
            assert (!received->header(h_CallId).value().empty());
            delete received;
         }
      }
   }

   UInt64 elapsed = Timer::getTimeMs() - startTime;
   cout <<  runs * ( 1000.0 / (float) elapsed) * ( 1000.0 / (float)Timer::getCpuSpeedMhz() ) 
        << " half calls/s/GHz  ["
        << runs << " calls peformed in " << elapsed << " ms, a rate of " 
        << runs / ((float) elapsed / 1000.0) << " calls per second.]" << endl;

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
