#include "rutil/dns/DnsThread.hxx"

#include "rutil/dns/DnsStub.hxx"
#include "rutil/FdPoll.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::DNS

namespace resip
{
DnsThread::DnsThread(DnsStub& dns) :
   mDnsStub(dns)
{
   // ?bwc? Allow creator to specify the FdPollGrp?
   // .bwc. Should use an FdSet based poll group if we're using cares.
   mPollGrp.reset(FdPollGrp::create());
   mDnsStub.setPollGrp(mPollGrp.get());
}

DnsThread::~DnsThread()
{}

void 
DnsThread::thread()
{
   while(!isShutdown())
   {
      try
      {
         mDnsStub.processTimers();
         mPollGrp->waitAndProcess(25);
      }
      catch(BaseException& e)
      {
         ErrLog(<< "Unhandled exception: " << e);
      }
   }
}

} // of namespace resip
