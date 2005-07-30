#if !defined(RESIP_DUMCOMMAND_HXX)
#define RESIP_DUMCOMMAND_HXX

#include "resiprocate/Message.hxx"

namespace resip
{

class DumCommand : public Message
{
   public:
      virtual ~DumCommand() {}
      virtual void execute() = 0;
};

}

#endif
