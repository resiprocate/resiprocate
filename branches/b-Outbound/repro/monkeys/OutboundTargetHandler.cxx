#include "repro/monkeys/OutboundTargetHandler.hxx"

#include "rutil/Logger.hxx"
#include "resip/stack/SipMessage.hxx"
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

   ResponseContext::OutboundMap& map=rsp.mOutboundMap;
   
   // !bwc! Check to see whether we need to move on to another reg-id
   resip::SipMessage* sip = dynamic_cast<resip::SipMessage*>(msg);
   if(sip && sip->isResponse() && sip->header(resip::h_StatusLine).responseCode() > 299)
   {
      const resip::Data& tid=rsp.mCurrentResponseTid;
      DebugLog(<<"Looking for tid " << tid);
      Target* target = rsp.getTarget(tid);
      assert(target);
      if(target)
      {
         resip::Data& instance = target->rec().mInstance;
         ResponseContext::OutboundMap::iterator i=map.find(instance);
         
         if(i!=map.end())
         {
            if(sip->header(resip::h_StatusLine).responseCode()==410 && !i->second.empty())
            {
               if(i->second.front()==tid)
               {
                  i->second.pop_front();
               }
            }
            else
            {
               map.erase(i);
            }
         }
      }
   }
   
   ResponseContext::OutboundMap::iterator i;
   
   for(i=map.begin();i!=map.end();++i)
   {
      bool bail = false;

      while(!bail && !i->second.empty() && 
            rsp.isCandidate(i->second.front()) )
      {
         bail = rsp.beginClientTransaction(i->second.front());
         if(!bail)
         {
            // !bwc! How did this happen?
            assert(0);
            i->second.pop_front();
         }
      }

   }

   // !bwc! cleanup
   for(i=map.begin();i!=map.end();)
   {
      if(i->second.empty())
      {
         ResponseContext::OutboundMap::iterator temp=i;
         ++i;
         map.erase(temp);
      }
      else
      {
         ++i;
      }
   }
   
   if(map.empty())
   {
      return Processor::Continue;
   }
   else
   {
      return Processor::WaitingForEvent;
   }
   
}

void 
OutboundTargetHandler::dump(std::ostream &os) const
{
   os << "OutboundTargetHandler baboon";
}

}
