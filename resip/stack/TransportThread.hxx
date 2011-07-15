#ifndef TransportThread_Include_Guard
#define TransportThread_Include_Guard

#include "rutil/ThreadIf.hxx"

namespace resip
{
class FdPollGrp;
class Transport;
class TransportThread : public ThreadIf
{
   public:
      explicit TransportThread(Transport& transport);
      virtual ~TransportThread();

/////////////////// Must implement unless abstract ///

      virtual void thread() ;

   protected:
      Transport& mTransport;
      std::auto_ptr<FdPollGrp> mPollGrp;
}; // class TransportThread

} // namespace resip

#endif // include guard
