#if !defined(RESIP_EXTERNAL_TIMER_HXX)
#define RESIP_EXTERNAL_TIMER_HXX

#include "resiprocate/external/AsyncID.hxx"

namespace resip
{

class ExternalTimerHandler
{
   public:
      virtual void handleTimeout(AsyncID timerID)=0;
};

//used by the asynchronous executive
class ExternalTimer
{
   public:
      virtual AsyncID generateAsyncID()=0;
      virtual void setHandler(ExternalTimerHandler* handler)=0;
      
      virtual void createTimer(AsyncID timerID, unsigned long ms)=0;
      virtual void createRecurringTimer(AsyncID timerID, unsigned long every_ms)=0;
      virtual void deleteTimer(AsyncID timerID)=0;
};

}

#endif
