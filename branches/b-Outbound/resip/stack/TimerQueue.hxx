#if !defined(RESIP_TIMERQUEUE_HXX)
#define RESIP_TIMERQUEUE_HXX 

#include <set>
#include <iosfwd>
#include "resip/stack/TransactionMessage.hxx"
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
  * This class takes a fifo as a place to where you can write your stuff.
  * In the main loop when using this you need to call process() on this.
  * During Transaction processing, TimerMessages and SIP messages get generated.
  **/


//!dcm! - refactor, templatize
class BaseTimerQueue
{
   public:
      /// deletes the message associated with the timer as well.
      virtual ~BaseTimerQueue()=0;
	  
      /// gets the set of timers that have fired and inserts TimerMsg into the state
      /// machine fifo and application messages into the TU fifo
      virtual void process()=0;

      int size() const;
      bool empty() const;
      
	  /** Give me the time before the next timer will fire in milliseconds
        *  @retval milliseconds (time until the next timer will fire)
	    *  @retval 0 (implies that timers occur in the past)
	    * @retval INT_MAX (implies that there are no timers)
	   **/
      unsigned int msTillNextTimer();
      
   protected:
      friend std::ostream& operator<<(std::ostream&, const BaseTimerQueue&);
      std::multiset<Timer> mTimers;
};

class BaseTimeLimitTimerQueue : public BaseTimerQueue
{
   public:
      void add(const Timer& timer);
      virtual void process();
   protected:
      virtual void addToFifo(Message*, TimeLimitFifo<Message>::DepthUsage)=0;      
};


class TimeLimitTimerQueue : public BaseTimeLimitTimerQueue
{
   public:
      TimeLimitTimerQueue(TimeLimitFifo<Message>& fifo);
   protected:
      virtual void addToFifo(Message*, TimeLimitFifo<Message>::DepthUsage);      
   private:
      TimeLimitFifo<Message>& mFifo;
};


class TuSelectorTimerQueue : public BaseTimeLimitTimerQueue
{
   public:
      TuSelectorTimerQueue(TuSelector& sel);
   protected:
      virtual void addToFifo(Message*, TimeLimitFifo<Message>::DepthUsage);      
   private:
      TuSelector& mFifoSelector;
};


class TimerQueue : public BaseTimerQueue
{
   public:
      TimerQueue(Fifo<TransactionMessage>& fifo);
      Timer::Id add(Timer::Type type, const Data& transactionId, unsigned long msOffset);
      virtual void process();
   private:
      Fifo<TransactionMessage>& mFifo;
};

#ifdef USE_DTLS

#include <openssl/ssl.h>

class DtlsTimerQueue : public BaseTimerQueue
{
   public:
      DtlsTimerQueue( Fifo<DtlsMessage>& fifo ) ;
      void add( SSL *, unsigned long msOffset ) ;
      virtual void process() ;
      
   private:
      Fifo<DtlsMessage>& mFifo ;
};

#endif

std::ostream& operator<<(std::ostream&, const BaseTimerQueue&);

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
