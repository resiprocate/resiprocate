#ifndef TransportSelectorThread_Include_Guard
#define TransportSelectorThread_Include_Guard

#include "rutil/FdPoll.hxx"
#include "rutil/ThreadIf.hxx"

#include "resip/stack/TransportSelector.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

namespace resip
{
class TransportSelectorThread : public ThreadIf
{
   public:
      explicit TransportSelectorThread(TransportSelector& selector):
         mSelector(selector),
         mPollGrp(FdPollGrp::create())
      {
         mSelector.setPollGrp(mPollGrp.get());
         mSelector.createSelectInterruptor();
      }
      virtual ~TransportSelectorThread()
      {
         mSelector.setPollGrp(0);
      }


/////////////////// Must implement unless abstract ///

      virtual void thread()
      {
         while(!isShutdown())
         {
            try
            {
               mSelector.process();
               mPollGrp->waitAndProcess(25);
            }
            catch(BaseException& e)
            {
               ErrLog(<< "Unhandled exception: " << e);
            }
         }
      }

   protected:
      TransportSelector& mSelector;
      std::auto_ptr<FdPollGrp> mPollGrp;

}; // class TransportSelectorThread

} // namespace resip

#endif // include guard
