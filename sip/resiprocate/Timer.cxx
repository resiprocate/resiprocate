#if defined( WIN32 )
#  include <windows.h>
#  include <winbase.h>
#else
#  include <sys/time.h>
#  include <unistd.h>
#endif

#include <cassert>
#include <iostream>
#include <sipstack/Timer.hxx>
#include <sipstack/Logger.hxx>

static const char version[] = "$Id: Timer.cxx,v 1.5 2002/09/22 22:32:10 dabryan Exp $";

using namespace Vocal2;

unsigned long 
Vocal2::Timer::mCpuSpeedMHz = 0L;

UInt64 
Vocal2::Timer::mBootTime=0L;

unsigned long
Vocal2::Timer::mTimerCount = 0L;

const unsigned long
Vocal2::Timer::T1 = 500;

const unsigned long
Vocal2::Timer::T2 = 8 * T1;

const unsigned long
Vocal2::Timer::T4 = 10 * T1;

const unsigned long
Vocal2::Timer::T100 = 80;

const unsigned long
Vocal2::Timer::TD = 32000;

const unsigned long
Vocal2::Timer::TC = 3*60*1000;

const unsigned long
Vocal2::Timer::TS = 32000;


#define VOCAL_SUBSYSTEM Subsystem::SIP

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
   asm("rdtsc" : "=A" (tick));
#  else
   tick = cjGetSystemTimeOfDay();
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
    
   cpuSpeed = count / uSec;
    
   static int speeds[] = { 0,1,25,33,60,90,100,133,150,166,200,
                           266,300,400,450,
                           500,533,550,600,633,650,667,
                           700,733,750,800,850,866,900,933,950,1000,
                           1100,1200,1400,1500,1600,1700,1800,1900,
                           2000,2200,-1 };
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
    
   DebugLog (<< "Computed cpu speed is " << cpuSpeed << " MHz ");

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


bool Vocal2::operator<(const Timer& t1, const Timer& t2)
{
   return t1.mWhen <  t2.mWhen;
}

