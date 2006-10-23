#include "DtlsTimer.hxx"

using namespace dtls;

DtlsTimer::DtlsTimer(unsigned int seq){
  mValid=true;
};
    
DtlsTimer::~DtlsTimer() {
  ;
}

void
DtlsTimer::fire() {
  if(mValid)
    expired();
}

void
DtlsTimerContext::fire(DtlsTimer *timer){
  timer->fire();

  delete timer;
}
