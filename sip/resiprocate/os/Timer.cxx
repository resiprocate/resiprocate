
#include "resiprocate/util/Socket.hxx"

#if defined( WIN32 )
#  include <windows.h>
#  include <winbase.h>
#else
#  include <sys/time.h>
#  include <unistd.h>
#endif

#include <cassert>
#include <iostream>
#include "resiprocate/util/Timer.hxx"
#include "resiprocate/util/Logger.hxx"
#include "resiprocate/util/Random.hxx"

using namespace Vocal2;

unsigned long 
Vocal2::Timer::mCpuSpeedMHz = 0L;

UInt64 
Vocal2::Timer::mBootTime=0L;

unsigned long
Vocal2::Timer::mTimerCount = 0L;

unsigned long
Vocal2::Timer::T1 = 500;

unsigned long
Vocal2::Timer::T2 = 8 * T1;

unsigned long
Vocal2::Timer::T4 = 10 * T1;

unsigned long
Vocal2::Timer::T100 = 80;

unsigned long
Vocal2::Timer::TD = 32000;

unsigned long
Vocal2::Timer::TC = 3*60*1000;

unsigned long
Vocal2::Timer::TS = 32000;


#define VOCAL_SUBSYSTEM Subsystem::SIP

Data
Timer::toData(Type timer)
{
   switch (timer)
   {
      case TimerA: // doubling
         return "Timer A";
      case TimerB:
         return "Timer B";
      case TimerC:
         return "Timer C";
      case TimerD:
         return "Timer D";
      case TimerE1:
         return "Timer E1";
      case TimerE2:
         return "Timer E2";
      case TimerF:
         return "Timer F";
      case TimerG: 
         return "Timer G";
      case TimerH:
         return "Timer H";
      case TimerI:
         return "Timer I";
      case TimerJ:
         return "Timer J";
      case TimerK:
         return "Timer K";
      case TimerTrying:
         return "Timer Trying";
      case TimerStaleClient:
         return "Timer StaleClient";
      case TimerStaleServer:
         return "Timer StaleServer";
      default:
         assert(0);
   }
   return "Bad Bad Bad in timer";
}


Timer::Timer(unsigned long tms, Timer::Type type, const Data& transactionId) :
   mWhen(tms + getTimeMs()),
   mId(++mTimerCount),
   mType(type),
   mTransactionId(transactionId),
   mDuration(tms)
{
}


Timer::Timer(unsigned long tms) :
   mWhen(tms + getTimeMs()),
   mId(0),
   mDuration(tms)
{
}


Timer::Timer(const Timer& other) : 
   mWhen(other.mWhen),
   mId(other.mTimerCount),
   mType(other.mType),
   mTransactionId(other.mTransactionId),
   mDuration(other.mDuration)
{
}


Timer&
Timer::operator=(const Timer& other)
{
   if (this != &other)
   {
      mWhen = other.mWhen;
      mId = other.mTimerCount;
      mType = other.mType;
      mTransactionId = other.mTransactionId;
      mDuration = other.mDuration;
   }
   return *this;
}


UInt64
Timer::getSystemTime()
{
   UInt64 time=0;
#if defined(WIN32)  
   SYSTEMTIME t;
   GetSystemTime( &t );
   time = (t.wHour*60+t.wMinute)*60+t.wSecond; 
   time = time*1000000;
   time += t.wMilliseconds*1000;
#else
   struct timeval now;
   gettimeofday( &now , NULL );
   //assert( now );
   time = now.tv_sec;
   time = time*1000000;
   time += now.tv_usec;
#endif
   return time;
}


UInt64
Timer::getSystemTicks()
{
   UInt64 tick;
#if defined(WIN32) 
   volatile unsigned int lowtick=0,hightick=0;
   __asm
      {
         rdtsc 
            mov lowtick, eax
            mov hightick, edx
            }
   tick = hightick;
   tick <<= 32;
   tick |= lowtick;
#else  
#  if defined(__GNUC__) && ( defined(__i686__) || defined(__i386__) )
   asm("rdtsc" : "=A" (tick)); // this should actually work anywhere GNUC does
#  else
#    if defined (__SUNPRO_CC)	
   tick = gethrtime();//This is Not expensive Under solaris 8 & above but systemcall in solaris7
#    else
#      if defined (__MACH__) || defined (__PPC__)
   struct timeval now;
   gettimeofday( &now , NULL );
   //assert( now );
   tick = now.tv_sec;
   tick *= 1000000;
   tick += now.tv_usec;
#      else
   struct timeval now;
   gettimeofday( &now , NULL );
   //assert( now );
   tick = now.tv_sec;
   tick *= 1000000;
   tick += now.tv_usec;
   //tick = cjGetSystemTimeOfDay();
#      endif
#    endif
#  endif
#endif
   return tick;
}


