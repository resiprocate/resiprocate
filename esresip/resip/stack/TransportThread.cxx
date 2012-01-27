#include "resip/stack/TransportThread.hxx"

#include "resip/stack/Transport.hxx"

#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

namespace resip
{
TransportThread::TransportThread(Transport& transport) :
   mTransport(transport)
{}

TransportThread::~TransportThread()
{}

void 
TransportThread::thread() 
{
   while(!isShutdown())
   {
      try
      {
         resip::FdSet fdset;
         mTransport.buildFdSet(fdset);
         int ret = fdset.selectMilliSeconds(25);
         if(ret >= 0)
         {
            mTransport.process(fdset);
         }
      }
      catch(BaseException& e)
      {
         ErrLog(<< "Unhandled exception: " << e);
      }
   }
   WarningLog(<< "Shutting down transport thread");
}

} // of namespace resip
