#include <iostream>
#include "DtlsTimer.hxx"
#include "TestTimerContext.hxx"
#include "rutil/Timer.hxx"

using namespace std;
using namespace dtls;
using namespace resip;

TestTimerContext::TestTimerContext()
{
   mTimer=0;
}

void
TestTimerContext::addTimer(DtlsTimer *timer, unsigned int lifetime)
{
   if(mTimer)
      delete mTimer;

   mTimer=timer;
   UInt64 timeMs=Timer::getTimeMs();
   mExpiryTime=timeMs+lifetime;

   cerr << "Setting a timer for " << lifetime << " ms from now" << endl;
}

UInt64
TestTimerContext::getRemainingTime()
{
   UInt64 timeMs=Timer::getTimeMs();

   if(mTimer)
   {
      if(mExpiryTime<timeMs)
         return(0);

      return(mExpiryTime-timeMs);
   }
   else
   {
      return Timer::getForever();
   }
}

void
TestTimerContext::updateTimer()
{
   UInt64 timeMs=Timer::getTimeMs();

   if(mTimer) 
   {
      if(mExpiryTime<timeMs)
      {
         DtlsTimer *tmpTimer=mTimer;
         mTimer=0;

         fire(tmpTimer);
      }
   }
}


/* ====================================================================

 Copyright (c) 2007-2008, Eric Rescorla and Derek MacDonald 
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:
 
 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 
 
 3. None of the contributors names may be used to endorse or promote 
    products derived from this software without specific prior written 
    permission. 
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
