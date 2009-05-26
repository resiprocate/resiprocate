#if !defined(RESIP_TIME_HXX)
#define RESIP_TIME_HXX

#include "rutil/Mutex.hxx"
#include <limits.h>
#include <cassert>

namespace resip
{

/** Monotonically increasing clock.  The time values returned by this class should be considered independent
    of any other clock time, including the system time (ie OS time,epoch, etc).
 */
class ResipClock
{
   public:
      ResipClock(void);
      ~ResipClock(void);

      /** Returns the current clock time in microseconds.
      */
      static UInt64 getTimeMicroSec();
      /** Returns the current clock time in milliseconds.
      */
      static UInt64 getTimeMs();
      /** Returns the current clock time in seconds.
      */
      static UInt64 getTimeSecs();

      /** Returns an absolute time in ms that is between 50% and 90% of
          passed in ms from now.
      */
      static UInt64 getRandomFutureTimeMs( UInt64 futureMs );
      /** Infinit time in future.
      */
      static UInt64 getForever();

      /** Some monotonic clock implementations may internally only return 32-bit values that will wrap.
        * @see Timer::getMaxSystemTimeWaitMs()
      */
      static unsigned getMaxSystemTimeWaitMs(void)
      {
         return mMaxSystemTimeWaitMs;
      }         

   private:
      /** Returns the current clock time in microseconds.  Does not guarantee that this is related to the actual
        * OS system time (eg epoch or other time).
        */
      static UInt64 getSystemTime();

      static unsigned mMaxSystemTimeWaitMs;

#ifdef WIN32
   private:
      /** Responsible for returning a 64-bit monotonic clock value for timing.  Currently implemented using
        * GetTickCount or GetTickCount64 (on Vista & Server 2008). Precision is milliseconds,
        * accuracy is limited to ::GetTickCount/::GetTickCount64 which is ~15-50ms, but can vary
        * with CPU load. GetTickCount values in the ballpark of 100ms have been seen under load.
        * For more information on GetTickCount's monotonic behavior and implementation 
          @see "GetTickCount – Truth and Fiction", https://blogs.msdn.com/sloh/archive/2005/04/05/405724.aspx
        */
      class WinMonoClock
      {
         public:
            WinMonoClock();

            /** Returns a monotonic clock value in milliseconds.  Currently this is the system uptime as reported
              * by GetTickCount/GetTickCount64
              */
            static UInt64 GetClock64(void)
            {
               return mGTC64();
            }

         private:

            /** Definition of a function that has no parameters and returns a 64-bit unsigned integer.
            */
            typedef UInt64 (*PGTC64)(void);

            /** Get Tick Count wrapper for 32-bit version of ::GetTickCount that is nearly lockless and handles 32-bit wraparound.
              * _InterlockedExchange64 is used, which requires the CMPXCHG8B instruction.  This instruction is found
              * on pentium and later intel processors and K5 and later AMD processors.
            */
            class GTCInterlocked
            {
               public:

                  static UInt64 GTC64(void);

                  /** The maximum time that can elapse when using this class as the timer for resip stack processing.
                    @see resip::SipStack::getTimeTillNextProcessMS().
                  */
                  static UInt32 GetMaxWaitMs(void)
                  {
                     //Since the base time isn't updated on every call, need to ensure that it's updated once every 49.7 days.
                     //The base time will lag behind the current tick count, which means the lag time must be used
                     //to determine the max wait.
                     //Also need to add a cushion to this calculaton because ::GetTickCount is not accurate to 1ms.
                     __int64 maxWait = (__int64)UINT_MAX - mBaseTimeUpdateInterval - mBaseTimeCushion;
                     if (maxWait <= 0)
                     {
                        assert(0);
                        const_cast<UInt32 &>(mBaseTimeUpdateInterval) = 60000;
                        const_cast<UInt32 &>(mBaseTimeCushion) = 120000;
                        return UINT_MAX - mBaseTimeUpdateInterval - mBaseTimeCushion;
                     }
                     return static_cast<UInt32>(maxWait);
                  }

               private:
                  /** Last stored time. Using InterlockedExchange (CMPXCHG8B) the alignment is not necessary, but it shouldn't hurt.
                      Align it on a cache line since it is rarely written and read often (to avoid false-sharing).
                  */
                  static _declspec(align(128)) volatile UInt64 mBaseTime;
                  /** Max elapsed time since last GTC64 call, in milliseconds, before writing mBaseTime.
                      Cannot exceed UINT_MAX - mBaseTimeCushion.
                  */
                  static const UInt32 mBaseTimeUpdateInterval = 60000;
                  static const UInt32 mBaseTimeCushion = 120000; //!< large cushion to be cautious
            };

            /** Get Tick Count wrapper for 32-bit version of ::GetTickCount that minimizes locking and handles 32-bit wraparound.
              * Issues a mutex lock only during a 2 minute window around the 49.7 day threshold.  The lock is issued for each call to
              * GTC64 during this window and durinng this window only.
              * Requires SipStack::getTimeTillNextProcessMS() to not return a value greater than 2 minutes.
            */
            class GTCLockDuringRange
            {
               public:

                  static UInt64 GTC64(void);

                  static UInt32 GetMaxWaitMs(void)
                  {
                     return 120000;
                  }

               private:
                  /** GetTickCounter() and timeGetTime() return DWORD - ms since system start
                    Therefore, the time will wrap around to zero if the system is run continuously for 49.7 days
                   if timer is called reasonable often we may manage wrap around by counter below
                   */
                  static UInt32 mWrapCounter;

                  /** Last obtained tick to detect the need to increment mWrapCounter
                  */
                  static DWORD mPrevTick;

                  /** we have to made it thread safe
                  */
                  static Mutex mWrapCounterMutex;

            };

            /** Get Tick Count wrapper for 32-bit version of ::GetTickCount that locks on a mutex on every call to GTC64()
                to safely handle 32-bit wraparound.
            */
            class GTCLock
            {
               public:
                  static UInt64 GTC64(void);

               private:
                  static ULARGE_INTEGER mBaseTime;

                  /** Primary lock that is executed on each call to GTC64().
                  */
                  static Mutex mMutex;
            };

            static PGTC64 mGTC64;
      };
#endif
};


}//namespace resip

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