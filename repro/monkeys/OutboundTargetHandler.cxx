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

OutboundTargetHandler::OutboundTargetHandler(resip::RegistrationPersistenceManager& store) : 
   Processor("OutboundTargetHandler"),
   mRegStore(store)
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
      resip_assert(target);
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
            (!sip->isFromWire() &&
             (sip->header(resip::h_StatusLine).responseCode()==408 ||         // Locally (stack) generated 408 or 503
              sip->header(resip::h_StatusLine).responseCode()==503)))
         {
            // Flow is dead remove contact from Location Database
            resip::Uri inputUri(ot->getAor());

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

}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 */
