#include "resiprocate/StatisticsMessage.hxx"
#include "resiprocate/os/Lock.hxx"
#include "resiprocate/os/Logger.hxx"

#include <string.h>

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::STATS

StatisticsMessage::StatisticsMessage(const StatisticsMessage::AtomicPayload& payload)
   : ApplicationMessage(),
     mPayload(payload)
{}

StatisticsMessage::StatisticsMessage(const StatisticsMessage& rhs)
   : ApplicationMessage(rhs),
     mPayload(rhs.mPayload)
{}

StatisticsMessage::~StatisticsMessage()
{}

Data 
StatisticsMessage::brief() const 
{
   return "StatisticsMessage";
}

std::ostream& 
StatisticsMessage::encode(std::ostream& strm) const 
{
   strm << "StatisticsMessage[";
/*
   Payload payload;
   mPayload.loadOut(payload);
   strm << payload << "]";
*/
   return strm;
}

void 
StatisticsMessage::logStats(const resip::Subsystem& subsystem, 
                            const StatisticsMessage::Payload& stats)
{
   unsigned int retriesFinal = 0;
   for (int c = 200; c < 300; ++c)
   {
      retriesFinal += stats.responsesRetransmittedByMethodByCode[RESIP_INVITE][c];
   }

   unsigned int retriesNonFinal = 0;      
   for (int c = 100; c < 200; ++c)
   {
      retriesNonFinal += stats.responsesRetransmittedByMethodByCode[RESIP_INVITE][c];
   }

   WarningLog(<< subsystem
              << " TU " << stats.tuFifoSize
              << " TRANSPORT " << stats.transportFifoSizeSum
              << " TRANSACTION " << stats.transactionFifoSize
              << " CLIENTTX " << stats.activeClientTransactions
              << " SERVERTX " << stats.activeServerTransactions
              << " TIMERS " << stats.activeTimers

              << " reqi " << stats.requestsReceived
              << " reqo " << stats.requestsSent
              << " rspi " << stats.responsesReceived
              << " rspo " << stats.responsesSent
              << " INVi " << stats.requestsReceivedByMethod[RESIP_INVITE]
              << " INVo " << stats.requestsSentByMethod[RESIP_INVITE]-stats.requestsRetransmittedByMethod[RESIP_INVITE]
              << " ACKi " << stats.requestsReceivedByMethod[RESIP_ACK]
              << " ACKo " << stats.requestsSentByMethod[RESIP_ACK]-stats.requestsRetransmittedByMethod[RESIP_ACK]
              << " BYEi " << stats.requestsReceivedByMethod[RESIP_BYE]
              << " BYEo " << stats.requestsSentByMethod[RESIP_BYE]-stats.requestsRetransmittedByMethod[RESIP_BYE]
              << " CANi " << stats.requestsReceivedByMethod[RESIP_CANCEL]
              << " CANo " << stats.requestsSentByMethod[RESIP_CANCEL]-stats.requestsRetransmittedByMethod[RESIP_CANCEL]
              << " OPTi " << stats.requestsReceivedByMethod[RESIP_OPTIONS]
              << " OPTo " << stats.requestsSentByMethod[RESIP_OPTIONS]-stats.requestsRetransmittedByMethod[RESIP_OPTIONS]
              << " REGi " << stats.requestsReceivedByMethod[RESIP_REGISTER]
              << " REGo " << stats.requestsSentByMethod[RESIP_REGISTER]-stats.requestsRetransmittedByMethod[RESIP_REGISTER]
              << " PUBi " << stats.requestsReceivedByMethod[RESIP_PUBLISH]
              << " PUBo " << stats.requestsSentByMethod[RESIP_PUBLISH]
              << " SUBi " << stats.requestsReceivedByMethod[RESIP_SUBSCRIBE]
              << " SUBo " << stats.requestsSentByMethod[RESIP_SUBSCRIBE]
              << " NOTi " << stats.requestsReceivedByMethod[RESIP_NOTIFY]
              << " NOTo " << stats.requestsSentByMethod[RESIP_NOTIFY]
              << " INVx " << stats.requestsRetransmittedByMethod[RESIP_INVITE]
              << " BYEx " << stats.requestsRetransmittedByMethod[RESIP_BYE]
              << " CANx " << stats.requestsRetransmittedByMethod[RESIP_CANCEL]
              << " OPTx " << stats.requestsRetransmittedByMethod[RESIP_OPTIONS]
              << " REGx " << stats.requestsRetransmittedByMethod[RESIP_REGISTER]
              << " finx " << retriesFinal
              << " nonx " << retriesNonFinal
              << " PUBx " << stats.requestsRetransmittedByMethod[RESIP_PUBLISH]
              << " SUBx " << stats.requestsRetransmittedByMethod[RESIP_SUBSCRIBE]
              << " NOTx " << stats.requestsRetransmittedByMethod[RESIP_NOTIFY]);
}


