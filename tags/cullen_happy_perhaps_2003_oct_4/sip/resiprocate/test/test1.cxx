#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <memory>      

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "resiprocate/UdpTransport.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/DataStream.hxx"

#include "Resolver.hxx"


using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{
   Log::initialize(Log::COUT, Log::DEBUG, argv[0]);
   int runs = 100000;
   if (argc == 2)
   {
      runs = atoi(argv[1]);
   }

   cout << "Performing " << runs << " runs." << endl;
   
   Fifo<Message> received;
   
   UdpTransport* udp = new UdpTransport("localhost", 5070, "default", received);

   NameAddr dest;
   dest.uri().scheme() = "sip";
   dest.uri().user() = "fluffy";
   dest.uri().host() = "localhost";
   dest.uri().port() = 5070;
   dest.uri().param(p_transport) = "udp";
   
   NameAddr from = dest;
   from.uri().port() = 5070;

   for (int i=0; i<runs; i++)
   {
      auto_ptr<SipMessage> message = auto_ptr<SipMessage>(Helper::makeInvite( dest, from, from));
      Resolver resolver(dest.uri());
      
      TransportType t = udp->transport();
      Data foo = Transport::toData(t); 
      message->header(h_Vias).front().transport() = foo;
      message->header(h_Vias).front().sentHost() = udp->hostName();
      message->header(h_Vias).front().sentPort() = udp->port();

      Data encoded;
      {
         DataStream strm(encoded);
         message->encode(strm);
      }
      assert(!resolver.mNextHops.empty());
      cerr << "!! " << resolver.mNextHops.front() << endl;

      udp->send(resolver.mNextHops.front(), encoded, "foo");
      
      FdSet fdset; 
      udp->buildFdSet(fdset);
      
      int  err = fdset.selectMilliSeconds(5);
      assert ( err != -1 );

      udp->process(fdset);

      while (received.messageAvailable())
      {
         Message* msg = received.getNext();
         SipMessage* next = dynamic_cast<SipMessage*>(msg);
         if (next)
         {
            DebugLog (<< "got: " << next->brief());
            assert (next->header(h_RequestLine).uri().host() == "localhost");
            assert (next->header(h_To).uri().host() == "localhost");
            assert (next->header(h_From).uri().host() == "localhost");
            assert (!next->header(h_Vias).begin()->sentHost().empty());
            assert (next->header(h_Contacts).begin()->uri().host() == "localhost");
            assert (!next->header(h_CallId).value().empty());
         }
         delete msg;
      }
   }
   delete udp;

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
