n#if !defined(DumPrdManagerCommand_hxx)
#define DumPrdManagerCommand_hxx

#include "rutil/SharedPtr.hxx"

namespace resip
{

class Postable;
class PrdManager;
class SipMessage;

class PrdManagerCommand
{
   public:
      virtual void operator()()=0;
      void post();
      
   protected:
      PrdManagerCommand(PrdManager& m);
      PrdManager& mPrdManager;
};

class ManagePrdManagerCommand : public PrdManagerCommand
{
   public:
      ManagePrdManagerCommand(PrdManager& m, SharedPtr<Prd> prd);
      virtual void operator()();

   protected:
      SharedPtr<Prd> mPrd;
};

class UnmanagePrdManagerCommand : public PrdManagerCommand
{
   public:
      UnmanagePrdManagerCommand(PrdManager& m, SharedPtr<Prd> prd);
      virtual void operator()();

   protected:
      SharedPtr<Prd> mPrd;
};

class SendPrdManagerCommand : public PrdManagerCommand
{
   public:
      SendPrdManagerCommand(PrdManager& m, std::auto_ptr<SipMessage> msg);
      virtual void operator()();

   protected:
      std::auto_ptr<SipMessage> mMsg;
};

   
}


#endif
