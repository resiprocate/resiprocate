#ifndef TransactionControllerThread_Include_Guard
#define TransactionControllerThread_Include_Guard

#include "rutil/ThreadIf.hxx"

#include "resip/stack/TransactionController.hxx"

namespace resip
{
class TransactionControllerThread : public ThreadIf
{
   public:
      explicit TransactionControllerThread(TransactionController& controller):
         mController(controller)
      {}
      virtual ~TransactionControllerThread(){}


/////////////////// Must implement unless abstract ///

      virtual void thread()
      {
         while(!isShutdown())
         {
            mController.process();
         }
      }

   protected:
      TransactionController& mController;

}; // class TransactionControllerThread

} // namespace resip

#endif // include guard
