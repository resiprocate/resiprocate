#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif


#include <limits.h>

#include "resiprocate/os/Socket.hxx"

#include "resiprocate/TimerQueue.hxx"
#include "resiprocate/TimerMessage.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION

TimerQueue::TimerQueue(Fifo<Message>& fifo) : mFifo(fifo)
{
}

unsigned int
TimerQueue::msTillNextTimer()
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
         int ret = int(ret64);
         return ret;
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

void
TimerQueue::process()
{
   // get the set of timers that have fired and insert TimerMsg into the fifo

   Timer now(0);
   for (std::multiset<Timer>::iterator i = mTimers.begin(); 
        i != mTimers.upper_bound(now);)
   {
      TimerMessage* t = new TimerMessage(i->mTransactionId, i->mType, i->mDuration);
      // Leaked !ah! BUGBUG valgrind says leaked.
      //  206884 bytes in 2270 blocks are definitely lost (...)
      //     by 0x8178A20: operator new(unsigned)
      //     by 0x80CDF75: resip::TimerQueue::process() (TimerQueue.cxx:63)
      //     by 0x80F6A4B: resip::Executive::processTimer() (Executive.cxx:52)

      //DebugLog (<< Timer::toData(i->mType) << " fired (" << i->mTransactionId << ") adding to fifo");
      mFifo.add(t);
      mTimers.erase(i++);
   }
}

void
TimerQueue::run()
{
   assert(0);
   // for the thread
}

ostream& 
resip::operator<<(ostream& str, const TimerQueue& tq)
{
   str << "TimerQueue[" ;

    for (std::multiset<Timer>::const_iterator i = tq.mTimers.begin(); 
        i != tq.mTimers.end();
		i++)
   {
      str << *i << " " ;
   }

   str << "]" << endl;
   return str;
}


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
