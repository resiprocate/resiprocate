#include "resiprocate/SipStack.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/DumProcessHandler.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

namespace resip {

DumProcessHandler::DumProcessHandler(ExternalTimer* et) :
   mDum(0),
   mExternalTimer(et),
   mCurrentlyProcessing(false),
   mStopped(false)
{
}

void 
DumProcessHandler::start(DialogUsageManager* dum)
{
   mDum = dum;
   //!dcm! -- temporary

   mExternalTimer->setHandler(this);
   mTimerID = mExternalTimer->generateAsyncID();
   mExternalTimer->createRecurringTimer(mTimerID, 30);   
}

void
DumProcessHandler::handleProcessNotification()
{
   //only works when there is exactly one thread causing notifications; could be
   //made thread safecancelled
   if (!mCurrentlyProcessing && !mStopped)
   {
      mCurrentlyProcessing = true;
      //very temporary
      //FD_ISSET     ??
      FdSet fds;
      mDum->buildFdSet(fds);
      if (fds.size > 0)
      {         
         fds.selectMilliSeconds((long)0);
      }
      mDum->process(fds);      
      mCurrentlyProcessing = false;
   }   
}

void 
DumProcessHandler::handleTimeout(AsyncID /*timerID*/)
{
   handleProcessNotification();   
}

void 
DumProcessHandler::stop()
{
   mStopped = true;
   mExternalTimer->deleteTimer(mTimerID);   
}

} // namespace resip
