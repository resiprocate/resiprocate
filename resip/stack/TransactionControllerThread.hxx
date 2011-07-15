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
      {
         // Unhook the transaction controller from the SipStack's 
         // AsyncProcessHandler, since the stack thread isn't giving cycles to 
         // the TransactionController anymore.
         mController.setInterruptor(0);
      }
      virtual ~TransactionControllerThread(){}


/////////////////// Must implement unless abstract ///

      virtual void thread()
      {
         while(!isShutdown())
         {
            mController.process(25);
         }
      }

   protected:
      TransactionController& mController;

}; // class TransactionControllerThread

} // namespace resip

#endif // include guard
