#if !defined(RESIP_TRANSACTION_CONTROLLER_HXX)
#define RESIP_TRANSACTION_CONTROLLER_HXX

#include "resiprocate/os/Fifo.hxx"
#include "resiprocate/Message.hxx"
#include "resiprocate/TransactionMap.hxx"
#include "resiprocate/TransportSelector.hxx"
#include "resiprocate/StatelessHandler.hxx"
#include "resiprocate/TimerQueue.hxx"
#include "resiprocate/DnsResolver.hxx"

namespace resip
{

class TransactionController
{
   public:
      TransactionController(bool multithreaded, Fifo<Message>& tufifo, bool stateless=false);
      ~TransactionController();

      void process(FdSet& fdset);
      int getTimeTillNextProcessMS();
      void buildFdSet(FdSet& fdset);
      
      // graceful shutdown (eventually)
      void shutdown();

      void addTransport( TransportType protocol, 
                         int port,
                         IpVersion version,
                         const Data& ipInterface);
      void addTlsTransport( int port, 
                            const Data& keyDir,
                            const Data& privateKeyPassPhrase,
                            const Data& domainname,
                            IpVersion version,
                            const Data& ipInterface);
      
      void send(SipMessage* msg);

      // Inform the TU that whenever a transaction has been terminated. 
      void registerForTransactionTermination();
      
   private:
      TransactionController(const TransactionController& rhs);
      TransactionController& operator=(const TransactionController& rhs);
      
      bool mMultiThreaded;
      bool mStateless;
      bool mRegisteredForTransactionTermination;
      
      // If true, indicate to the Transaction to ignore responses for which
      // there is no transaction. 
      // !jf! Probably should transmit stray responses statelessly. see RFC3261
      bool mDiscardStrayResponses;

      // fifo used to communicate to the transaction state machine within the
      // stack. Not for external use by the application. May contain, sip
      // messages (requests and responses), timers (used by state machines),
      // asynchronous dns responses, transport errors from the underlying
      // transports, etc. 
      // For stateless stacks, this has a different behavior and does not create
      // a transaction for each request and does not do any special transaction
      // processing for requests or responses
      Fifo<Message> mStateMacFifo;

      // from the sipstack (for convenience)
      Fifo<Message>& mTUFifo;

      // Used to decide which transport to send a sip message on. 
      TransportSelector mTransportSelector;

      // stores all of the transactions that are currently active in this stack 
      TransactionMap mClientTransactionMap;
      TransactionMap mServerTransactionMap;

      // Used to handle the stateless stack incoming requests and responses as
      // well as maintaining a state machine for the async dns responses
      StatelessHandler mStatelessHandler;

      // timers associated with the transactions. When a timer fires, it is
      // placed in the mStateMacFifo
      TimerQueue  mTimers;

      unsigned long StatelessIdCounter;

      friend class DnsResolver;
      friend class StatelessHandler;
      friend class TransactionState;
      friend class TransportSelector;

      friend class TestDnsResolver;
      friend class TestFSM;
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