void 
Timer::setupTimeOffsets()
{
   unsigned long cpuSpeed = 1;
    
   /* timing loop to calculate - cpu speed */ 
   UInt64 start;
   UInt64 now;
    
   UInt64 startTick=0;
   UInt64 nowTick=0;
    
   UInt64 uSec; 
   UInt64 count;
    
   // wait for a time tick
   now = getSystemTime();
   start = now;
   while ( (start == now) )
   {
      start = getSystemTime(); 
      startTick = getSystemTicks();
   }

   // wait for next time tick
   do     
   {
      now = getSystemTime(); 
      nowTick = getSystemTicks();
   }
   while ( (now-start) < 200*1000 );
    
   uSec = now - start;
   count = nowTick - startTick;
    
   assert( uSec >= 100*1000 );
   assert( uSec < 500*1000 );
   assert( count > 100 );
    
   //cerr << "diff in uSec is " << uSec << endl;
   //cerr << "diff in sec is " << sec << endl;
   //cerr << "diff in count is " << count << endl;
    
   UInt64 cpuSpeed64 = count / uSec;
   cpuSpeed = (unsigned long)cpuSpeed64;
    
   static int speeds[] = { 0,1,16,25,33,60,90,100,133,150,166,200,
                           266,300,400,450,
                           500,533,550,600,633,650,667,
                           700,733,750,800,850,866,900,933,950,1000,
                           1100,1200,1266,1400,1500,1600,1700,1800,1900,
                           2000,2100,2200,2266,2400,2500,2533,2600,2666,2800,
                           3000,3200,3600,3733,4000,4266,-1 };
   int diff=cpuSpeed;
   int index = 0;
   int i=1;
   while (speeds[i]!=-1)
   {
      int d =  speeds[i] - cpuSpeed ;
      d = (d>=0) ? d : -d ;
      if ( d<diff )
      {
         diff = d;
         index = i;
      }
      i++;
   }
#if defined(__MACH__) || defined(__PPC__)
   cpuSpeed = 1;
#else
   assert( index != 0 );
   cpuSpeed = 0;

   // if it is faster than know processors ....
   if ( (index == i-1) && (diff>50) )
   {
      // just keep the estimated speed
   }
   else
   {
      cpuSpeed = speeds[index];
   }
#endif
    
   now = getSystemTime();
   nowTick = getSystemTicks();
    
   mBootTime = now - nowTick/cpuSpeed;
   mCpuSpeedMHz = cpuSpeed;
}


UInt64 
Timer::getTimeMicroSec()
{
   assert( sizeof(UInt64) == 64/8 );
    
   UInt64 time=0; /* 64 bit */ 
    
   if ( mCpuSpeedMHz == 0 ) 
   {
      setupTimeOffsets();   
   }
   assert( mCpuSpeedMHz != 0 );

   time = getSystemTicks()/mCpuSpeedMHz + mBootTime;
    
   assert( time != 0 );
   return time;
}

UInt64 
Timer::getTimeMs()
{
   return getTimeMicroSec() / 1000;
}


UInt64 
Timer::getRandomFutureTimeMs( UInt64 futureMs )
{
   UInt64 now = getTimeMs();
   
   // make r a random number between 5000 and 9000 
   int r = Random::getRandom()%4000;
   r += 5000;
   
   UInt64 ret = now;
   ret += (futureMs*r)/10000;

   assert( ret >= now );
   assert( ret >= now+(futureMs/2) );
   assert( ret <= now+futureMs );

   return ret;
}


bool 
Vocal2::operator<(const Timer& t1, const Timer& t2)
{
   //std::cerr << "operator(<" << t1.mWhen << ", " << t2.mWhen << ") = " << (t1.mWhen < t2.mWhen) << std::endl;
   return t1.mWhen < t2.mWhen;
}


std::ostream& 
Vocal2::operator<<(std::ostream& str, const Timer& t)
{
   str << "Timer[id=" << t.mId << " when=" << t.mWhen << "]";
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
