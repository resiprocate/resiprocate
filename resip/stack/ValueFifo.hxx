#ifndef ValueFifo_hxx
#define ValueFifo_hxx

#include <cerrno>
#include <iosfwd>
#include <deque>

#include "rutil/Condition.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/Lock.hxx"
#include "rutil/AbstractFifo.hxx"
#include "resip/stack/CancelableTimerQueue.hxx"

namespace resip
{

/**
   @internal

   Distinct from resip::Fifo; by value and has cancellable timers.
*/
template <class T>
class ValueFifo : public resip::FifoStatsInterface
{
   public:
      typedef typename CancelableTimerQueue<T>::Id TimerId;

      ValueFifo(const Data& name) :
         myFifoSize(0), 
         myTimerSize(0) 
      {
      }

      ~ValueFifo() 
      {
      }

      void add(const T& t)
      {
         resip::Lock lock(myMutex);
         myList.push_back(t);
         myFifoSize++;
         wakeup();
      }
      
      TimerId addDelayMs(const T& t, int offsetInMs)
      {
         resip::Lock lock(myMutex);
         if (offsetInMs < 0)
         {
            offsetInMs = 0;
         }

         bool doWakeup = false;
         if (myTimerQueue.empty() ||
             offsetInMs < myTimerQueue.getTimeout())
         {
            doWakeup = true;
         }

         TimerId id = myTimerQueue.addRelative(t, offsetInMs);
         myTimerSize++;

         //wakeup if new timer is sooner than next timer that would have
         //fired, or no timer set
         if (doWakeup)
         {

            wakeup();
         }
         return id;
      }
      
      bool cancel(TimerId id)
      {
         resip::Lock lock(myMutex);
         if (myTimerQueue.cancel(id))
         {
            myTimerSize--;
            return true;
         }
         return false;
      }
      
      T getNext()
      {
         resip::Lock lock(myMutex);

         while (!messageAvailableNoLock())
         {
            if (myTimerQueue.empty())
            {
               myCondition.wait(&myMutex);
            }
            else
            {
               myCondition.wait(&myMutex, myTimerQueue.getTimeout());
            }            
         }

         while (myTimerQueue.available())
         {
            myList.push_back(myTimerQueue.getNext());
            myFifoSize++;
            myTimerSize--;
         }

         resip_assert (myFifoSize > 0);
         resip_assert (!myList.empty());
         
         T firstMessage = myList.front();
         
         myList.pop_front(); //dcm -- should do this with a guard to avoid extra copy
         myFifoSize--;
         return firstMessage;
      }
      
      void clear()
      {
         resip::Lock lock(myMutex);
         myFifoSize = 0;
         myTimerSize = 0;
         myTimerQueue.clear();
         myList.clear();
      }

      //size includes timer events
      unsigned int size() const
      {
         resip::Lock lock(myMutex);
         return myFifoSize + myTimerSize;
      }

      bool empty() const
      {
         return myFifoSize + myTimerSize == 0;
      }

      bool messageAvailable()
      {
         resip::Lock lock(myMutex);
         return messageAvailableNoLock();
      }

      virtual size_t getCountDepth() const
      {
         return size();
      }

      virtual time_t getTimeDepth() const
      {
         return 0;
      }

      virtual time_t expectedWaitTimeMilliSec() const
      {
         return 0;
      }
      
      virtual time_t averageServiceTimeMicroSec() const
      {
         return 1;
      }

   private:
      bool messageAvailableNoLock()
      {
         return myFifoSize > 0 || myTimerQueue.available(); 
      }
      
      void wakeup()
      {
         myCondition.signal();
      }
      
      std::deque<T> myList;

      CancelableTimerQueue<T> myTimerQueue;

      unsigned long myFifoSize;
      unsigned long myTimerSize;
      
      mutable resip::Mutex  myMutex;
      resip::Condition myCondition;

      // no value semantics
      ValueFifo(const ValueFifo&);
      ValueFifo& operator=(const ValueFifo&);
};
 
}
#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2004 PurpleComm, Inc.  All rights reserved.
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
 */

