#include "Time.hxx"
#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <mmsystem.h>
#if (_MSC_VER >= 1400) //no atomic intrinsics on Visual Studio 2003 and below
#include <intrin.h>
#pragma intrinsic(_InterlockedCompareExchange64)
#endif
#endif
#include "rutil/Random.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Lock.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

using namespace resip;

void resip::sleepMs(unsigned int ms)
{
#ifdef WIN32
   Sleep(ms);
#else
   usleep(ms*1000);
#endif
}

void resip::sleepSeconds(unsigned int seconds)
{
#ifdef WIN32
   Sleep(seconds*1000);
#else
   sleep(seconds);
#endif
}

ResipClock::ResipClock(void)
{
}

ResipClock::~ResipClock(void)
{
}

#if defined(WIN32) && defined(_RESIP_MONOTONIC_CLOCK)
unsigned ResipClock::mMaxSystemTimeWaitMs = 60000; //set low initially, may get adjusted by actual timer chosen
#else
unsigned ResipClock::mMaxSystemTimeWaitMs = UINT_MAX;
#endif

#ifdef WIN32
ResipClock::WinMonoClock::PGTC64 ResipClock::WinMonoClock::mGTC64 = &ResipClock::WinMonoClock::GTCLockDuringRange::GTC64;

ResipClock::WinMonoClock::WinMonoClock()
{
   Initialize();
}

void 
ResipClock::WinMonoClock::Initialize(void)
{
   static Mutex mtxStart;
   static volatile bool isInitialized=false;    

   Lock lock(mtxStart);

   if (isInitialized)
   {
      return;
   }
 
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
   ResipClock::mMaxSystemTimeWaitMs  = GTCInterlocked::GetMaxWaitMs();
   mGTC64 = (PGTC64)&GTCInterlocked::GTC64;
   DebugLog(<< "Using GTCInterlocked::GTC64 as the monotonic clock for time functions.");
#else                
   ResipClock::mMaxSystemTimeWaitMs  = GTCLockDuringRange::GetMaxWaitMs();
   mGTC64 = (PGTC64)&GTCLockDuringRange::GTC64;
   DebugLog(<< "Using GTCLockDuringRange::GTC64 as the monotonic clock for time functions.");
#endif 

   unsigned min=0,max=0,actual=0;
   bool isMono = false;

   ResipClock::queryTimerInfo(min,max,actual,isMono);

   InfoLog(<< "Timer resolution: (min/max/actual/isMonotonic) = " << min << '/' << max << '/' << actual <<
      '/' << ((isMono)?("true"):("false")));

   isInitialized = true;
}

#define IS_ALIGNED(_pointer, _alignment) ((((ULONG_PTR) (_pointer)) & ((_alignment) - 1)) == 0)

UInt64 ResipClock::WinMonoClock::GTCInterlocked::GTC64(void)
{
#if defined(_MSC_VER) && (_MSC_VER >= 1400) //no atomic intrinsics on Visual Studio 2003 and below
   ULARGE_INTEGER timeVal;

   resip_assert(IS_ALIGNED(&mBaseTime,8)); //if the implementation ever changes to use 64-bit atomic read/write then 64-bit alignment will be required.

   //InterlockedCompareExchange64 will issue a LOCK CMPXCHG8B to ensure atomic access to mBaseTime
   //Not the most efficient wat to do a 64-bit atomic read (see fild instruction), but no intrinsic for 64-bit atomic read.
   timeVal.QuadPart = _InterlockedCompareExchange64((LONGLONG volatile *)&mBaseTime,0,0);

   DWORD tickNow = ::timeGetTime();

   if (tickNow != timeVal.LowPart)
   {
      //the difference in the low 32-bits and the current 32-bit timeGetTime will be the time difference from
      //the base time till now.  Integer arithmentic will correctly handle cases where tickNow < timeVal.LowPart (rollover).
      //This diff cannot be greater than 0xFFFFFFFF, so this function must be called more frequently than once
      //every 49.7 days.

      DWORD diff = tickNow - timeVal.LowPart;

      //periodically update the basetime, only really necessary at least once every 49.7 days.
      if (diff > mBaseTimeUpdateInterval)
      {
         ULARGE_INTEGER newVal;

         newVal.QuadPart = timeVal.QuadPart + diff; //any 32-bit rollover is now part of the high 32-bits.

         //don't care if this CAS64 actually writes mBaseTime, as long as at least 1 thread updates mBaseTime.
         _InterlockedCompareExchange64((LONGLONG volatile *)&mBaseTime,(LONGLONG)newVal.QuadPart,(LONGLONG)timeVal.QuadPart);
      }

      timeVal.QuadPart += diff; //any 32-bit rollover is now part of the high 32-bits.
   }

   return timeVal.QuadPart;
#else
   resip_assert(0); //this counter only compiles on Visual Studio 2005 +
   return GTCLock::GTC64();
#endif //#if (_MSC_VER >= 1400) //no atomic intrinsics on Visual Studio 2003 and below
}

volatile UInt64 ResipClock::WinMonoClock::GTCInterlocked::mBaseTime = 0;

UInt64
ResipClock::WinMonoClock::GTCLock::GTC64(void)
{
   Lock lock(mMutex);

   DWORD tickNow = ::timeGetTime();

   if (tickNow != mBaseTime.LowPart)
   {
      mBaseTime.QuadPart += (tickNow - mBaseTime.LowPart);
   }

   return mBaseTime.QuadPart;
}

ULARGE_INTEGER ResipClock::WinMonoClock::GTCLock::mBaseTime = {0,0};
Mutex ResipClock::WinMonoClock::GTCLock::mMutex;

