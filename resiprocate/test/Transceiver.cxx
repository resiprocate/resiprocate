#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Transceiver.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/os/Timer.hxx"


#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

using namespace std;
using namespace resip;
using namespace Loadgen;

Transceiver::Transceiver(int port)
   : mUdp(mReceived, port)
{
   mContactUri.host() = mUdp.hostName();
   mContactUri.port() = mUdp.port();
   mContactUri.param(p_transport) = Transport::toData(mUdp.transport());
}

void
Transceiver::send(SipMessage& message)
{
   Uri target;
   if (message.isRequest())
   {
      if (message.header(h_Routes).size() && !message.header(h_Routes).front().exists(p_lr))
      {
         target = message.header(h_Routes).front().uri();
      }
      else
      {
         target = message.header(h_RequestLine).uri();         
      }
   }
   else if (message.isResponse())
   {
      assert (!message.header(h_Vias).empty());
      const Via& via = message.header(h_Vias).front();
      
      // should look at via.transport()
      target.param(p_transport) = Symbols::UDP; // !jf!
      target.host() = via.sentHost();
      target.port() = via.sentPort();
      
      if (via.exists(p_received))
      {
         target.host() = via.param(p_received);
      }
      if (via.exists(p_rport))
      {
         target.port() = via.param(p_rport).port();
      }
   }
   else
   {
      assert(0);
   }
   
   // do a dns lookup !jf!
   // should only do this once and store in the SipMessage (or somewhere)
   DebugLog(<<"Trying to resolve target: " << target);
   Resolver resolver(target);

   send(target, message);
}


void 
Transceiver::send(const Resolver& target,  
                  SipMessage& message)
{
   DebugLog(<< message.header(h_Vias).size() );
 
   if (message.isRequest())
   {
      assert(!message.header(h_Vias).empty());
      message.header(h_Vias).front().transport() = Transport::toData(mUdp.transport()); 
      message.header(h_Vias).front().sentHost() = mUdp.hostName();
      message.header(h_Vias).front().sentPort() = mUdp.port();
   }
   
   Data& encoded = message.getEncoded();
   DataStream strm(encoded);
   message.encode(strm);
   strm.flush();
   mUdp.send(target.mNextHops.front(), encoded, "bogus"); 
   mUdp.process(mFdset);
}

SipMessage*
Transceiver::receive(int waitMs)
{
   UInt64 startTime = Timer::getTimeMs();

   UInt64 currentTime = Timer::getTimeMs();
   
   while( UInt64(waitMs) > currentTime - startTime)
   {
      int timeLeft = waitMs - (currentTime - startTime);
      if (!mReceived.messageAvailable())
      {
         mUdp.buildFdSet(mFdset);
         
         int  err = mFdset.selectMilliSeconds(timeLeft);
         int e = errno;
         if ( err == -1 )
         {
            InfoLog(<< "Error " << e << " " << strerror(e) << " in select");
         }
         
         DebugLog(<<"Calling process in Transceiver::receive");
         mUdp.process(mFdset);
      }

      if (mReceived.messageAvailable())
      {
         Message* msg = mReceived.getNext();
         DebugLog(<<"Received a message in the transceiver, " << msg);
         
         SipMessage* next = dynamic_cast<SipMessage*>(msg);
         DebugLog(<<"Dynamic cast resulted in: " << next);
         if (next)
         {
            DebugLog(<<"Received a sip message in the transceiver.");
            return next;
         }
         else
         {
            DebugLog(<<"Which was apparently not a sip message.");
            delete msg;
         }
      }
      currentTime = Timer::getTimeMs();
   }
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
