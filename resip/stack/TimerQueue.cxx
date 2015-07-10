
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "rutil/ResipAssert.h"
#include <limits.h>

#include "resip/stack/TimerQueue.hxx"
#include "resip/stack/TimerMessage.hxx"
#include "resip/stack/TransactionMessage.hxx"
#include "resip/stack/TuSelector.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION

TransactionTimerQueue::TransactionTimerQueue(Fifo<TimerMessage>& fifo)
   : mFifo(fifo)
{
}

#ifdef USE_DTLS

DtlsTimerQueue::DtlsTimerQueue( Fifo<DtlsMessage>& fifo )
    : mFifo( fifo )
{
}

DtlsTimerQueue::~DtlsTimerQueue()
{
   while(!mTimers.empty())
   {
      delete mTimers.top().getMessage();
      mTimers.pop();
   }
}

#endif

UInt64
TransactionTimerQueue::add(Timer::Type type, const Data& transactionId, unsigned long msOffset)
{
   TransactionTimer t(msOffset, type, transactionId);
   mTimers.push(t);
   DebugLog (<< "Adding timer: " << Timer::toData(type) << " tid=" << transactionId << " ms=" << msOffset);
   return mTimers.top().getWhen();
}

#ifdef USE_DTLS

UInt64
DtlsTimerQueue::add( SSL *ssl, unsigned long msOffset )
{
   TimerWithPayload t( msOffset, new DtlsMessage( ssl ) ) ;
   mTimers.push( t ) ;
   return mTimers.top().getWhen();
}

#endif

BaseTimeLimitTimerQueue::~BaseTimeLimitTimerQueue()
{
   while(!mTimers.empty())
   {
      delete mTimers.top().getMessage();
      mTimers.pop();
   }
}

UInt64
BaseTimeLimitTimerQueue::add(unsigned int timeMs,Message* payload)
{
   resip_assert(payload);
   DebugLog(<< "Adding application timer: " << payload->brief() << " ms=" << timeMs);
   mTimers.push(TimerWithPayload(timeMs,payload));
   return mTimers.top().getWhen();
}

void
BaseTimeLimitTimerQueue::processTimer(const TimerWithPayload& timer)
{
   resip_assert(timer.getMessage());
   addToFifo(timer.getMessage(), TimeLimitFifo<Message>::InternalElement);
}

void
TransactionTimerQueue::processTimer(const TransactionTimer& timer)
{
   mFifo.add(new TimerMessage(timer.getTransactionId(), 
                              timer.getType(), 
                              timer.getDuration()));
}

TimeLimitTimerQueue::TimeLimitTimerQueue(TimeLimitFifo<Message>& fifo) : mFifo(fifo)
{}

void
TimeLimitTimerQueue::addToFifo(Message*msg, TimeLimitFifo<Message>::DepthUsage d)
{
   mFifo.add(msg, d);
}

TuSelectorTimerQueue::TuSelectorTimerQueue(TuSelector& sel) : mFifoSelector(sel)
{}

TuSelectorTimerQueue::~TuSelectorTimerQueue()
{
   while(!mTimers.empty())
   {
      delete mTimers.top().getMessage();
      mTimers.pop();
   }
}

UInt64
TuSelectorTimerQueue::add(unsigned int timeMs,Message* payload)
{
   resip_assert(payload);
   DebugLog(<< "Adding application timer: " << payload->brief() << " ms=" << timeMs);
   mTimers.push(TimerWithPayload(timeMs,payload));
   return mTimers.top().getWhen();
}

void
TuSelectorTimerQueue::processTimer(const TimerWithPayload& timer)
{
   mFifoSelector.add(timer.getMessage(), 
                        TimeLimitFifo<Message>::InternalElement);
}

#ifdef USE_DTLS

void
DtlsTimerQueue::processTimer(const TimerWithPayload& timer)
{
   mFifo.add( (DtlsMessage *)timer.getMessage() ) ;
}

#endif

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
