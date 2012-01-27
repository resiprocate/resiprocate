#ifndef DummyTU_Include_Guard
#define DummyTU_Include_Guard

#include "resip/stack/TransactionUser.hxx"

#include "resip/stack/SipStack.hxx"
#include "resip/stack/StackThread.hxx"

#include "rutil/Data.hxx"

namespace resip
{

// Convenience class
class DummyTU : public TransactionUser
{
   public:
      DummyTU() :
         mStack(new SipStack),
         mStackThread(new StackThread(*mStack))
      {
         mName="DummyTU";
         mStack->registerTransactionUser(*this);
         mStackThread->run();
      }

      virtual ~DummyTU() 
      {
         mStackThread->shutdown();
         mStackThread->join();
         mStack->shutdown();
         delete mStackThread;
         delete mStack;
      }

      virtual void go() = 0;

      virtual const Data& name() const
      {
         return mName;
      }

      inline SipStack* getStack() const { return mStack;} 

   protected:
      SipStack* mStack;
      StackThread* mStackThread;
      Data mName;

};

} // namespace resip

#endif
