#include "resiprocate/SipStack.hxx"
#include "resiprocate/Transport.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/test/TestSupport.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/DataStream.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::APP
#define CRLF "\r\n"


SipStack* client=0;
Fifo<Message> received;

void
doit(int serverResponse, int expectedRetrans, int expectedClientResponse);


int
main(int argc, char *argv[])
{
   Log::initialize(Log::COUT, Log::DEBUG, argv[0]);

   DebugLog( << "Starting up, making stack");
   InfoLog( << "Starting up, making stack");

   client = new SipStack();
   client->addTransport(UDP, 5070);

   // Test 1: 
   // client sends a reg, server does nothing, client should retransmit 10
   // times, client should receive 408

   // Test 2: 
   // client sends a reg, server sends 100, client should retransmit 7 times
   // client should receive 408
    
   // Test 3:
   // client sends a reg, server sends 200, client shouldn't retransmit at all
   // client should receive 200

   // Test 4:
   // client sends a reg, server sends 400, client shouldn't retransmit at all
   // client should receive 400

   //doit(100, 7, 408);
   doit(200, 1, 200);
   //    doit(400, 1, 400);
   //doit(0, 10, 408);
    
   return 0;
}


void
doit(int serverResponse, int expectedRetrans, int expectedClientResponse)
{
   InfoLog (<< "Running test: " << serverResponse << " " << expectedRetrans << " " << expectedClientResponse);
   
   NameAddr me;
   me.uri().host() = "localhost";
   me.uri().port() = 5070;
   SipMessage* reg = Helper::makeRegister(me, me);
   InfoLog (<< "Sending: " << *reg);

   Data encoded(2048, true);
   DataStream strm(encoded);
   reg->encode(strm);
   strm.flush();
    
   client->send(*reg);   // send message down the stack

   FdSet fdset;
   client->process(fdset);

   // read the message off the stack
   Data fromStack;
   getFromWire(fromStack);

   InfoLog(<< "Received from wire " << fromStack);

   SipMessage* message = TestSupport::makeMessage(fromStack);
      
   // send the response message

   SipMessage* response = Helper::makeResponse(*message, 100);
   InfoLog (<< "sending to wire = " << endl << *response);
            
   Data encodedResponse(2048, true);

   DataStream estrm(encodedResponse);
   response->encode(estrm);
   estrm.flush();

   //    sendToWire(encodedResponse);

   while (1)
   {
      client->process(fdset);
      usleep(20);
   }
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
