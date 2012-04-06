#ifndef RESIP_StatisticsManager_hxx
#define RESIP_StatisticsManager_hxx

#include "rutil/Timer.hxx"
#include "rutil/Data.hxx"
#include "resip/stack/StatisticsMessage.hxx"
#include "resip/stack/StatisticsHandler.hxx"

namespace resip
{
class SipStack;
class SipMessage;
class TransactionController;

/**
   @brief Keeps track of various statistics on the stack's operation, and 
      periodically issues a StatisticsMessage to the TransactionUser (or, if the
      ExternalStatsHandler is set, it will be sent there).
*/
class StatisticsManager : public StatisticsMessage::Payload
{
   public:
      // not implemented
      typedef enum
      {
         TransportFifoSize,
         TUFifoSize,
         ActiveTimers,
         OpenTcpConnections,
         ActiveClientTransactions,
         ActiveServerTransactions,
         PendingDnsQueries,
         StatsMemUsed
      } Measurement;
      
      StatisticsManager(SipStack& stack, unsigned long intervalSecs=60);
      ~StatisticsManager();

      void process();
      // not stricly thread-safe; needs to be called through the fifo somehow
      void setInterval(unsigned long intvSecs);

      /**
         @ingroup resip_config
         @brief Allows the application to set the ExternalStatsHandler for this
            StatisticsManager.
      */
      void setExternalStatsHandler(ExternalStatsHandler *handler)
      {
         mExternalHandler = handler;
      }

   private:
      friend class TransactionState;
      bool sent(SipMessage* msg);
      bool retransmitted(MethodTypes type, bool request, unsigned int code);
      bool received(SipMessage* msg);

      void poll(); // force an update

      SipStack& mStack;
      UInt64 mInterval;
      UInt64 mNextPoll;

      ExternalStatsHandler *mExternalHandler;
      //
      // When statistics are published, a copy of values are made
      // and copied into this member, and then reference to this is
      // published thru both ExternalHandler and posted to stack as message.
      // This payload is mutex protected.
      StatisticsMessage::AtomicPayload *mPublicPayload;
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
