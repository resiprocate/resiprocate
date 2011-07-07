#ifndef OUTBOUND_TARGET_HANDLER
#define OUTBOUND_TARGET_HANDLER 1

#include "repro/Processor.hxx"
#include "resip/dum/RegistrationPersistenceManager.hxx"

namespace repro
{

class OutboundTargetHandler : public Processor
{
   public:
      OutboundTargetHandler(resip::RegistrationPersistenceManager& store);
      virtual ~OutboundTargetHandler();
      
      virtual processor_action_t process(RequestContext &);
      virtual void dump(EncodeStream &os) const;

    private:
      resip::RegistrationPersistenceManager& mRegStore;
};

}

#endif
