#ifndef RESIP_StatisticsMessage_hxx
#define RESIP_StatisticsMessage_hxx

#include <iostream>
#include "resip/stack/ApplicationMessage.hxx"
#include "resip/stack/MethodTypes.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/HeapInstanceCounter.hxx"

namespace resip
{
class Subsystem;

class StatisticsMessage : public ApplicationMessage
{
   public:
      RESIP_HeapCount(StatisticsMessage);
      class AtomicPayload;
      StatisticsMessage(const AtomicPayload& payload);
      StatisticsMessage(const StatisticsMessage& rhs);

      virtual ~StatisticsMessage();

      struct Payload
      {
            enum {MaxCode = 700};

            Payload();
            
            unsigned int tuFifoSize;
            unsigned int transportFifoSizeSum;
            unsigned int transactionFifoSize;
            unsigned int activeTimers;
            unsigned int openTcpConnections; // .dlb. not implemented
            unsigned int activeClientTransactions;
            unsigned int activeServerTransactions;
            unsigned int pendingDnsQueries; // .dlb. not implemented

            unsigned int requestsSent; // includes retransmissions
            unsigned int responsesSent; // includes retransmissions
            unsigned int requestsRetransmitted; // counts each retransmission
            unsigned int responsesRetransmitted; // counts each retransmission
            unsigned int requestsReceived;
            unsigned int responsesReceived;

            unsigned int responsesByCode[MaxCode];

            unsigned int requestsSentByMethod[MAX_METHODS];
            unsigned int requestsRetransmittedByMethod[MAX_METHODS];
            unsigned int requestsReceivedByMethod[MAX_METHODS];
            unsigned int responsesSentByMethod[MAX_METHODS];
            unsigned int responsesRetransmittedByMethod[MAX_METHODS];
            unsigned int responsesReceivedByMethod[MAX_METHODS];

            unsigned int responsesSentByMethodByCode[MAX_METHODS][MaxCode];
            unsigned int responsesRetransmittedByMethodByCode[MAX_METHODS][MaxCode];
            unsigned int responsesReceivedByMethodByCode[MAX_METHODS][MaxCode];

            unsigned int sum2xxIn(MethodTypes method) const;
            unsigned int sumErrIn(MethodTypes method) const;
            unsigned int sum2xxOut(MethodTypes method) const;
            unsigned int sumErrOut(MethodTypes method) const;
            void zeroOut();

            Payload& operator=(const Payload& payload);
      };

      void loadOut(Payload& payload) const;
      static void logStats(const Subsystem& subsystem, const Payload& stats);

      virtual EncodeStream& encode(EncodeStream& strm) const;
      virtual EncodeStream& encodeBrief(EncodeStream& str) const;

      Message* clone() const;

      class AtomicPayload : private Payload
      {
         public:
            AtomicPayload();
            void loadIn(const Payload& payload);
            void loadOut(Payload& payload) const;
         private:
            mutable Mutex mMutex;

            // dis-allowed by not implemented
            AtomicPayload(const AtomicPayload&);
            AtomicPayload& operator=(const AtomicPayload&);
      };
      
   private:
      const AtomicPayload& mPayload;
      friend EncodeStream& operator<<(EncodeStream& strm, const StatisticsMessage::Payload& stats);
};

EncodeStream& operator<<(EncodeStream& strm, const StatisticsMessage::Payload& stats);

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
