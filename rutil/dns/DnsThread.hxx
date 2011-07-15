#ifndef DnsThread_Include_Guard
#define DnsThread_Include_Guard

#include "rutil/ThreadIf.hxx"

namespace resip
{
class DnsStub;
class FdPollGrp;

class DnsThread : public ThreadIf
{
   public:
      DnsThread(DnsStub& dns);
      virtual ~DnsThread();


/////////////////// Must implement unless abstract ///

      virtual void thread();

   protected:
      DnsStub& mDnsStub;
      std::auto_ptr<FdPollGrp> mPollGrp;
}; // class DnsThread

} // namespace resip

#endif // include guard
