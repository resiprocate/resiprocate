#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <algorithm>

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/ExtensionParameter.hxx"
#include "repro/monkeys/LocationServer.hxx"
#include "repro/RequestContext.hxx"
#include "repro/UserInfoMessage.hxx"
#include "repro/OutboundTarget.hxx"
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

   // UserInfoMessage is used to look for existence of user if we cannot find
   // them in the Regisration Database.  This code handles the asynchronous
   // lookup.
   UserInfoMessage *userInfo = dynamic_cast<UserInfoMessage*>(context.getCurrentEvent());
   if(userInfo && userInfo->getOriginatorAddress() == getAddress())  // Ensure we generated the UserInfo - it could be from the Digest Authenticator
   {
      // If A1 is empty, then user does not exist - return 404
      if(userInfo->A1().empty())
      {
         resip::SipMessage response;
         Helper::makeResponse(response, context.getOriginalRequest(), 404); 
         context.sendResponse(response);
         return Processor::SkipThisChain;
      }
      else
      {
         // User exists, but is just not registered - continue processing
         return Processor::Continue;
      }
   }

   resip::Uri inputUri = context.getOriginalRequest().header(h_RequestLine).uri().getAorAsUri(context.getOriginalRequest().getSource().getType());

   //!RjS! This doesn't look exception safe - need guards
   mStore.lockRecord(inputUri);

   resip::ContactList contacts;
   mStore.getContacts(inputUri,contacts);
   
   if(contacts.size() > 0)
   {
      TargetPtrList batch;
      std::map<resip::Data,resip::ContactList> outboundBatch;
      UInt64 now = Timer::getTimeSecs();
      for(resip::ContactList::iterator i  = contacts.begin(); i != contacts.end(); ++i)
      {
         resip::ContactInstanceRecord contact = *i;
         if (contact.mRegExpires > now)
         {
            InfoLog (<< *this << " adding target " << contact.mContact <<
                  " with tuple " << contact.mReceivedFrom);
            if(contact.mInstance.empty() || contact.mRegId==0)
            {
               QValueTarget* target = new QValueTarget(contact);
               batch.push_back(target);
            }
            else
            {
               outboundBatch[contact.mInstance].push_back(contact);
            }
         }
         else
         {
            // remove expired contact 
            mStore.removeContact(inputUri, contact);
         }
      }

      mStore.unlockRecord(inputUri);

      std::map<resip::Data,resip::ContactList>::iterator o;
      
      for(o=outboundBatch.begin(); o!=outboundBatch.end(); ++o)
      {
         o->second.sort(OutboundTarget::instanceCompare);  // Orders records by lastUpdate time
         OutboundTarget* ot = new OutboundTarget(o->first, o->second);
         batch.push_back(ot);
      }
      
      if(!batch.empty())
      {
         // Note: some elements of list are already in a sorted order (see outbound batch sorting
         // above), however list::sort is stable, so it's safe to sort twice, as relative order 
         // of equal elements is preserved
#ifdef __SUNPRO_CC
         sort(batch.begin(), batch.end(), Target::priorityMetricCompare);
#else
         batch.sort(Target::priorityMetricCompare);
#endif
         context.getResponseContext().addTargetBatch(batch, false /* high priority */);
         //ResponseContext should be consuming the vector
         resip_assert(batch.empty());
      }
   }
   else
   {
      mStore.unlockRecord(inputUri);

      // Note: if we already have targets added from the Route processor we will just skip this
      // as we don't want to return a 404
      if(mUserInfoDispatcher && !context.getResponseContext().hasTargets())
      {
         // User does not have an active registration - check if they even exist or not
         // so we know if should send a 404 vs a 480.
         // Since users are not kept in memory we need to go to the database asynchronously 
         // to look for existence.  We will use the existing mechanism in place for asynchronous 
         // authentication lookups in order to check for existence - we don't need the returned 
         // A1 hash, but the efficiency of this request is more than adequate for this purpose.
         // Currently repro authentication treats authentication realm the same as users aor domain,
         // if this changes in the future we may need to add a different mechanism to check for
         // existence.
         // Note:  repro authentication must be enabled in order for mUserInfoDispatcher to be
         //        defined and for 404 responses to work
         UserInfoMessage* async = new UserInfoMessage(*this, context.getTransactionId(), &(context.getProxy()));
         async->user() = inputUri.user();
         async->realm() = inputUri.host();
         async->domain() = inputUri.host();
         std::auto_ptr<ApplicationMessage> app(async);
         mUserInfoDispatcher->post(app);
         return WaitingForEvent;
      }
   }

   return Processor::Continue;
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
