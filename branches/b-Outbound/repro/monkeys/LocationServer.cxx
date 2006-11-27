#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/ExtensionParameter.hxx"
#include "repro/monkeys/LocationServer.hxx"
#include "repro/RequestContext.hxx"
#include "repro/QValueTarget.hxx"
#include "repro/Proxy.hxx"
#include "resip/stack/SipStack.hxx"

#include "rutil/WinLeakCheck.hxx"


#include "rutil/Logger.hxx"
#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;


Processor::processor_action_t
LocationServer::process(RequestContext& context)
{
   DebugLog(<< "Monkey handling request: " << *this << "; reqcontext = " << context);

   const resip::Uri& inputUri
      = context.getOriginalRequest().header(h_RequestLine).uri();

   //!RjS! This doesn't look exception safe - need guards
   mStore.lockRecord(inputUri);

   if (true) // TODO fix mStore.aorExists(inputUri))
   {  
      resip::ContactList contacts;

      mStore.unlockRecord(inputUri);
      
      std::list<Target*> batch;
      std::map<resip::Data,std::list<Target*> > outboundBatch;
      for ( resip::ContactList::iterator i  = contacts.begin()
               ; i != contacts.end()    ; ++i)
      {
         resip::ContactInstanceRecord contact = *i;
         if (contact.mRegExpires>=(UInt64)time(NULL))
         {
            InfoLog (<< *this << " adding target " << contact.mContact);
            if(contact.mInstance.empty())
            {
               QValueTarget* target = new QValueTarget(contact);
               batch.push_back(target);
            }
            else if(context.getProxy().getStack().isFlowAlive(contact.mReceivedFrom))
            {
               Target* target = new Target(contact);
               target->mPriorityMetric=contact.mLastUpdated;
               outboundBatch[contact.mInstance].push_back(target);
            }
            else
            {
               mStore.removeContact(inputUri,contact);
            }
         }
         else
         {
            // remove expired contact 
            mStore.removeContact(inputUri, contact);
         }
      }

      std::map<resip::Data, std::list<Target*> >::iterator o;
      
      for(o=outboundBatch.begin();o!=outboundBatch.end();++o)
      {
         o->second.sort(Target::targetPtrCompare);
      }
      
      context.getResponseContext().addOutboundBatch(outboundBatch);
      
      if(!batch.empty())
      {
         batch.sort(Target::targetPtrCompare);
         context.getResponseContext().addTargetBatch(batch);
         //ResponseContext should be consuming the vector
         assert(batch.empty());
      }
   }
   else
   {
      mStore.unlockRecord(inputUri);

      // make 404, send, dispose of memory 
      resip::SipMessage response;
      Helper::makeResponse(response, context.getOriginalRequest(), 404); 
      context.sendResponse(response);
      return Processor::SkipThisChain;
   }

   return Processor::Continue;
}

void
LocationServer::dump(std::ostream &os) const
{
   os << "LocationServer monkey" << std::endl;
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
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
