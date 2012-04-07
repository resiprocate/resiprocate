#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "rutil/Logger.hxx"
#include "resip/stack/StatisticsManager.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/TransactionController.hxx"
#include "resip/stack/SipStack.hxx"

using namespace resip;
using std::vector;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION

StatisticsManager::StatisticsManager(SipStack& stack, unsigned long intervalSecs) 
   : StatisticsMessage::Payload(),
     mStack(stack),
     mInterval(intervalSecs*1000),
     mNextPoll(Timer::getTimeMs() + mInterval),
     mExternalHandler(NULL),
     mPublicPayload(NULL)
{}

StatisticsManager::~StatisticsManager()
{
   if ( mPublicPayload )
       delete mPublicPayload;
}

void 
StatisticsManager::setInterval(unsigned long intervalSecs)
{
   mInterval = intervalSecs * 1000;
}

void 
StatisticsManager::poll()
{
   // get snapshot data now..
   tuFifoSize = mStack.mTransactionController->getTuFifoSize();
   transportFifoSizeSum = mStack.mTransactionController->sumTransportFifoSizes();
   transactionFifoSize = mStack.mTransactionController->getTransactionFifoSize();
   activeTimers = mStack.mTransactionController->getTimerQueueSize();
   activeClientTransactions = mStack.mTransactionController->getNumClientTransactions();
   activeServerTransactions = mStack.mTransactionController->getNumServerTransactions();

   // .kw. At last check payload was > 146kB, which seems too large
   // to alloc on stack. Also, the post'd message has reference
   // to the appStats, so not safe queue as ref to stack element.
   // Converted to dynamic memory allocation.
   if ( mPublicPayload==NULL )
   {
       mPublicPayload = new StatisticsMessage::AtomicPayload;
       // re-used each time, free'd in destructor
   }
   mPublicPayload->loadIn(*this);

   bool postToStack = true;
   StatisticsMessage msg(*mPublicPayload);
   // WATCHOUT: msg contains reference to the payload, and this reference
   // is preserved thru clone().

   if( mExternalHandler )
   {
      postToStack = (*mExternalHandler)(msg);
   }

   if( postToStack )
   {
      // let the app do what it wants with it
      mStack.post(msg);
   }
   
   // !bwc! TODO maybe change this? Or is a flexible implementation of 
   // CongestionManager::logCurrentState() enough?
   if(mStack.mCongestionManager)
   {
      mStack.mCongestionManager->logCurrentState();
   }
}

void 
StatisticsManager::process()
{
   if (Timer::getTimeMs() >= mNextPoll)
   {
      poll();
      mNextPoll += mInterval;
   }
}

bool
StatisticsManager::sent(SipMessage* msg)
{
   MethodTypes met = msg->method();

   if (msg->isRequest())
   {
      ++requestsSent;
      ++requestsSentByMethod[met];
   }
   else if (msg->isResponse())
   {
      int code = msg->const_header(h_StatusLine).statusCode();
      if (code < 0 || code >= MaxCode)
      {
         code = 0;
      }

      ++responsesSent;
      ++responsesSentByMethod[met];
      ++responsesSentByMethodByCode[met][code];
   }
   
   return false;
}

bool 
StatisticsManager::retransmitted(MethodTypes met, 
                                 bool request, 
                                 unsigned int code)
{
   if(request)
   {
      ++requestsRetransmitted;
      ++requestsRetransmittedByMethod[met];
   }
   else
   {
      ++responsesRetransmitted;
      ++responsesRetransmittedByMethod[met];
      ++responsesRetransmittedByMethodByCode[met][code];
   }
   return false;
}

bool
StatisticsManager::received(SipMessage* msg)
{
   MethodTypes met = msg->header(h_CSeq).method();

   if (msg->isRequest())
   {
      ++requestsReceived;
      ++requestsReceivedByMethod[met];
   }
   else if (msg->isResponse())
   {
      ++responsesReceived;
      ++responsesReceivedByMethod[met];
      int code = msg->const_header(h_StatusLine).statusCode();
      if (code < 0 || code >= MaxCode)
      {
         code = 0;
      }
      ++responsesReceivedByMethodByCode[met][code];
   }

   return false;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
