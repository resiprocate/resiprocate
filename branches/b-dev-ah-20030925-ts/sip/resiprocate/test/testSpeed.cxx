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

#include "resiprocate/Dialog.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{
   Log::initialize(Log::COUT, argc > 1 ? Log::toLevel(argv[1]) :  Log::ERR, argv[0]);

   SipStack stack1;
   SipStack stack2;
   stack1.addTransport(UDP, 5070);
   stack2.addTransport(UDP, 5080);

   NameAddr dest;
   dest.uri().scheme() = "sip";
   dest.uri().user() = "fluffy";
   dest.uri().host() = "localhost";
   dest.uri().port() = 5080;
   dest.uri().param(p_transport) = "udp";
   
   NameAddr from = dest;
   from.uri().port() = 5070;
   
   int totalCalls = 500;

   bool done[10000];
   assert( sizeof(done)/sizeof(bool) > (unsigned int)(totalCalls+1) );
   for ( int ii=0; ii<=totalCalls; ii++ )
   {
      done[ii]=false;
   }
   
   int lastSent=0;
   int lastRecv=0;
   int i=0;
   
   UInt64 startTime = Timer::getTimeMs();

   while ( lastRecv < totalCalls )
   {
      InfoLog( << "loop " << i++  );
      
      if ( (lastRecv >= lastSent) && (lastSent <= totalCalls) )
      {
         Data count( ++lastSent );
         from.uri().user() = count;
         SipMessage* message = Helper::makeInvite( dest, from, from);
         stack1.send(*message);
         delete message;
         InfoLog( << "Stack1 sent msg from user: " << count );
      }
         
      FdSet fdset;
      stack1.buildFdSet(fdset);
      stack2.buildFdSet(fdset);
      int err = fdset.selectMilliSeconds(0);
      assert (err != -1);

      stack1.process(fdset);
      stack2.process(fdset);

      SipMessage* received1 = (stack1.receive());
      if (received1)
      {
         InfoLog (<< "stack1 got: " << received1->brief() 
                  << " from user: " << received1->header(h_From).uri().user() );

         int user = atoi( received1->header(h_From).uri().user().c_str() );
      
         if ( (received1->isResponse()) )
         {
            if ( received1->header(h_StatusLine).responseCode() == 200 )
            {
               if ( !done[user] )
               {
                  DebugLog(<< "Creating dialog.");
                  done[user]=true;
               
                  Dialog dlog(from);
               
                  DebugLog(<< "Creating dialog as UAC.");
                  dlog.createDialogAsUAC(*received1);
               
                  DebugLog(<< "making ack.");
                  auto_ptr<SipMessage> ack(dlog.makeAck(*received1) );
               
                  DebugLog(<< "making bye.");
                  auto_ptr<SipMessage> bye(dlog.makeBye());
               
                  DebugLog(<< "Sending ack: << *ack");
                  stack1.send(*ack);

                  DebugLog(<< "Sending bye: << *bye");
                  stack1.send(*bye);  
               }
               else
               { 
                  if ( user > lastRecv )
                  {
                     lastRecv = user;
                  }
               }
            }
         }     
         
         delete received1;
      }
               
      SipMessage* received2 = (stack2.receive());
      if (received2)
      {
         InfoLog (<< "stack2 got: " << received2->brief() 
                  << " from user: " << received2->header(h_From).uri().user() );   
 
         MethodTypes meth = received2->header(h_RequestLine).getMethod();

         if ( meth == INVITE )
         {
            //Data localTag = Helper::computeTag(4);

            auto_ptr<SipMessage> msg180(Helper::makeResponse(*received2, 180, dest, "Ringing"));
            //msg180->header(h_To).uri().param(p_tag) = localTag;
            stack2.send( *msg180);
            
            auto_ptr<SipMessage> msg200(Helper::makeResponse(*received2, 200, dest, "OK"));
            //msg200->header(h_To).uri().param(p_tag) = localTag;
            stack2.send(*msg200);
         }

         if ( meth == BYE )
         {
            auto_ptr<SipMessage> msg200(Helper::makeResponse(*received2, 200, dest, "OK"));
            InfoLog (<< "stack2 got bye - send 200 : " << *msg200 );   
            
            stack2.send(*msg200);
         }

         delete received2;
      }
   }
  
   UInt64 elapsed = Timer::getTimeMs() - startTime;
   cout << 2.0 * totalCalls * ( 1000.0 / (float) elapsed) * ( 1000.0 / (float)Timer::getCpuSpeedMhz() ) 
        << " half calls/s/GHz  ["
        << totalCalls << " calls peformed in " << elapsed << " ms, a rate of " 
        << totalCalls / ((float) elapsed / 1000.0) << " calls per second.]" << endl;

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
