#include "DtlsTimer.hxx"
#include "TestTimerContext.hxx"
#include "rutil/Timer.hxx"

using namespace dtls;
using namespace resip;

void
TestTimerContext::addTimer(DtlsTimer *timer, unsigned int lifetime){
  
  mTimer=timer;
  UInt64 timeMs=Timer::getTimeMs();
  mExpiryTime=timeMs+lifetime;
}

UInt64
TestTimerContext::getRemainingTime(){
  UInt64 timeMs=Timer::getTimeMs();

  if(mExpiryTime<timeMs)
    return(0);

  return(mExpiryTime<timeMs);
}




    
