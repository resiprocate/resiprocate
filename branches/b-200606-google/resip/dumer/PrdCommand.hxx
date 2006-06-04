#if !defined(DumPrdCommand_hxx)
#define DumPrdCommand_hxx

#include "rutil/SharedPtr.hxx"
#include "rutil/WeakPtr.hxx"

namespace resip
{

class ConnectionTerminated;
class DumTimeout;
class SipMessage;
class TransactionTerminated;
class SipStack;

class PrdCommand
{
   public:
      virtual void operator()()=0;

   protected:
      Prd(SharedPtr<Prd> prd) : mPrd(prd) {;}
      WeakPtr<Prd> mPrd;
};


class ConnectionTerminatedPrdCommand : public PrdCommand
{
   public:
      ConnectionTerminatedPrdCommand(
              SharedPtr<Prd> prd, 
              std::auto_ptr<ConnectionTerminated> msg)
         : mPrd(prd), mConnectionTerminated(msg) { }

      virtual void operator()() 
      { 
         SharedPtr<Ptr> prd(mPrd);
         if (prd.get() != 0)
         {
           prd->onConnectionTerminated(mConnectionTerminated);
         }
      }

   private:
      std::auto_ptr<ConnectionTerminated> mConnectionTerminated;
};


class DumTimeoutPrdCommand : public PrdCommand
{
   public:
      DumTimeoutPrdCommand(
              SharedPtr<Prd> prd, 
              std::auto_ptr<DumTimeout> msg)
         : mPrd(prd), mDumTimeout(msg) { }

      virtual void operator()() 
      { 
         SharedPtr<Ptr> prd(mPrd);
         if (prd.get() != 0)
         {
           prd->onDumTimeout(mDumTimeout);
         }
      }

   private:
      std::auto_ptr<DumTimeout> mDumTimeout;
};


class SipMessagePrdCommand : public PrdCommand
{
   public:
      SipMessagePrdCommand(
              SharedPtr<Prd> prd, 
              std::auto_ptr<SipMessage> msg,
              SipStack &stack)
         : mPrd(prd), mSipMessage(msg), mStack(mStack) { }

      virtual void operator()() 
      { 
         SharedPtr<Ptr> prd(mPrd);
         if (prd.get() != 0)
         {
           prd->onSipMessage(mSipMessage);
         }
      }

   private:
      std::auto_ptr<SipMessage> mSipMessage;
      SipStack &mStack;
};


class TransactionTerminatedPrdCommand : public PrdCommand
{
   public:
      TransactionTerminatedPrdCommand(
              SharedPtr<Prd> prd, 
              std::auto_ptr<TransactionTerminated> msg)
         : mPrd(prd), mTransactionTerminated(msg) { }

      virtual void operator()() 
      { 
         SharedPtr<Ptr> prd(mPrd);
         if (prd.get() != 0)
         {
           prd->onTransactionTerminated(mTransactionTerminated);
         }
      }

   private:
      std::auto_ptr<TransactionTerminated> mTransactionTerminated;
};

   
}
#endif
