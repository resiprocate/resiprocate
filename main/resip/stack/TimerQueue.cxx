
#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include <cassert>
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

TimerQueue::TimerQueue(Fifo<TransactionMessage>& fifo)
   : mFifo(fifo)
{
}

#ifdef USE_DTLS

DtlsTimerQueue::DtlsTimerQueue( Fifo<DtlsMessage>& fifo )
    : mFifo( fifo )
{
}

#endif

BaseTimerQueue::~BaseTimerQueue()
{
   //xkd-2004-11-4
   // delete the message associated with the timer
   for (std::multiset<Timer>::iterator i = mTimers.begin(); i !=  mTimers.end(); ++i)
   {
      if (i->getMessage())
      {
         delete i->getMessage();
      }
   }
}

unsigned int
BaseTimerQueue::msTillNextTimer()
{
   if (!mTimers.empty())
   {
      UInt64 next = mTimers.begin()->mWhen;
      UInt64 now = Timer::getTimeMs();
      if (now > next) 
      {
         return 0;
      }
      else
      {
         UInt64 ret64 = next - now;
         if ( ret64 > UInt64(INT_MAX) )
         {
            return INT_MAX;
         }
         else
         { 
            int ret = int(ret64);
            return ret;
         }
      }
   }
   else
   {
      return INT_MAX;
   }
}

Timer::Id
TimerQueue::add(Timer::Type type, const Data& transactionId, unsigned long msOffset)
{
   Timer t(msOffset, type, transactionId);
   mTimers.insert(t);
   DebugLog (<< "Adding timer: " << Timer::toData(type) << " tid=" << transactionId << " ms=" << msOffset);
   
   return t.getId();
}

#ifdef USE_DTLS

void
DtlsTimerQueue::add( SSL *ssl, unsigned long msOffset )
{
   Timer t( msOffset, new DtlsMessage( ssl ) ) ;
   mTimers.insert( t ) ;
}

#endif

void
BaseTimeLimitTimerQueue::add(const Timer& timer)
{
   assert(timer.getMessage());
   DebugLog(<< "Adding application timer: " << timer.getMessage()->brief());
   mTimers.insert(timer);
}

int
BaseTimerQueue::size() const
{
   return mTimers.size();
}

bool
BaseTimerQueue::empty() const
{
   return mTimers.empty();
}

void
BaseTimeLimitTimerQueue::process()
{
   // get the set of timers that have fired and insert TimerMsg into the state
   // machine fifo and application messages into the TU fifo

   if (!mTimers.empty() && msTillNextTimer() == 0)
   {
      Timer now(0);
      std::multiset<Timer>::iterator end = mTimers.upper_bound(now);
      for (std::multiset<Timer>::iterator i = mTimers.begin(); i != end; ++i)
      {
         assert(i->getMessage());
         addToFifo(i->getMessage(), TimeLimitFifo<Message>::InternalElement);
      }
      mTimers.erase(mTimers.begin(), end);
   }
}

void
TimerQueue::process()
{
   // get the set of timers that have fired and insert TimerMsg into the state
   // machine fifo and application messages into the TU fifo

   if (!mTimers.empty() && msTillNextTimer() == 0)
   {
      Timer now(0);
      std::multiset<Timer>::iterator end = mTimers.upper_bound(now);
      for (std::multiset<Timer>::iterator i = mTimers.begin(); i != end; ++i)
      {
         mFifo.add(new TimerMessage(i->mTransactionId, i->mType, i->mDuration));
      }
      mTimers.erase(mTimers.begin(), end);
   }
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

void
TuSelectorTimerQueue::addToFifo(Message*msg, TimeLimitFifo<Message>::DepthUsage d)
{
   mFifoSelector.add(msg, d);
}
   

#ifdef USE_DTLS

void
DtlsTimerQueue::process()
{
   // get the set of timers that have fired and insert TimerMsg into the state
   // machine fifo and application messages into the TU fifo
   
   if (!mTimers.empty() && msTillNextTimer() == 0)
   {
      Timer now(0);
      std::multiset<Timer>::iterator end = mTimers.upper_bound(now);
      for (std::multiset<Timer>::iterator i = mTimers.begin(); i != end; ++i)
      {
          mFifo.add( (DtlsMessage *)i->getMessage() ) ;
      }
      mTimers.erase( mTimers.begin(), end );
   }
}

#endif

std::ostream& 
resip::operator<<(std::ostream& str, const BaseTimerQueue& tq)
{
   str << "TimerQueue[" ;

    for (std::multiset<Timer>::const_iterator i = tq.mTimers.begin(); 
        i != tq.mTimers.end(); ++i)
   {
      str << *i << " " ;
   }

   str << "]" << endl;
   return str;
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