Message*
StatisticsMessage::clone() const
{
   return new StatisticsMessage(*this);
}

StatisticsMessage::Payload::Payload()
   : tuFifoSize(0),
     transportFifoSizeSum(0),
     transactionFifoSize(0),
     activeTimers(0),
     openTcpConnections(0),
     activeClientTransactions(0),
     activeServerTransactions(0),
     pendingDnsQueries(0),
     requestsSent(0),
     responsesSent(0),
     requestsRetransmitted(0),
     responsesRetransmitted(0),
     requestsReceived(0),
     responsesReceived(0)
{
   memset(responsesByCode, 0, sizeof(responsesByCode));
   memset(requestsSentByMethod, 0, sizeof(requestsSentByMethod));
   memset(requestsRetransmittedByMethod, 0, sizeof(requestsRetransmittedByMethod));
   memset(requestsReceivedByMethod, 0, sizeof(requestsReceivedByMethod));
   memset(responsesSentByMethod, 0, sizeof(responsesSentByMethod));
   memset(responsesRetransmittedByMethod, 0, sizeof(responsesRetransmittedByMethod));
   memset(responsesReceivedByMethod, 0, sizeof(responsesReceivedByMethod));
   memset(responsesSentByMethodByCode, 0, sizeof(responsesSentByMethodByCode));
   memset(responsesRetransmittedByMethodByCode, 0, sizeof(responsesRetransmittedByMethodByCode));
   memset(responsesReceivedByMethodByCode, 0, sizeof(responsesReceivedByMethodByCode));
}

StatisticsMessage::Payload&
StatisticsMessage::Payload::operator=(const StatisticsMessage::Payload& rhs)
{
   if (&rhs != this)
   {
      transportFifoSizeSum = rhs.transportFifoSizeSum;
      tuFifoSize = rhs.tuFifoSize;
      activeTimers = rhs.activeTimers;
      transactionFifoSize = rhs.transactionFifoSize;

      openTcpConnections = rhs.openTcpConnections;
      activeClientTransactions = rhs.activeClientTransactions;
      activeServerTransactions = rhs.activeServerTransactions;
      pendingDnsQueries = rhs.pendingDnsQueries;

      requestsSent = rhs.requestsSent;
      responsesSent = rhs.responsesSent;
      requestsRetransmitted = rhs.requestsRetransmitted;
      responsesRetransmitted = rhs.responsesRetransmitted;
      requestsReceived = rhs.requestsReceived;
      responsesReceived = rhs.responsesReceived;

      memcpy(responsesByCode, rhs.responsesByCode, sizeof(responsesByCode));
      memcpy(requestsSentByMethod, rhs.requestsSentByMethod, sizeof(requestsSentByMethod));
      memcpy(requestsRetransmittedByMethod, rhs.requestsRetransmittedByMethod, sizeof(requestsRetransmittedByMethod));
      memcpy(requestsReceivedByMethod, rhs.requestsReceivedByMethod, sizeof(requestsReceivedByMethod));
      memcpy(responsesSentByMethod, rhs.responsesSentByMethod, sizeof(responsesSentByMethod));
      memcpy(responsesRetransmittedByMethod, rhs.responsesRetransmittedByMethod, sizeof(responsesRetransmittedByMethod));
      memcpy(responsesReceivedByMethod, rhs.responsesReceivedByMethod, sizeof(responsesReceivedByMethod));
      memcpy(responsesSentByMethodByCode, rhs.responsesSentByMethodByCode, sizeof(responsesSentByMethodByCode));
      memcpy(responsesRetransmittedByMethodByCode, rhs.responsesRetransmittedByMethodByCode, sizeof(responsesRetransmittedByMethodByCode));
      memcpy(responsesReceivedByMethodByCode, rhs.responsesReceivedByMethodByCode, sizeof(responsesReceivedByMethodByCode));
   }

   return *this;
}

void 
StatisticsMessage::loadOut(Payload& payload) const
{
   mPayload.loadOut(payload);
}

StatisticsMessage::AtomicPayload::AtomicPayload()
{}

void
StatisticsMessage::AtomicPayload::loadIn(const Payload& payload)
{
   Lock lock(mMutex);
   Payload::operator=(payload);
}

void
StatisticsMessage::AtomicPayload::loadOut(Payload& payload) const
{
   Lock lock(mMutex);
   payload = (*this);
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2004 Vovida Networks, Inc.  All rights reserved.
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
