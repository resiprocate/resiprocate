#if !defined(RESIP_ASYNC_PROCESS_NOTIFIER_DNS_HXX)
#define RESIP_ASYNC_PROCESS_NOTIFIER_DNS_HXX

#include "resiprocate/ProcessNotifier.hxx"
#include "resiprocate/external/ExternalTimer.hxx"


namespace resip
{

class DialogUsageManager;


class DumProcessHandler : public ProcessNotifier::Handler, public ExternalTimerHandler
{
   public:
      DumProcessHandler(ExternalTimer*);      
      virtual void handleProcessNotification(); 
      virtual void handleTimeout(AsyncID timerID);
      
      //would put in constructor, but things are circular
      void start(DialogUsageManager*);
      void stop();//!dcm! -- temporary      
   private:
      AsyncID mTimerID;      
      DialogUsageManager* mDum;      
      ExternalTimer* mExternalTimer;      
      bool mStopped;      
};

#endif
