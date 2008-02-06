#include "DtlsTimer.hxx"

using namespace dtls;

DtlsTimer::DtlsTimer(unsigned int seq)
{
  mValid=true;
};
    
DtlsTimer::~DtlsTimer() {}

void
DtlsTimer::fire() {
   if(mValid)
   {
      expired();
   }
   else
   {
      //memory mangement is overly tricky and possibly wrong...deleted by target
      //if valid is the contract. weak pointers would help.
      delete this;
   }
}

void
DtlsTimerContext::fire(DtlsTimer *timer){
  timer->fire();
}
