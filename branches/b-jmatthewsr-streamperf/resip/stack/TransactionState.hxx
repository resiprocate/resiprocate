#if !defined(RESIP_TRANSACTIONSTATE_HXX)
#define RESIP_TRANSACTIONSTATE_HXX

#include <iosfwd>
#include "rutil/dns/DnsHandler.hxx"
#include "resip/stack/Transport.hxx"
#include "rutil/HeapInstanceCounter.hxx"

namespace resip
{

class DnsResult;
class TransactionMessage;
class SipMessage;
class TransactionMap;
class TransactionController;
class TransactionUser;

class TransactionState : public DnsHandler
{
   public:
      RESIP_HeapCount(TransactionState);
      static void process(TransactionController& controller); 
      ~TransactionState();
     
   private:
      typedef enum 
      {
         ClientNonInvite,
         ClientInvite,
         ServerNonInvite,
         ServerInvite,
         ClientStale,
         ServerStale,
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

      TransactionState(TransactionController& controller, 
                       Machine m, 
                       State s, 
                       const Data& tid, 
                       TransactionUser* tu=0);
      
      void rewriteRequest(const Uri& rewrite);
      void handle(DnsResult*);

      void processStateless(TransactionMessage* msg);
      void processClientNonInvite(TransactionMessage* msg);
      void processClientInvite(TransactionMessage* msg);
      void processServerNonInvite(TransactionMessage* msg);
      void processServerInvite(TransactionMessage* msg);
      void processClientStale(TransactionMessage* msg);
      void processServerStale(TransactionMessage* msg);
      void processTransportFailure(TransactionMessage* failure);
      void processNoDnsResults();
      void processReliability(TransportType type);
      
      void add(const Data& tid);
      void erase(const Data& tid);
      
   private:
      bool isRequest(TransactionMessage* msg) const;
      bool isInvite(TransactionMessage* msg) const;
      bool isTimer(TransactionMessage* msg) const;
      bool isResponse(TransactionMessage* msg, int lower=0, int upper=699) const;
      bool isFromTU(TransactionMessage* msg) const;
      bool isFromWire(TransactionMessage* msg) const;
      bool isTransportError(TransactionMessage* msg) const;
      bool isSentReliable(TransactionMessage* msg) const;
      bool isSentUnreliable(TransactionMessage* msg) const;
      bool isReliabilityIndication(TransactionMessage* msg) const;
      bool isSentIndication(TransactionMessage* msg) const;
      void sendToTU(TransactionMessage* msg) const;
      static void sendToTU(TransactionUser* tu, TransactionController& controller, TransactionMessage* msg);
      void sendToWire(TransactionMessage* msg, bool retransmit=false);
      SipMessage* make100(SipMessage* request) const;
      void terminateClientTransaction(const Data& tid); 
      void terminateServerTransaction(const Data& tid); 
      const Data& tid(SipMessage* sip) const;

      void startServerNonInviteTimerTrying(SipMessage& sip, Data& tid);

      static TransactionState* makeCancelTransaction(TransactionState* tran, Machine machine, const Data& tid);
      
      /**
         Attempts to responds to a malformed non-ACK request.
         @param badReq MUST be a non-ACK request. This function will assert
            if otherwise.
         @return true iff a response was successfully sent.
      **/
      static bool handleBadRequest(const resip::SipMessage& badReq,TransactionController& controller);
      
      TransactionController& mController;
      
      Machine mMachine;
      State mState;
      bool mIsCancel;
      
      // Indicates that the message has been sent with a reliable protocol. Set
      // by the TransportSelector
      bool mIsReliable;

      // !rk! The contract for this variable needs to be defined.
      SipMessage* mMsgToRetransmit;

      // Handle to the dns results queried by the TransportSelector
      DnsResult* mDnsResult;

      // current selection from the DnsResult. e.g. it is important to send the
      // CANCEL to exactly the same tuple as the original INVITE went to. 
      Tuple mTarget; 
      Tuple mResponseTarget; // used to reply to requests

      Data mId;
      Data mToTag; // for failure responses on ServerInviteTransaction 
      TransactionUser* mTransactionUser;
      TransportFailure::FailureReason mFailureReason;      

      static unsigned long StatelessIdCounter;
      
      friend EncodeStream& operator<<(EncodeStream& strm, const TransactionState& state);
      friend class TransactionController;
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
