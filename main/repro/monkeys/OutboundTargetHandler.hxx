#ifndef OUTBOUND_TARGET_HANDLER
#define OUTBOUND_TARGET_HANDLER 1

#include "repro/Processor.hxx"

namespace repro
{

class OutboundTargetHandler : public Processor
{
   public:
      OutboundTargetHandler();
      virtual ~OutboundTargetHandler();
      
      virtual processor_action_t process(RequestContext &);
      virtual void dump(std::ostream &os) const;

};

}

#endif
