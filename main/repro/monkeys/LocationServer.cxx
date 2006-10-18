#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/ExtensionParameter.hxx"
#include "repro/monkeys/LocationServer.hxx"
#include "repro/RequestContext.hxx"
#include "repro/QValueTarget.hxx"
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
	 RegistrationPersistenceManager::ContactRecordList contacts = mStore.getContacts(inputUri);

     mStore.unlockRecord(inputUri);

      std::list<Target*> batch;
     for ( RegistrationPersistenceManager::ContactRecordList::iterator i  = contacts.begin()
             ; i != contacts.end()    ; ++i)
     {
	    RegistrationPersistenceManager::ContactRecord contact = *i;
        if (contact.expires>=time(NULL))
        {
            static ExtensionParameter p_cid("cid");
            contact.uri.param(p_cid)=resip::Data(contact.cid);
           InfoLog (<< *this << " adding target " << contact.uri);
           if(contact.useQ)
           {
               batch.push_back(new QValueTarget(contact.uri,contact.q));
           }
           else
           {
               batch.push_back(new QValueTarget(contact.uri,1.0));   
           }
        }
        else
        {
            // remove expired contact 
            mStore.removeContact(inputUri, contact.uri);
        }
     }

      
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
