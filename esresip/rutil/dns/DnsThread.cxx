#include "rutil/dns/DnsThread.hxx"

#include "rutil/dns/DnsStub.hxx"

namespace resip
{
DnsThread::DnsThread(DnsStub& dns) :
   mDnsStub(dns)
{}

DnsThread::~DnsThread()
{}

void 
DnsThread::thread()
{
   while(!isShutdown())
   {
      FdSet fdset;
      mDnsStub.buildFdSet(fdset);
      // ?bwc? Should we set up an interrupt when something does into the 
      // command fifo?
      fdset.selectMilliSeconds(25);
      mDnsStub.process(fdset);
   }
}

} // of namespace resip
