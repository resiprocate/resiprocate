#if !defined(RESIP_TIMERQUEUE_HXX)
#define RESIP_TIMERQUEUE_HXX 

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include <functional>
#include <queue>
#include <set>
#include <iosfwd>
#include "resip/stack/TimerMessage.hxx"
#include "resip/stack/DtlsMessage.hxx"
#include "rutil/Fifo.hxx"
#include "rutil/TimeLimitFifo.hxx"
#include "rutil/Timer.hxx"

// .dlb. 
// to do: timer wheel for transaction-bound timers and a heap for
// everything longer.

namespace resip
{

class Message;
class TransactionMessage;
class TuSelector;

/**
  * @internal
  * @brief This class takes a fifo as a place to where you can write your stuff.
  * When using this in the main loop, call process() on this.
  * During Transaction processing, TimerMessages and SIP messages are generated.
  * 
    @todo .dlb. timer wheel for transaction-bound timers and a heap for 
      everything longer.
  */
template <class T>
class TimerQueue
{
   public:
      // This is the logic that runs when a timer goes off. This is the only
      // thing subclasses must implement.
      virtual void processTimer(const T& timer)=0;

      /// @brief deletes the message associated with the timer as well.
      virtual ~TimerQueue()
      {
         //xkd-2004-11-4
         // delete the message associated with the timer
         while (!mTimers.empty())
         {
            mTimers.pop();
         }
      }

      /// @brief provides the time in milliseconds before the next timer will fire
      ///  @retval milliseconds time until the next timer will fire
      ///  @retval 0 implies that timers occur in the past
      /// @retval INT_MAX implies that there are no timers
      ///
      unsigned int msTillNextTimer()
      {
         if (!mTimers.empty())
         {
            UInt64 next = mTimers.top().getWhen();
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

      /// @brief gets the set of timers that have fired, and inserts TimerMsg into the state
      /// machine fifo and application messages into the TU fifo
      virtual UInt64 process()
      {
         if (!mTimers.empty())
         {
            UInt64 now=Timer::getTimeMs();
            while (!mTimers.empty() && !(mTimers.top().getWhen() > now))
            {
               processTimer(mTimers.top());
               mTimers.pop();
            }

            if(!mTimers.empty())
            {
               return mTimers.top().getWhen();
            }
         }
         return 0;
      }

      int size() const
      {
         return (int)mTimers.size();
      }

      bool empty() const
      {
         return mTimers.empty();
      }

      std::ostream& encode(std::ostream& str) const
      {
         if(mTimers.size() > 0)
         {
            return str << "TimerQueue[ size =" << mTimers.size() 
                       << " top=" << mTimers.top() << "]" ;
         }
         else
         {
            return str << "TimerQueue[ size = 0 ]";
         }
      }

#ifndef RESIP_USE_STL_STREAMS
      EncodeStream& encode(EncodeStream& str) const
      {
         if(mTimers.size() > 0)
         {
            return str << "TimerQueue[ size =" << mTimers.size() 
                       << " top=" << mTimers.top() << "]" ;
         }
         else
         {
            return str << "TimerQueue[ size = 0 ]";
         }
      }
#endif

   protected:
      typedef std::vector<T, std::allocator<T> > TimerVector;
      std::priority_queue<T, TimerVector, std::greater<T> > mTimers;
};

/**
   @internal
*/
class BaseTimeLimitTimerQueue : public TimerQueue<TimerWithPayload>
{
   public:
      ~BaseTimeLimitTimerQueue();
      UInt64 add(unsigned int timeMs,Message* payload);
      virtual void processTimer(const TimerWithPayload& timer);
   protected:
      virtual void addToFifo(Message*, TimeLimitFifo<Message>::DepthUsage)=0;      
};


/**
   @internal
*/
class TimeLimitTimerQueue : public BaseTimeLimitTimerQueue
{
   public:
      TimeLimitTimerQueue(TimeLimitFifo<Message>& fifo);
   protected:
      virtual void addToFifo(Message*, TimeLimitFifo<Message>::DepthUsage);
   private:
      TimeLimitFifo<Message>& mFifo;
};


/**
   @internal
*/
class TuSelectorTimerQueue : public TimerQueue<TimerWithPayload>
{
   public:
      TuSelectorTimerQueue(TuSelector& sel);
      ~TuSelectorTimerQueue();
      UInt64 add(unsigned int timeMs,Message* payload);
      virtual void processTimer(const TimerWithPayload& timer);
   private:
      TuSelector& mFifoSelector;
};


/**
   @internal
*/
class TransactionTimerQueue : public TimerQueue<TransactionTimer>
{
   public:
      TransactionTimerQueue(Fifo<TimerMessage>& fifo);
      UInt64 add(Timer::Type type, const Data& transactionId, unsigned long msOffset);
      virtual void processTimer(const TransactionTimer& timer);
   private:
      Fifo<TimerMessage>& mFifo;
};

#ifdef USE_DTLS

#include <openssl/ssl.h>

/**
   @internal
*/
class DtlsTimerQueue : public TimerQueue<TimerWithPayload>
{
   public:
      DtlsTimerQueue(Fifo<DtlsMessage>& fifo);
      ~DtlsTimerQueue();
      UInt64 add(SSL *, unsigned long msOffset);
      virtual void processTimer(const TimerWithPayload& timer) ;
      
   private:
      Fifo<DtlsMessage>& mFifo ;
};

#endif

template <class T>
std::ostream& operator<<(std::ostream& str, const TimerQueue<T>& tq)
{
   return tq.encode(str);
}

#ifndef RESIP_USE_STL_STREAMS
template <class T>
EncodeStream& operator<<(EncodeStream& str, const TimerQueue<T>& tq)
{
   return tq.encode(str);
}
#endif


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
