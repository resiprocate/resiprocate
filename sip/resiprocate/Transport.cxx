#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include <iostream>

#if defined(HAVE_SYS_SOCKIO_H)
#include <sys/sockio.h>
#endif

#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Socket.hxx"

#include "resiprocate/Transport.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/TransportMessage.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

Transport::Exception::Exception(const Data& msg, const Data& file, const int line) :
   BaseException(msg,file,line)
{
}

Transport::Transport(Fifo<Message>& rxFifo, int portNum, const Data& intfc, bool ipv4) : 
   mFd(-1),
   mPort(portNum), 
   mInterface(intfc),
   mStateMachineFifo(rxFifo)
{
}

Transport::~Transport()
{
   mFd = -2;
}


void
Transport::thread()
{
   InfoLog (<< "Starting transport thread for " << this);
   while (!mShutdown)
   {
      FdSet fdset; 
      buildFdSet(fdset);
      int  err = fdset.selectMilliSeconds(100);
      if (err >= 0)
      {
         try
         {
            process(fdset);
         }
         catch (BaseException& e)
         {
            InfoLog (<< "Uncaught exception: " << e);
         }
      }
   }
}

void
Transport::fail(const Data& tid)
{
   mStateMachineFifo.add(new TransportMessage(tid, true));
}

bool 
Transport::hasDataToSend() const
{
   return mTxFifo.messageAvailable();
}


void 
Transport::send( const Tuple& dest, const Data& d, const Data& tid)
{
   SendData* data = new SendData(dest, d, tid);
   assert(dest.port != -1);
   DebugLog (<< "Adding message to tx buffer to: " << dest); // << " " << d.escaped());
   mTxFifo.add(data); // !jf!
}

void
Transport::stampReceived(SipMessage* message)
{
   //DebugLog (<< "adding new SipMessage to state machine's Fifo: " << message->brief());
   // set the received= and rport= parameters in the message if necessary !jf!
   if (message->isRequest() && message->exists(h_Vias) && !message->header(h_Vias).empty())
   {
      const Tuple& tuple = message->getSource();
      message->header(h_Vias).front().param(p_received) = DnsUtil::inet_ntop(tuple);
      if (message->header(h_Vias).front().exists(p_rport))
      {
         message->header(h_Vias).front().param(p_rport).port() = tuple.port;
      }
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
