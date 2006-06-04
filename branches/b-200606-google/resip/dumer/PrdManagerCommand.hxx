n#if !defined(DumPrdManagerCommand_hxx)
#define DumPrdManagerCommand_hxx

#include "rutil/SharedPtr.hxx"

namespace resip
{

class Postable;
class PrdManager;
class SipMessage;

class PrdManagerCommandFunctor
{
   public:
      virtual void operator()()=0;
};

class PrdManagerCommand : public PrdManagerCommandFunctor
{
   protected:
      PrdManagerCommand(PrdManagerCore& m) : mPrdManagerCore(m){}
      PrdManagerCore& mPrdManagerCore;
};

class ManagePrdManagerCommand : public PrdManagerCommand
{
   public:
      ManagePrdManagerCommand(PrdManager& m, SharedPtr<Prd> prd)
        : PrdManagerCommand(m), mPrd(prd) : {}

      virtual void operator()()
      {
         mPrdManagerCore->internalManage(mPrd);
      }

   protected:
      SharedPtr<Prd> mPrd;
};

class UnmanagePrdManagerCommand : public PrdManagerCommand
{
   public:
      UnmanagePrdManagerCommand(PrdManager& m, PrdId prd)
        : PrdManagerCommand(m), mPrd(prdId) : {}

      virtual void operator()()
      {
         mPrdManagerCore->internalUnmanage(mPrdId);
      }

   protected:
      PrdId mPrdId;
};

class SendPrdManagerCommand : public PrdManagerCommand
{
   public:
      SendPrdManagerCommand(PrdManager& m, std::auto_ptr<SipMessage> msg)
        : PrdManagerCommand(m), mSipMessage(msg) : {}

      virtual void operator()()
      {
         mPrdManagerCore->internalSend(mSipMessage);
      }

   protected:
      std::auto_ptr<SipMessage> mSipMessage;
};

class RussianDollPrdManagerCommand : public PrdManagerCommandFunctor
{
   public:
      RussianDollPrdManagerCommand(std::auto_ptr<PrdCommand> cmd,
                                   Postable &postable)
        :  mPrdCommand(msg), mPostable(postable) {}

      virtual void operator()()
      {
         mPostable->post(mPrdCommand);
      }

   protected:
      std::auto_ptr<PrdCommand> mPrdCommand;
      Postable mPostable;
};

   
} 
#endif
