
#include "util/Socket.hxx"


#include "sipstack/SipStack.hxx"
#include "sipstack/Executive.hxx"
#include "sipstack/SipMessage.hxx"
#include "sipstack/Message.hxx"
#include "util/Fifo.hxx"
#include "util/Data.hxx"
#include "util/Logger.hxx"


using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::SIP

SipStack::SipStack(bool multiThreaded)
  : mExecutive(*this),
    mTransportSelector(*this),
    mTimers(mStateMacFifo),
    mDnsResolver(*this)
{
   //addTransport(Transport::UDP, 5060);
   //addTransport(Transport::TCP, 5060); // !jf!
}

void 
SipStack::addTransport( Transport::Type protocol, 
                        int port,
                        const Data& hostName,
                        const Data& nic) 
{
   mTransportSelector.addTransport(protocol, port, hostName, nic);
}


void 
SipStack::send(const SipMessage& msg)
{
   SipMessage* toSend = new SipMessage(msg);
   toSend->setFromTU();
   mStateMacFifo.add(toSend);
}

// this is only if you want to send to a destination not in the route. You
// probably don't want to use it. 
void 
SipStack::sendTo(const SipMessage& msg, const Data& dest)
{
   SipMessage* toSend = new SipMessage(msg);
   toSend->setFixedDest(dest);
   toSend->setFromTU();
   mStateMacFifo.add(toSend);
}


SipMessage* 
SipStack::receive()
{
   // Check to see if a message is available and if it is return the 
   // waiting message. Otherwise, return 0
   if (mTUFifo.messageAvailable())
   {
      DebugLog (<< "message available");
      
      // we should only ever have SIP messages on the TU Fifo
      Message *tmpMsg = mTUFifo.getNext();
      SipMessage *sipMsg = dynamic_cast<SipMessage*>(tmpMsg);
      assert (sipMsg);
      return sipMsg;
   }
   else
   {
	   //DebugLog (<< "no message available");
      return 0;
   }
}


void 
SipStack::process(FdSet& fdset)
{
   mExecutive.process(fdset);
}


/// returns time in milliseconds when process next needs to be called 
int 
SipStack::getTimeTillNextProcess()
{
	return mExecutive.getTimeTillNextProcess();
} 


void 
SipStack::buildFdSet(FdSet& fdset)
{
   mExecutive.buildFdSet( fdset );
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