UInt64 ResipClock::WinMonoClock::GTCLockDuringRange::GTC64(void)
{
   // we assume that this function is called reasonable often
   // monitor wrap around only in dangerous time range:
   // empiric constants
   const DWORD TIMER_BEGIN_SAFE_RANGE = 0xffff; // about one minute after
   const DWORD TIMER_END_SAFE_RANGE = 0xffff0000; // and before wrap around
  
   DWORD tick = ::timeGetTime();

   if ( ( tick > TIMER_BEGIN_SAFE_RANGE ) && ( tick < TIMER_END_SAFE_RANGE ) )
   {
      LARGE_INTEGER ret;
      ret.HighPart = mWrapCounter;
      ret.LowPart = tick;
      return (UInt64)ret.QuadPart;
   }
   // most application will never be here - only long running servers
   Lock lock(mWrapCounterMutex);
   if (mPrevTick > tick)
   {
      mWrapCounter++;
   }
   mPrevTick = tick;
   LARGE_INTEGER ret;
   ret.HighPart = mWrapCounter;
   ret.LowPart = tick;
   return (UInt64)ret.QuadPart;
}

UInt32   ResipClock::WinMonoClock::GTCLockDuringRange::mWrapCounter = 0;
DWORD    ResipClock::WinMonoClock::GTCLockDuringRange::mPrevTick = 0;
Mutex    ResipClock::WinMonoClock::GTCLockDuringRange::mWrapCounterMutex;
#endif

UInt64
ResipClock::getSystemTime()
{
   resip_assert(sizeof(UInt64) == 64/8);

#if defined(WIN32) || defined(UNDER_CE)
#ifdef _RESIP_MONOTONIC_CLOCK
   static ResipClock::WinMonoClock clockInit;
   return WinMonoClock::GetClock64() * 1000;
#else
   FILETIME ft;

#ifdef UNDER_CE
   SYSTEMTIME st;
   ::GetSystemTime(&st);
   ::SystemTimeToFileTime(&st,&ft);
#else
   ::GetSystemTimeAsFileTime(&ft);
#endif

   ULARGE_INTEGER li;
   li.LowPart = ft.dwLowDateTime;
   li.HighPart = ft.dwHighDateTime;
   return li.QuadPart/10;

#endif //_RESIP_MONOTONIC_CLOCK

#else //#if defined(WIN32) || defined(UNDER_CE)

   UInt64 time=0;
#ifdef _RESIP_MONOTONIC_CLOCK
   struct timespec now_monotonic;
   if (clock_gettime( CLOCK_MONOTONIC, &now_monotonic ) == 0)
//   if ( syscall( __NR_clock_gettime, CLOCK_MONOTONIC, &now_monotonic ) == 0 )
   {
      time = now_monotonic.tv_sec;
      time = time*1000000;
      time += now_monotonic.tv_nsec/1000;
      return time;
   }
#endif
   struct timeval now;
   gettimeofday( &now , NULL );
   //assert( now );
   time = now.tv_sec;
   time = time*1000000;
   time += now.tv_usec;
   return time;
#endif
}

UInt64
ResipClock::getForever()
{
   resip_assert( sizeof(UInt64) == 8 );
#if defined(WIN32) && !defined(__GNUC__)
   return 18446744073709551615ui64;
#else
   return 18446744073709551615ULL;
#endif
}

UInt64
ResipClock::getRandomFutureTimeMs( UInt64 futureMs )
{
   UInt64 now = getTimeMs();

   // make r a random number between 5000 and 9000
   int r = Random::getRandom()%4000;
   r += 5000;

   UInt64 ret = now;
   ret += (futureMs*r)/10000;

   resip_assert( ret >= now );
   resip_assert( ret >= now+(futureMs/2) );
   resip_assert( ret <= now+futureMs );

   return ret;
}

void 
ResipClock::queryTimerInfo(unsigned &min, unsigned &max, unsigned &actual, bool &isMonotonic)
{  
   min = max = actual = 0;
   isMonotonic = false;

#if defined(WIN32) 
#if defined(_RESIP_MONOTONIC_CLOCK)
#if !defined(NTSTATUS)
#define NTSTATUS DWORD
#endif
   typedef NTSTATUS (WINAPI*PNTQTR)(PULONG,PULONG,PULONG);  
	
	HMODULE hm = ::LoadLibrary("ntdll");
	
   if (hm != NULL)
   {
      PNTQTR ntqtr = (PNTQTR)::GetProcAddress(hm,"NtQueryTimerResolution");

      if (ntqtr)
      {
         ntqtr((PULONG)&min,(PULONG)&max,(PULONG)&actual);
         min /= 10;
         max /= 10;
         actual /= 10;
      }
   
      ::FreeLibrary(hm);
      hm = NULL;
   }   
   isMonotonic = true;
#else
   DWORD timeAdjustment=0;
	BOOL timeAdjustmentDisabled=0;
   //on Vista it looks like GetSystemTime has 1ms resolution, but GetSystemTimeAdjustment still returns 15ms
   //so just set the min resolution to whatever is reported, no actual resolution can be consistently found.
   ::GetSystemTimeAdjustment(&timeAdjustment,(PDWORD)&min,&timeAdjustmentDisabled);
   min /= 10;
   isMonotonic = false;   
#endif
#else //WIN32
#ifdef __APPLE__
   //@TODO 
#else
   clockid_t clock = CLOCK_REALTIME; //need to test/verify CLOCK_REALTIME returns the gettimeofday resolution.  
#if defined(_RESIP_MONOTONIC_CLOCK)
   clock = CLOCK_MONOTONIC;
   isMonotonic = true;
#endif
   struct timespec res;
   if (clock_getres(clock,&res) == 0)
   {
      actual = (res.tv_sec * 1000000) + (res.tv_nsec / 1000);
   } 
#endif
#endif
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
 * vi: set shiftwidth=3 expandtab:
 */

