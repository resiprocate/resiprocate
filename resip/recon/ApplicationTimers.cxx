#include "ApplicationTimers.hxx"

using namespace recon;

using resip::DialogUsageManager;

ApplicationTimers::ApplicationTimers( resip::SharedPtr<DialogUsageManager> dum )
   : mDum( dum )
{
}


void
ApplicationTimers::invokeOnce( boost::shared_ptr<ApplicationTimer> timerCbk, unsigned int timerId, unsigned int durationMs, unsigned int seqNumber )
{
   if (timerCbk.get() == NULL)
      return;

   TimerCommand tc(timerCbk, timerId, durationMs, seqNumber);
   if(durationMs > 0)
      mDum->getSipStack().postMS(tc, durationMs, mDum.get());
   else
      mDum->post(&tc);
}