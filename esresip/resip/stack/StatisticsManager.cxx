/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
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
     mExternalHandler(NULL)
{}

void 
StatisticsManager::setInterval(unsigned long intervalSecs)
{
   mInterval = intervalSecs * 1000;
}

void 
StatisticsManager::poll()
{
   // get snapshot data now..
   tuFifoSize = mStack.mTransactionController.getTuFifoSize();
   transportFifoSizeSum = mStack.mTransactionController.sumTransportFifoSizes();
   transactionFifoSize = mStack.mTransactionController.getTransactionFifoSize();
   activeTimers = mStack.mTransactionController.getTimerQueueSize();
   activeClientTransactions = mStack.mTransactionController.getNumClientTransactions();
   activeServerTransactions = mStack.mTransactionController.getNumServerTransactions();   

   StatisticsMessage::AtomicPayload appStats;
   appStats.loadIn(*this);

   bool postToStack = true;
   StatisticsMessage msg(appStats);

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
   static int count=0;
   if (count%128==0 && Timer::getTimeMs() >= mNextPoll)
   {
      count=0;
      poll();
      mNextPoll += mInterval;
   }
   ++count;
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
      ++responsesSentByCode[code];
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
      ++responsesRetransmittedByCode[code];
   }
   return false;
}

bool
StatisticsManager::received(SipMessage* msg)
{
   MethodTypes met=msg->method();

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
      ++responsesReceivedByCode[code];
   }

   return false;
}

/* Copyright 2007 Estacado Systems */

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
