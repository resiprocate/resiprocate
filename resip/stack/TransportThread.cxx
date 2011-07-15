#include "resip/stack/TransportThread.hxx"

#include "resip/stack/Transport.hxx"

#include "rutil/FdPoll.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

namespace resip
{
TransportThread::TransportThread(Transport& transport) :
   mTransport(transport),
   mPollGrp(FdPollGrp::create())
{
   // ?bwc? Allow creator to specify the FdPollGrp?
   mTransport.setPollGrp(mPollGrp.get());
}

TransportThread::~TransportThread()
{
   mTransport.setPollGrp(0);
}

void 
TransportThread::thread() 
{
   while(!isShutdown())
   {
      try
      {
         mTransport.process();
         mPollGrp->waitAndProcess(25);
      }
      catch(BaseException& e)
      {
         ErrLog(<< "Unhandled exception: " << e);
      }
   }
   WarningLog(<< "Shutting down transport thread");
}

} // of namespace resip
