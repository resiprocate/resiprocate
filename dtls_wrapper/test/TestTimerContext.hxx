// World's lamest timer implementation
#ifndef TestTimerContext_hxx
#define TestTimerContext_hxx

#include "rutil/Timer.hxx"

namespace dtls {
     
class TestTimerContext: public DtlsTimerContext{
  public:
     void addTimer(DtlsTimer *timer, unsigned int seq);
     UInt64 getRemainingTime();
     
     DtlsTimer *mTimer;
     UInt64 mExpiryTime;
};

}
#endif
