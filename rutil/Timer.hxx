#if !defined(RESIP_TIMER_HXX)
#define RESIP_TIMER_HXX 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
     
#include "rutil/Data.hxx"
#include "rutil/HeapInstanceCounter.hxx"
#include <iostream>
#include "rutil/Time.hxx"

/// !slg!  TODO - a large portion of the code in this file belongs in the resip/stack as
/// opposed to rutil.  Candidates for move are:
/// 1.  All SIP Specific timer logic in Timer class
/// 2.  Transaction Timer
/// 3.  TimerWithPayload

namespace resip
{

class Message;

/**
   @brief This class is used to get the current system time.

   @note Should we refactor this? It seems like the SIP-timer stuff should live
      in resip/stack somewhere.
*/
class Timer
{
   public:
      RESIP_HeapCount(Timer);
      typedef enum 
      {
         TimerA, // doubling
         TimerB,
         TimerC,
         TimerD,
         TimerE1,// doubling
         TimerE2,// doubling
         TimerF,
         TimerG, // doubling
         TimerH,
         TimerI,
         TimerJ,
         TimerK,
         TimerTrying,
         TimerStaleClient,
         TimerStaleServer,
         TimerStateless,
         TimerCleanUp,
         ApplicationTimer, // .dlb. Fifo, so no thread issues
         TcpConnectTimer
      } Type;
      
      static Data toData(Type timer);

      /** @deprecated.  Does not do anything at the moment.
      */
      static void setupTimeOffsets()
      {}

      /** Returns the current clock time in microseconds.
      */
      static UInt64 getTimeMicroSec()
      {
         return ResipClock::getTimeMicroSec();
      }
      /** Returns the current clock time in milliseconds.
      */
      static UInt64 getTimeMs()
      {
         return ResipClock::getTimeMs();
      }
      /** Returns the current clock time in seconds.
      */
      static UInt64 getTimeSecs()
      {
         return ResipClock::getTimeSecs();
      }
      /** Returns an absolute time in ms that is between 50% and 90% of
          passed in ms from now.
      */
      static UInt64 getRandomFutureTimeMs(UInt64 futureMs)
      {
         return ResipClock::getRandomFutureTimeMs(futureMs);
      }
      /** Infinit time in future.
      */
      static UInt64 getForever()
      {
         return ResipClock::getForever();
      }

      /** On windows, change getSystemTime() to not use the actual clock time
          returned by ::GetSystemTime().  The replacement time function on some versions of windows (< Vista,
          GetTickCount()) returns a 32-bit value, so it will rollover at some point.
          Any code that waits on timers should not wait longer than this value.

         @see resiprocate.org devlist discussion "Timers: why system time?";
         @see resip::SipStack::getTimeTillNextProcessMS().
      */
      static unsigned getMaxSystemTimeWaitMs(void)
      {
         return ResipClock::getMaxSystemTimeWaitMs();
      }

      static void resetT1(unsigned long t1);

      // These values can be changed but it is not recommended to do so after a
      // stack is up and running since they are not mutexed
      // These times are all in ms 
      static unsigned long T1;
      static unsigned long T2;
      static unsigned long T4;
      static unsigned long T100;
      
      static unsigned long TB; // default 64*T1
      static unsigned long TC; 
      static unsigned long TF; // default 64*T1
      static unsigned long TH; // default 64*T1
      
      static unsigned long TD;
      static unsigned long TS;

      static unsigned long TcpConnectTimeout;
};

// !bwc! There is some duplicated code between TransactionTimer and 
// TimerWithPayload. Should eventually create a single template class like the 
// following, and use it with Payload=Message* and Payload=TransactionTimer
// template <typename Payload>
// class Timestamped
// {
//    // blah blah blah
//    private:
//       UInt64 mTimestamp;
//       Payload mPayload;
// };
class TransactionTimer
{
   public:
      RESIP_HeapCount(TransactionTimer);

      TransactionTimer(unsigned long ms, 
                        Timer::Type type, 
                        const Data& transactionId);

