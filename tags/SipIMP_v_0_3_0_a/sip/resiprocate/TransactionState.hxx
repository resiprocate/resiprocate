#if !defined(TRANSACTIONSTATE_HXX)
#define TRANSACTIONSTATE_HXX

#include <iostream>
#include "resiprocate/DnsResolver.hxx"

namespace resip
{

class Message;
class SipMessage;
class DnsMessage;
class SipStack;
class TransactionMap;

class TransactionState
{
   public:
      static void process(SipStack& stack); 
      ~TransactionState();
     
   private:
      typedef enum 
      {
         ClientNonInvite,
         ClientInvite,
         ServerNonInvite,
         ServerInvite,
         Stateless  // may not be needed
      } Machine;
      
      typedef enum 
      {
         Calling,
         Trying,
         Proceeding,
         Completed,
         Confirmed,
         Terminated,
         Bogus
      } State;

      typedef enum
      {
         NotStarted,
         Waiting,
         Complete,
         NoLookupRequired // used for sending responses
      } DnsState;

      
      TransactionState(SipStack& stack, Machine m, State s);
      
      void processDns( Message* msg );
      void processStateless( Message* msg);
      void processClientNonInvite(  Message* msg );
      void processClientInvite(  Message* msg );
      void processServerNonInvite(  Message* msg );
      void processServerInvite(  Message* msg );
      void processStale(  Message* msg );

      void add(const Data& tid);
      void erase(const Data& tid);
      
   private:
      bool isRequest(Message* msg) const;
      bool isInvite(Message* msg) const;
      bool isTimer(Message* msg) const;
      bool isDns(Message* msg) const;
      bool isResponse(Message* msg, int lower=0, int upper=699) const;
      bool isFromTU(Message* msg) const;
      bool isFromWire(Message* msg) const;
      bool isTransportError(Message* msg) const;
      bool isSentReliable(Message* msg) const;
      bool isSentUnreliable(Message* msg) const;
      bool isReliabilityIndication(Message* msg) const;
      bool isSentIndication(Message* msg) const;
      void sendToTU(Message* msg) const;
      void sendToWire(Message* msg);
      void resendToWire(Message* msg) const;
      SipMessage* make100(SipMessage* request) const;
      void terminateClientTransaction(const Data& tid); 
      void terminateServerTransaction(const Data& tid); 
      const Data& tid(SipMessage* sip) const;
      
      static TransactionState* makeCancelTransaction(TransactionState* tran, Machine machine);
      
      SipStack& mStack;
      
      Machine mMachine;
      State mState;

      // Indicates that the message has been sent with a reliable protocol. Set
      // by the TransportSelector
      bool mIsReliable;

      TransactionState* mCancelStateMachine;

      // !rk! The contract for this variable needs to be defined.
      SipMessage* mMsgToRetransmit;

      DnsState mDnsState;
      DnsResolver::TupleList mTuples;
      DnsResolver::TupleIterator mCurrent;
      int mDnsOutstandingQueries;
      
      Transport::Tuple mSource; // used to reply to requests
      Data mId;
      Data mToTag; // for failure responses on ServerInviteTransaction 

      // this shouldn't be static since there can be more than one SipStack in
      // an app. Should be stored in the SipStack
      static unsigned long StatelessIdCounter;
      
      friend std::ostream& operator<<(std::ostream& strm, const TransactionState& state);
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
