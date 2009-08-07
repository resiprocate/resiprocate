#ifndef __APPLICATIONTIMERS_HXX__
#define __APPlICATIONTIMERS_HXX__

#include <boost/shared_ptr.hpp>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/DumCommand.hxx>
#include <rutil/SharedPtr.hxx>
#include "ApplicationTimer.hxx"

namespace recon
{
   class ApplicationTimers
   {
   public:
      ApplicationTimers( resip::SharedPtr<resip::DialogUsageManager> dum );
      virtual ~ApplicationTimers() {};

      /**
         Used by an application to start a timer. When the timer expires,
         the callback will be called, but only one time.

         @param timerCallback a pointer to the timer callback that should
                be invoked at time t + durationMs. This is a smart pointer
                in order to avoid race conditions between the timer and
                the death of the original object.
         @param timerId Id representing the timers purpose
         @param durationMs the duration of the timer in ms
         @param seqNumber Can be used by the application to differentiate 
                           "active" from "non-active" timers, since timers
                           cannot be stopped
      */
      void invokeOnce(
         boost::shared_ptr<ApplicationTimer> timerCallback,
         unsigned int timerId,
         unsigned int durationMs,
         unsigned int seqNumber);

   private:
      // Handle to the DUM, for invoking timers in the DUM thread.
      resip::SharedPtr<resip::DialogUsageManager> mDum;

      // Private class used for timer
      class TimerCommand : public resip::DumCommand
      {
         public:
            TimerCommand( boost::shared_ptr<ApplicationTimer> timerCbk, unsigned int timerId, unsigned int duration, unsigned int seqNumber) :
               mTimerCbk(timerCbk), mTimerId(timerId), mDuration(duration), mSeqNumber(seqNumber) {}
            TimerCommand(const TimerCommand& rhs) :
               mTimerCbk(rhs.mTimerCbk), mTimerId(rhs.mTimerId), mDuration(rhs.mDuration), mSeqNumber(rhs.mSeqNumber) {}
            ~TimerCommand() {}

            void executeCommand() { mTimerCbk->onApplicationTimer(mTimerId, mDuration, mSeqNumber); }

            resip::Message* clone() const { return new TimerCommand(*this); }
            EncodeStream& encode(EncodeStream& strm) const { strm << "UserAgentTimeout: id=" << mTimerId << ", duration=" << mDuration << ", seq=" << mSeqNumber; return strm; }
            EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }

            unsigned int id() const { return mTimerId; }
            unsigned int seqNumber() const { return mSeqNumber; }
            unsigned int duration() const { return mDuration; }
            
         private:
            boost::shared_ptr<ApplicationTimer> mTimerCbk;
            unsigned int mTimerId;
            unsigned int mDuration;
            unsigned int mSeqNumber;
      };
   };
}

#endif // __APPlICATIONTIMERS_HXX__