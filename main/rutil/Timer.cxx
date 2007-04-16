#include "rutil/Socket.hxx"

#if defined( WIN32 )
#  include <windows.h>
#  include <winbase.h>
#else
#  include <sys/time.h>
#  include <unistd.h>
#endif

#include <cassert>

#include "rutil/Timer.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

using namespace resip;
 
unsigned long
resip::Timer::mTimerCount = 1L;


unsigned long
resip::Timer::T1 = 500;

unsigned long
resip::Timer::T2 = 8 * T1;

unsigned long
resip::Timer::T4 = 10 * T1;

unsigned long
resip::Timer::T100 = 80;

unsigned long
resip::Timer::TB = 64*T1;

unsigned long
resip::Timer::TD = 32000;

unsigned long
resip::Timer::TC = 3*60*1000;

unsigned long
resip::Timer::TF = 64*T1;

unsigned long
resip::Timer::TH = 64*T1;

unsigned long
resip::Timer::TS = 32000;


#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


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
      case TimerStateless:
         return "Timer Stateless";
      case TimerCleanUp:
         return "Timer Cleanup";
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

Timer::Timer(unsigned long tms, Message* message) 
   : mWhen(tms + getTimeMs()),
     mId(++mTimerCount),
     mType(Timer::ApplicationTimer),
     mTransactionId(),
     mDuration(tms),
     mMessage(message)
{
   assert(mMessage);
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
    mDuration(other.mDuration),
    mMessage(other.mMessage)
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
        mMessage = other.mMessage;
    }
    return *this;
}

Timer::~Timer() 
{}

UInt64
Timer::getSystemTime()
{
    assert( sizeof(UInt64) == 64/8 );
    UInt64 time=0;
#if defined(WIN32)  
    FILETIME ft;
    GetSystemTimeAsFileTime( &ft);
    ULARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    time = li.QuadPart/10;
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


void 
Timer::setupTimeOffsets()
{
}


UInt64 
Timer::getTimeMicroSec()
{
    return getSystemTime();
}


UInt64 
Timer::getTimeMs()
{
    return getSystemTime()/1000LL;
}

UInt64 
Timer::getTimeSecs()
{
    return getSystemTime()/1000000LL;
}


UInt64 
Timer::getForever()
{
    assert( sizeof(UInt64) == 8 );
#if defined(WIN32) && !defined(__GNUC__)
    return 18446744073709551615ui64;
#else
    return 18446744073709551615ULL;
#endif
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
resip::operator<(const Timer& t1, const Timer& t2)
{
    //std::cerr << "operator(<" << t1.mWhen << ", " << t2.mWhen << ") = " << (t1.mWhen < t2.mWhen) << std::endl;
    return t1.mWhen < t2.mWhen;
}

bool 
resip::operator>(const Timer& t1, const Timer& t2)
{
    return t1.mWhen > t2.mWhen;
}

std::ostream& 
resip::operator<<(std::ostream& str, const Timer& t)
{
   UInt64 now = Timer::getTimeMs();

   str << "Timer[id=" << t.mId << " when=" << t.mWhen << " rel=";
   if (t.mWhen < now)
   {
      str << "past";
   }
   else
   {
      str << (t.mWhen - now);
   }
   str << "]";
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
