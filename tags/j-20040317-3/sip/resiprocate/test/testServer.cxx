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

#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/Dialog.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/ThreadIf.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

class Server : public ThreadIf
{
    public:

      Server(SipStack& stack, int numCalls, TransportType transport) 
         : mStack(stack), 
           mNumCalls(numCalls),
           mTransport(transport)
      {}
      
      void thread()
      {
         InfoLog(<<"This is the Server");

         UInt64 startTime = Timer::getTimeMs();

         NameAddr contact;
         contact.uri().scheme() = "sip";
         contact.uri().user() = "fluffy";
         
         contact.uri().host() = SipStack::getHostname();
         contact.uri().port() = 5070;
         contact.uri().param(p_transport) = Tuple::toData(mTransport);
         
         int calls = mNumCalls;
         while(calls > 0)
         {
            FdSet fdset;
            mStack.buildFdSet(fdset);
            int err = fdset.selectMilliSeconds(0);
            assert (err != -1);
            mStack.process(fdset);
            
            SipMessage* received = mStack.receive();
            if (received)
            {
               auto_ptr<SipMessage> forDel(received);
               MethodTypes meth = received->header(h_RequestLine).getMethod();
               ErrLog ( << "Server received: " << MethodNames[meth]);
               if ( meth == INVITE )
               {
                  Data localTag = Helper::computeTag(4);
                  auto_ptr<SipMessage> msg180(Helper::makeResponse(*received, 180, contact));
                  msg180->header(h_To).param(p_tag) = localTag;
                  ErrLog( << "Sent 180");
                  mStack.send( *msg180);

                  auto_ptr<SipMessage> msg200(Helper::makeResponse(*received, 200, contact));
                  msg200->header(h_To).param(p_tag) = localTag;
                  ErrLog( << "Sent 200");
                  mStack.send(*msg200);
               }
               if ( meth == BYE)
               {
                  auto_ptr<SipMessage> msg200(Helper::makeResponse(*received, 200, contact));
                  calls--;
                  ErrLog( << "Sent 200 to BYE");
                  mStack.send(*msg200);
               }
            }
         }
         UInt64 endTime = Timer::getTimeMs();

         CritLog(<< "Completed: " << mNumCalls << " calls in " << endTime - startTime << "ms, " 
                 << mNumCalls*1000 / (float)(endTime - startTime) << " CPS");
      }
   private:
      SipStack& mStack;
      int mNumCalls;
      TransportType mTransport;
};


int
main(int argc, char* argv[])
{
   if (argc != 4)
   {
      cerr << argv[0] << " LOG_LEVEL NUM_CALLS PROTOCOL" << endl;
      exit(-1);
   } 
   Log::initialize(Log::COUT, Log::toLevel(argv[1]), argv[0]);
   SipStack stack;


   TransportType protocol = UDP;
   if (strcasecmp(argv[3], "UDP") == 0)
   {
      protocol = UDP;
   }
   else if (strcasecmp(argv[3], "TCP") == 0)
   {
      protocol = TCP;
   }
   else
   {
      cerr << argv[0] << " LOG_LEVEL TARGET_URI PROTOCOL" << endl;
   }

   stack.addTransport(protocol, 5070);


   int numCalls = atoi(argv[2]);

   if (numCalls == 0)
   {
      cerr << argv[0] << " LOG_LEVEL NUM_CALLS" << endl;
      exit(-1);
   } 

   Server server(stack, numCalls, protocol);
   
   server.run();
   server.join();
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
