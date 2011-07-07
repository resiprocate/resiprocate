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
   OutboundTargetHandler::OutboundTargetHandler(resip::RegistrationPersistenceManager& store) : mRegStore(store)
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
         int flowDeadCode;
         if(resip::InteropHelper::getOutboundVersion() >= 5)
         {
            flowDeadCode=430;
         }
         else
         {
            flowDeadCode=410;
         }
         if(sip->header(resip::h_StatusLine).responseCode()==flowDeadCode ||  // Remote or locally(stack) generate 430
            (sip->getReceivedTransport() == 0 &&
             (sip->header(resip::h_StatusLine).responseCode()==408 ||         // Locally (stack) generated 408 or 503
              sip->header(resip::h_StatusLine).responseCode()==503)))
         {
            // Flow is dead remove contact from Location Database
            resip::Uri inputUri = rc.getOriginalRequest().header(resip::h_RequestLine).uri().getAorAsUri(rc.getOriginalRequest().getSource().getType());

            //!RjS! This doesn't look exception safe - need guards
            mRegStore.lockRecord(inputUri);
            mRegStore.removeContact(inputUri,ot->rec());
            mRegStore.unlockRecord(inputUri);

            std::auto_ptr<Target> newTarget(ot->nextInstance());
            if(newTarget.get())
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
