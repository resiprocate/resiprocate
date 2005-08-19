#if !defined(RESIP_DUMCOMMAND_HXX)
#define RESIP_DUMCOMMAND_HXX

#include "resip/sip/ApplicationMessage.hxx"

namespace resip
{

class DumCommand : public ApplicationMessage
{
   public:
      virtual ~DumCommand() {}
      virtual void execute() = 0;
};

}

#endif

