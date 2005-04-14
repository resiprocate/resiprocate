#include "resiprocate/SipStack.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/DumProcessHandler.hxx"
#include "resiprocate/os/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

namespace resip {

DumProcessHandler::DumProcessHandler(ExternalTimer* et) :
   mHaveActiveTimer(false),
   mDum(0),
   mExternalTimer(et),
   mStopped(false),
   mCurrentlyProcessing(false)
{
}

void 
DumProcessHandler::start(DialogUsageManager* dum)
{
   mDum = dum;
   mExternalTimer->setHandler(this);
   mTimerID = mExternalTimer->generateAsyncID();
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
      
      int timeTillProcess = 0;      
      do
      {
         mDum->process(fds);
         timeTillProcess = mDum->getTimeTillNextProcessMS();
      }
      while (timeTillProcess == 0);
      if (timeTillProcess != INT_MAX)
      {
         if (mHaveActiveTimer)
         {
            mExternalTimer->deleteTimer(mTimerID);
         }         
         assert(timeTillProcess < 60*4*60*1000); //4hr sanity check
         mTimerID = mExternalTimer->generateAsyncID();
         DebugLog ( << "Setting dum process timer: " << timeTillProcess);
         mExternalTimer->createTimer(mTimerID, timeTillProcess);
         mHaveActiveTimer = true;         
      }
      mCurrentlyProcessing = false;
   }   
}

void 
DumProcessHandler::handleTimeout(AsyncID timerID)
{
   assert(timerID == mTimerID);   
   mHaveActiveTimer = false;   
   handleProcessNotification();
}

void 
DumProcessHandler::stop()
{
   mStopped = true;
   if (mHaveActiveTimer)
   {
      mExternalTimer->deleteTimer(mTimerID);
   }         
}

} // namespace resip
