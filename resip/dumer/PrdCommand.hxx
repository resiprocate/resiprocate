#if !defined(DumPrdCommand_hxx)
#define DumPrdCommand_hxx

namespace resip
{

class Postable;

class PrdCommand
{
   public:
      virtual void operator()()=0;
      void post();
      
   protected:
      PrdCommand(PrdManager& m);
      PrdManager& mPrdManager;
};

class ManagePrdCommand : public PrdCommand
{
   public:
      ManagePrdCommand(PrdManager& m, SharedPtr<Prd> prd);
      virtual void operator()();

   protected:
      SharedPtr<Prd> mPrd;
};

class UnmanagePrdCommand : public PrdCommand
{
   public:
      UnmanagePrdCommand(PrdManager& m, SharedPtr<Prd> prd);
      virtual void operator()();

   protected:
      SharedPtr<Prd> mPrd;
};

class SendPrdCommand : public PrdCommand
{
   public:
      SendPrdCommand(PrdManager& m, std::auto_ptr<SipMessage> msg);
      virtual void operator()();

   protected:
      std::auto_ptr<SipMessage> mMsg;
};

}


#endif
