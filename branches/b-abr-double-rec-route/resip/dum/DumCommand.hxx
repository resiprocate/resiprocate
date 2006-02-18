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

}

#endif

