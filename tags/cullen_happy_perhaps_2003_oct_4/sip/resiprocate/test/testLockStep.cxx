
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


#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


class Client : public ThreadIf
{
    public:
      Client(SipStack& stack) : mStack(stack)
      {}
      
      void thread()
      {
         InfoLog(<<"This is the Client");

         NameAddr dest;
         dest.uri().scheme() = "sip";
         dest.uri().user() = "fluffy";
         dest.uri().host() = "localhost";
         dest.uri().port() = 5080;
         dest.uri().param(p_transport) = "tcp";
         
         NameAddr from = dest;
         from.uri().port() = 5070;
         
         Data count(1);
   
         from.uri().user() = count;
         {
            auto_ptr<SipMessage> message(Helper::makeInvite( dest, from, from));
            mStack.send(*message);
         }
         
         bool done = false;
         bool inviteState = true;
         
         
         while(true)
         {
            FdSet fdset;
            mStack.buildFdSet(fdset);
            int err = fdset.selectMilliSeconds(5);
            assert (err != -1);
            mStack.process(fdset);
            
            SipMessage* received = mStack.receive();
            if (received)
            {
               InfoLog (<< "Client received: " << received->brief());
               
               auto_ptr<SipMessage> forDel(received);
               if ( (received->isResponse()) )
               {
                  if ( received->header(h_StatusLine).responseCode() == 200 )
                  {
                     if (done)
                     {
                        break;
                     }
                     done = true;
                     
                     DebugLog(<< "Creating dialog.");
                     Dialog dlog(from);
                        
                     DebugLog(<< "Creating dialog as UAC.");
                     dlog.createDialogAsUAC(*received);
                        
                     DebugLog(<< "making ack.");
                     auto_ptr<SipMessage> ack(dlog.makeAck(*received) );
                        
                     DebugLog(<< "making bye.");
                     auto_ptr<SipMessage> bye(dlog.makeBye());
                        
                     DebugLog(<< "Sending ack: << " << endl << *ack);
                     mStack.send(*ack);
                        
                     DebugLog(<< "Sending bye: << " << endl << *bye);
                     mStack.send(*bye);
                  }
               }
            }
            usleep(1000);
         }
      }
   private:
      SipStack& mStack;
};

class Server : public ThreadIf
{
    public:

      Server(SipStack& stack) : mStack(stack)
      {}
      
      void thread()
      {
         InfoLog(<<"This is the Server");

         NameAddr dest;
         dest.uri().scheme() = "sip";
         dest.uri().user() = "fluffy";
         dest.uri().host() = "localhost";
         dest.uri().port() = 5080;
         dest.uri().param(p_transport) = "tcp";

         while(true)
         {
            FdSet fdset;
            mStack.buildFdSet(fdset);
            int err = fdset.selectMilliSeconds(5);
            assert (err != -1);
            mStack.process(fdset);
            
            SipMessage* received = mStack.receive();
            if (received)
            {
               auto_ptr<SipMessage> forDel(received);
               InfoLog ( << "Server recieved: " << received->brief());
               MethodTypes meth = received->header(h_RequestLine).getMethod();
               if ( meth == INVITE )
               {
                  Data localTag = Helper::computeTag(4);
                  auto_ptr<SipMessage> msg180(Helper::makeResponse(*received, 180, dest));
                  msg180->header(h_To).param(p_tag) = localTag;
                  mStack.send( *msg180);
                  
                  auto_ptr<SipMessage> msg200(Helper::makeResponse(*received, 200, dest));
                  msg200->header(h_To).param(p_tag) = localTag;
                  mStack.send(*msg200);
               }
               if ( meth == BYE)
               {
                  auto_ptr<SipMessage> msg200(Helper::makeResponse(*received, 200, dest));
                  InfoLog (<< "stack2 got bye - send 200 : " << *msg200 );   
               
                  mStack.send(*msg200);
               }
            }
         }
      }
   private:
      SipStack& mStack;
};


int
main(int argc, char* argv[])
{
   Log::initialize(Log::COUT, argc > 1 ? Log::toLevel(argv[1]) :  Log::ERR, argv[0]);
   Log::toLevel( Data("DEBUG") );

   SipStack stack1;
   stack1.addTransport(TCP, 5070);

   SipStack stack2;
   stack2.addTransport(TCP, 5080);

   Client client(stack1);
   Server server(stack2);
   
   client.run();
   server.run();

   client.join();
   server.join();

   InfoLog(<< "Test failed.");

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
