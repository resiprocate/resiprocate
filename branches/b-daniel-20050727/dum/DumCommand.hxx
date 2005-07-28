#if !defined(RESIP_DUMCOMMAND_HXX)
#define RESIP_DUMCOMMAND_HXX

#include "resiprocate/dum/DumCommand.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"

namespace resip
{

class DumCommand
{
   public:
      virtual ~DumCommand() {}
      virtual void excute() = 0;
};

}

#endif
