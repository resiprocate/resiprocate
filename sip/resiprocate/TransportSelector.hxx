#if !defined(TRANSPORTSELECTOR_HXX)
#define TRANSPORTSELECTOR_HXX

#ifndef WIN32
#include <sys/select.h>
#endif

#include <vector>

#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Fifo.hxx"

#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Transport.hxx"


namespace resip
{

class DnsResolver;
class SipMessage;
class UdpTransport;
class SipStack;

class TransportSelector
{
   public:
      TransportSelector(SipStack& stack);
      ~TransportSelector();
      bool hasDataToSend() const;
      
      void process(FdSet& fdset);
      void buildFdSet(FdSet& fdset);
     
      void addTransport( Transport::Type, int port, const Data& hostName="", const Data& nic="");
      void dnsResolve(SipMessage* msg, const Data& tid);

      // this will result in msg->resolve() being called to either
      // kick off dns resolution or to pick the next tuple , will cause the
      // message to be encoded and via updated
      void send( SipMessage* msg, Transport::Tuple destination, const Data& tid, bool isResend=false );

      // just resend to the same transport as last time
      void retransmit(SipMessage* msg, Transport::Tuple& destination );
      
   private:
      Transport* findTransport(const Transport::Type type) const;
      Transport* findTransport(const Transport::Tuple& tuple) const;

      SipStack& mStack;
      std::vector<Transport*> mTransports;

      friend class DnsResolver;
};

}

#endif


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
