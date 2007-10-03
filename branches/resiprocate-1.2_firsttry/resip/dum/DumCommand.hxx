#if !defined(RESIP_DUMCOMMAND_HXX)
#define RESIP_DUMCOMMAND_HXX

#include "resip/stack/ApplicationMessage.hxx"

namespace resip
{

class DumCommand : public ApplicationMessage
{
   public:
      virtual ~DumCommand() {}
      virtual void executeCommand() = 0;
};

class DumCommandAdapter : public DumCommand
{
public:
   virtual ~DumCommandAdapter() {}

   virtual Message* clone() const
   {
      assert(false);
      return NULL;
   }

   virtual std::ostream& encode(std::ostream& strm) const
   {
      return encodeBrief(strm);
   }
};
}

#endif

