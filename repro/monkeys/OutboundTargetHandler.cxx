#include "repro/monkeys/OutboundTargetHandler.hxx"

#include "rutil/Logger.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "repro/OutboundTarget.hxx"
#include "repro/ResponseContext.hxx"
#include "repro/RequestContext.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

namespace repro
{
OutboundTargetHandler::OutboundTargetHandler()
{

}

OutboundTargetHandler::~OutboundTargetHandler()
{

}

Processor::processor_action_t 
OutboundTargetHandler::process(RequestContext & rc)
{
   resip::Message* msg = rc.getCurrentEvent();
   ResponseContext& rsp = rc.getResponseContext();
   if(!msg)
   {
      return Processor::Continue;
   }

   // !bwc! Check to see whether we need to move on to another reg-id
   resip::SipMessage* sip = dynamic_cast<resip::SipMessage*>(msg);
   if(sip && sip->isResponse() && sip->header(resip::h_StatusLine).responseCode() > 299)
   {
      const resip::Data& tid=sip->getTransactionId();
      DebugLog(<<"Looking for tid " << tid);
      Target* target = rsp.getTarget(tid);
      assert(target);
      OutboundTarget* ot = dynamic_cast<OutboundTarget*>(target);
      if(ot)
      {
         std::auto_ptr<Target> newTarget(ot->nextInstance());
         if(newTarget.get())
         {
            int flowDeadCode;
            if(resip::InteropHelper::getOutboundVersion() >= 5)
            {
               flowDeadCode=430;
            }
            else
            {
               flowDeadCode=410;
            }

            if(sip->header(resip::h_StatusLine).responseCode()==flowDeadCode)
            {
               // Try next reg-id
               rsp.addTarget(newTarget);
               return Processor::SkipAllChains;
            }
         }
      }
   }

   return Processor::Continue;
}

void 
OutboundTargetHandler::dump(EncodeStream &os) const
{
   os << "OutboundTargetHandler baboon";
}

}
