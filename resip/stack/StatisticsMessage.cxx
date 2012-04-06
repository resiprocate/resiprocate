#include "resip/stack/StatisticsMessage.hxx"
#include "rutil/Lock.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

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

EncodeStream&
StatisticsMessage::encodeBrief(EncodeStream& str) const 
{
   return str << "StatisticsMessage";
}

EncodeStream& 
StatisticsMessage::encode(EncodeStream& strm) const 
{
   strm << "StatisticsMessage";
/*
   strm << " [";
   Payload payload;
   mPayload.loadOut(payload);
   strm << payload << "]";
*/
   return strm;
}

unsigned int
StatisticsMessage::Payload::sum2xxIn(MethodTypes method) const
{
   unsigned int ret = 0;
   for (int code = 200; code < 300; ++code)
   {
      ret += responsesReceivedByMethodByCode[method][code];
   }

   return ret;
}

unsigned int
StatisticsMessage::Payload::sum2xxOut(MethodTypes method) const
{
   unsigned int ret = 0;
   for (int code = 200; code < 300; ++code)
   {
      ret += responsesSentByMethodByCode[method][code];
   }

   return ret;
}

unsigned int
StatisticsMessage::Payload::sumErrIn(MethodTypes method) const
{
   unsigned int ret = 0;
   for (int code = 300; code < MaxCode; ++code)
   {
      ret += responsesReceivedByMethodByCode[method][code];
   }

   return ret;
}

unsigned int
StatisticsMessage::Payload::sumErrOut(MethodTypes method) const
{
   unsigned int ret = 0;
   for (int code = 300; code < MaxCode; ++code)
   {
      ret += responsesSentByMethodByCode[method][code];
   }

   return ret;
}

void 
StatisticsMessage::logStats(const resip::Subsystem& subsystem, 
                            const StatisticsMessage::Payload& stats)
{
   WarningLog(<< subsystem
              << std::endl
              << stats);
}


Message*
StatisticsMessage::clone() const
{
   return new StatisticsMessage(*this);
}

StatisticsMessage::Payload::Payload()
{
   zeroOut();
}

