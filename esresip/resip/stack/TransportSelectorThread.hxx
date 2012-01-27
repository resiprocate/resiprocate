#ifndef TransportSelectorThread_Include_Guard
#define TransportSelectorThread_Include_Guard

#include "rutil/ThreadIf.hxx"

#include "resip/stack/TransportSelector.hxx"

namespace resip
{
class TransportSelectorThread : public ThreadIf
{
   public:
      explicit TransportSelectorThread(TransportSelector& selector):
         mSelector(selector)
      {}
      virtual ~TransportSelectorThread(){}


/////////////////// Must implement unless abstract ///

      virtual void thread()
      {
         while(!isShutdown())
         {
            FdSet fdset;
            mSelector.buildFdSet(fdset);
            fdset.selectMilliSeconds(mSelector.hasDataToSend() ? 0:25);
            mSelector.process(fdset);
         }
      }

   protected:
      TransportSelector& mSelector;

}; // class TransportSelectorThread

} // namespace resip

#endif // include guard
