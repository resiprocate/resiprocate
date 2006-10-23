#include <iostream>
#include "DtlsTimer.hxx"
#include "TestTimerContext.hxx"
#include "rutil/Timer.hxx"

using namespace std;
using namespace dtls;
using namespace resip;

void
TestTimerContext::addTimer(DtlsTimer *timer, unsigned int lifetime){
  
  mTimer=timer;
  UInt64 timeMs=Timer::getTimeMs();
  mExpiryTime=timeMs+lifetime;

  cerr << "Setting a timer for " << lifetime << " ms from now" << endl;
}

UInt64
TestTimerContext::getRemainingTime(){
  UInt64 timeMs=Timer::getTimeMs();

  if(mTimer){
    if(mExpiryTime<timeMs)
      return(0);

    return(mExpiryTime-timeMs);
  }
  else{
    return Timer::getForever();
  }
}

void
TestTimerContext::updateTimer(){
  UInt64 timeMs=Timer::getTimeMs();

  if(mTimer) {
    if(mExpiryTime<timeMs){
      DtlsTimer *tmpTimer=mTimer;
      mTimer=0;
      
      tmpTimer->expired();
    }
  }
}


    