      ~TransactionTimer(){}

      const Data& getTransactionId() const {return mTransactionId;}
      Timer::Type getType() const { return mType; }
      unsigned long getDuration() const { return mDuration;} 

      UInt64 getWhen() const {return mWhen;}
#ifndef RESIP_USE_STL_STREAMS
      EncodeStream& encode(EncodeStream& str) const;
#endif
      std::ostream& encode(std::ostream& str) const;

      inline bool operator<(const TransactionTimer& rhs) const
      {
         return mWhen < rhs.mWhen;
      }

      inline bool operator>(const TransactionTimer& rhs) const
      {
         return mWhen > rhs.mWhen;
      }

   protected:
      UInt64 mWhen;
      Timer::Type mType;
      Data mTransactionId;
      unsigned long mDuration; // duration of time in ms 

   private:
      // disabled
      TransactionTimer();
};

class TimerWithPayload
{
   public:
      RESIP_HeapCount(TimerWithPayload);

      TimerWithPayload(unsigned long ms, Message* message);
      ~TimerWithPayload() {}

      // return the message to queue, possibly null
      Message* getMessage() const { return mMessage; }

      UInt64 getWhen() const {return mWhen;}
#ifndef RESIP_USE_STL_STREAMS
      EncodeStream& encode(EncodeStream& str) const;
#endif
      std::ostream& encode(std::ostream& str) const;

      inline bool operator<(const TimerWithPayload& rhs) const
      {
         return mWhen < rhs.mWhen;
      }

      inline bool operator>(const TimerWithPayload& rhs) const
      {
         return mWhen > rhs.mWhen;
      }

   protected:
      UInt64 mWhen;
      Message* mMessage; // message to queue on timeout

   private:
      // disabled
      TimerWithPayload();
};


inline std::ostream& operator<<(std::ostream& str, const TransactionTimer& t)
{
   return t.encode(str);
}

inline std::ostream& operator<<(std::ostream& str, const TimerWithPayload& t)
{
   return t.encode(str);
}

#ifndef RESIP_USE_STL_STREAMS
inline EncodeStream& operator<<(EncodeStream& str, const TransactionTimer& t)
{
   return t.encode(str);
}

inline EncodeStream& operator<<(EncodeStream& str, const TimerWithPayload& t)
{
   return t.encode(str);
}
#endif

}

#if 0
Timer    Value            Section               Meaning
----------------------------------------------------------------------
T1       500ms default    Section 17.1.1.1     RTT Estimate
T2       4s               Section 17.1.2.2     The maximum retransmit
                                               interval for non-INVITE
                                               requests and INVITE
                                               responses
T4       5s               Section 17.1.2.2     Maximum duration a
                                               message will
                                               remain in the network
Timer A  initially T1     Section 17.1.1.2     INVITE request retransmit
                                               interval, for UDP only
Timer B  64*T1            Section 17.1.1.2     INVITE transaction
                                               timeout timer
Timer C  > 3min           Section 16.6         proxy INVITE transaction
                           bullet 11            timeout
Timer D  > 32s for UDP    Section 17.1.1.2     Wait time for response
         0s for TCP/SCTP                       retransmits
Timer E  initially T1     Section 17.1.2.2     non-INVITE request
                                               retransmit interval,
                                               UDP only
Timer F  64*T1            Section 17.1.2.2     non-INVITE transaction
                                               timeout timer
Timer G  initially T1     Section 17.2.1       INVITE response
                                               retransmit interval
Timer H  64*T1            Section 17.2.1       Wait time for
                                               ACK receipt
Timer I  T4 for UDP       Section 17.2.1       Wait time for
         0s for TCP/SCTP                       ACK retransmits
Timer J  64*T1 for UDP    Section 17.2.2       Wait time for
         0s for TCP/SCTP                       non-INVITE request
                                               retransmits
Timer K  T4 for UDP       Section 17.1.2.2     Wait time for
         0s for TCP/SCTP                       response retransmits

                   Table 4: Summary of timers
#endif

#endif

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
