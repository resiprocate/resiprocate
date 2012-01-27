#ifndef DnsThread_Include_Guard
#define DnsThread_Include_Guard

#include "rutil/ThreadIf.hxx"

namespace resip
{
class DnsStub;
class DnsThread : public ThreadIf
{
   public:
      DnsThread(DnsStub& dns);
      virtual ~DnsThread();


/////////////////// Must implement unless abstract ///

      virtual void thread();

   protected:
      DnsStub& mDnsStub;
}; // class DnsThread

} // namespace resip

#endif // include guard
