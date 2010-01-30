#ifndef __APPLICATION_TIMER_HXX__
#define __APPLICATION_TIMER_HXX__

namespace recon
{
   /**
    * Pure virtual class describing the timer interface. Use this class
    * in conjunction with the "ApplicationTimers" class.
    */
   class ApplicationTimer
   {
   public:

      /**
         Callback used when an application timer expires.

         @param timerId Id representing the timers purpose
         @param durationMs the duration of the timer in ms
         @param seqNumber Can be used by the application to differentiate 
                           "active" from "non-active" timers, since timers
                           cannot be stopped
      */
      virtual void onApplicationTimer(unsigned int timerId, unsigned int durationMs, unsigned int seqNumber) = 0;
   };
}
#endif // __APPLICATION_TIMER_HXX__