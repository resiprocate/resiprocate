#if !defined(DumPrdCommand_hxx)
#define DumPrdCommand_hxx

#include "rutil/SharedPtr.hxx"

namespace resip
{

class Postable;
class PrdManager;
class SipMessage;

class PrdCommand
{
   public:
      virtual void operator()()=0;
      void post();
      
   protected:
      PrdCommand(PrdManager& m);
      PrdManager& mPrdManager;
};


class DumTimeoutPrdCommand : public PrdCommand
{
   public:
      DumTimeoutPrdCommand(Prd& m, std_auto_ptr<DumTimeout> t);
      virtual void operator()();

   protected:
};
   
}


#endif