void
StatisticsMessage::Payload::zeroOut()
{
   tuFifoSize = 0;
   transportFifoSizeSum = 0;
   transactionFifoSize = 0;
   activeTimers = 0;
   openTcpConnections = 0;
   activeClientTransactions = 0;
   activeServerTransactions = 0;
   pendingDnsQueries = 0;
   requestsSent = 0;
   responsesSent = 0;
   requestsRetransmitted = 0;
   responsesRetransmitted = 0;
   requestsReceived = 0;
   responsesReceived = 0;
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

EncodeStream& 
resip::operator<<(EncodeStream& strm, const StatisticsMessage::Payload& stats)
{
   unsigned int retriesFinal = 0;
   for (int c = 200; c < 300; ++c)
   {
      retriesFinal += stats.responsesRetransmittedByMethodByCode[INVITE][c];
   }

   unsigned int retriesNonFinal = 0;      
   for (int c = 100; c < 200; ++c)
   {
      retriesNonFinal += stats.responsesRetransmittedByMethodByCode[INVITE][c];
   }

   strm << "TU summary: " << stats.tuFifoSize
        << " TRANSPORT " << stats.transportFifoSizeSum
        << " TRANSACTION " << stats.transactionFifoSize
        << " CLIENTTX " << stats.activeClientTransactions
        << " SERVERTX " << stats.activeServerTransactions
        << " TIMERS " << stats.activeTimers
        << std::endl
        << "Transaction summary: reqi " << stats.requestsReceived
        << " reqo " << stats.requestsSent
        << " rspi " << stats.responsesReceived
        << " rspo " << stats.responsesSent
        << std::endl
        << "Details: INVi " << stats.requestsReceivedByMethod[INVITE] << "/S" << stats.sum2xxOut(INVITE) << "/F" << stats.sumErrOut(INVITE)
        << " INVo " << stats.requestsSentByMethod[INVITE]-stats.requestsRetransmittedByMethod[INVITE] << "/S" << stats.sum2xxIn(INVITE) << "/F" << stats.sumErrIn(INVITE)
        << " ACKi " << stats.requestsReceivedByMethod[ACK]
        << " ACKo " << stats.requestsSentByMethod[ACK]-stats.requestsRetransmittedByMethod[ACK]
        << " BYEi " << stats.requestsReceivedByMethod[BYE] << "/S" << stats.sum2xxOut(BYE) << "/F" << stats.sumErrOut(BYE)
        << " BYEo " << stats.requestsSentByMethod[BYE]-stats.requestsRetransmittedByMethod[BYE] << "/S" << stats.sum2xxIn(BYE) << "/F" << stats.sumErrIn(BYE)
        << " CANi " << stats.requestsReceivedByMethod[CANCEL] << "/S" << stats.sum2xxOut(BYE) << "/F" << stats.sumErrOut(BYE)
        << " CANo " << stats.requestsSentByMethod[CANCEL]-stats.requestsRetransmittedByMethod[CANCEL] << "/S" << stats.sum2xxIn(CANCEL) << "/F" << stats.sumErrIn(CANCEL)
        << " MSGi " << stats.requestsReceivedByMethod[MESSAGE] << "/S" << stats.sum2xxOut(MESSAGE) << "/F" << stats.sumErrOut(MESSAGE)
        << " MSGo " << stats.requestsSentByMethod[MESSAGE]-stats.requestsRetransmittedByMethod[MESSAGE] << "/S" << stats.sum2xxIn(MESSAGE) << "/F" << stats.sumErrIn(MESSAGE)
        << " OPTi " << stats.requestsReceivedByMethod[OPTIONS] << "/S" << stats.sum2xxOut(OPTIONS) << "/F" << stats.sumErrOut(OPTIONS)
        << " OPTo " << stats.requestsSentByMethod[OPTIONS]-stats.requestsRetransmittedByMethod[OPTIONS] << "/S" << stats.sum2xxIn(OPTIONS) << "/F" << stats.sumErrIn(OPTIONS)
        << " REGi " << stats.requestsReceivedByMethod[REGISTER] << "/S" << stats.sum2xxOut(REGISTER) << "/F" << stats.sumErrOut(REGISTER)
        << " REGo " << stats.requestsSentByMethod[REGISTER]-stats.requestsRetransmittedByMethod[REGISTER] << "/S" << stats.sum2xxIn(REGISTER) << "/F" << stats.sumErrIn(REGISTER)
        << " PUBi " << stats.requestsReceivedByMethod[PUBLISH] << "/S" << stats.sum2xxOut(PUBLISH) << "/F" << stats.sumErrOut(PUBLISH)
        << " PUBo " << stats.requestsSentByMethod[PUBLISH] << "/S" << stats.sum2xxIn(PUBLISH) << "/F" << stats.sumErrIn(PUBLISH)
        << " SUBi " << stats.requestsReceivedByMethod[SUBSCRIBE] << "/S" << stats.sum2xxOut(SUBSCRIBE) << "/F" << stats.sumErrOut(SUBSCRIBE)
        << " SUBo " << stats.requestsSentByMethod[SUBSCRIBE] << "/S" << stats.sum2xxIn(SUBSCRIBE) << "/F" << stats.sumErrIn(SUBSCRIBE)
        << " NOTi " << stats.requestsReceivedByMethod[NOTIFY] << "/S" << stats.sum2xxOut(NOTIFY) << "/F" << stats.sumErrOut(NOTIFY)
        << " NOTo " << stats.requestsSentByMethod[NOTIFY] << "/S" << stats.sum2xxIn(NOTIFY) << "/F" << stats.sumErrIn(NOTIFY)
        << " REFi " << stats.requestsReceivedByMethod[REFER] << "/S" << stats.sum2xxOut(REFER) << "/F" << stats.sumErrOut(REFER)
        << " REFo " << stats.requestsSentByMethod[REFER] << "/S" << stats.sum2xxIn(REFER) << "/F" << stats.sumErrIn(REFER)
        << " INFi " << stats.requestsReceivedByMethod[INFO] << "/S" << stats.sum2xxOut(INFO) << "/F" << stats.sumErrOut(INFO)
        << " INFo " << stats.requestsSentByMethod[INFO] << "/S" << stats.sum2xxIn(INFO) << "/F" << stats.sumErrIn(INFO)
        << " PRAi " << stats.requestsReceivedByMethod[PRACK] << "/S" << stats.sum2xxOut(PRACK) << "/F" << stats.sumErrOut(PRACK)
        << " PRAo " << stats.requestsSentByMethod[PRACK] << "/S" << stats.sum2xxIn(PRACK) << "/F" << stats.sumErrIn(PRACK)
        << " SERi " << stats.requestsReceivedByMethod[SERVICE] << "/S" << stats.sum2xxOut(SERVICE) << "/F" << stats.sumErrOut(SERVICE)
        << " SERo " << stats.requestsSentByMethod[SERVICE] << "/S" << stats.sum2xxIn(SERVICE) << "/F" << stats.sumErrIn(SERVICE)
        << " UPDi " << stats.requestsReceivedByMethod[UPDATE] << "/S" << stats.sum2xxOut(UPDATE) << "/F" << stats.sumErrOut(UPDATE)
        << " UPDo " << stats.requestsSentByMethod[UPDATE] << "/S" << stats.sum2xxIn(UPDATE) << "/F" << stats.sumErrIn(UPDATE)
        << std::endl
        << "Retransmissions: INVx " << stats.requestsRetransmittedByMethod[INVITE]
        << " finx " << retriesFinal
        << " nonx " << retriesNonFinal
        << " BYEx " << stats.requestsRetransmittedByMethod[BYE]
        << " CANx " << stats.requestsRetransmittedByMethod[CANCEL]
        << " MSGx " << stats.requestsRetransmittedByMethod[MESSAGE]
        << " OPTx " << stats.requestsRetransmittedByMethod[OPTIONS]
        << " REGx " << stats.requestsRetransmittedByMethod[REGISTER]
        << " PUBx " << stats.requestsRetransmittedByMethod[PUBLISH]
        << " SUBx " << stats.requestsRetransmittedByMethod[SUBSCRIBE]
        << " NOTx " << stats.requestsRetransmittedByMethod[NOTIFY]
        << " REFx " << stats.requestsRetransmittedByMethod[REFER]
        << " INFx " << stats.requestsRetransmittedByMethod[INFO]
        << " PRAx " << stats.requestsRetransmittedByMethod[PRACK]
        << " SERx " << stats.requestsRetransmittedByMethod[SERVICE]
        << " UPDx " << stats.requestsRetransmittedByMethod[UPDATE];
   strm.flush();
   return strm;
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
