#if !defined(RESIP_EXTERNAL_TIMER_HXX)
#define RESIP_EXTERNAL_TIMER_HXX

#include "resiprocate/external/AsyncID.hxx"

namespace resip
{
   //used by the asynchronous executive
class ExternalTimer
{
   public:
      virtual AsyncID generateAsyncID()=0;
      virtual void setHandler(ExternalTimerHandler* handler)=0;
      
      virtual void createTimer(AsyncID timerID);
      virtual void deleteTimer(AsyncID timerID);
};

class ExternalTimerHandler
{
   public:
      virtual void handleTimeout(AsyncID timerID);
};
}

#endif
