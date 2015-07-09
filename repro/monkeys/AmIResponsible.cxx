#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "repro/monkeys/AmIResponsible.hxx"
#include "repro/monkeys/IsTrustedNode.hxx"
#include "repro/RequestContext.hxx"
#include "repro/Proxy.hxx"
#include <ostream>

#include "rutil/Logger.hxx"
#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

AmIResponsible::AmIResponsible(bool alwaysAllowRelaying) :
   Processor("AmIResponsible"),
   mAlwaysAllowRelaying(alwaysAllowRelaying)
{}

AmIResponsible::~AmIResponsible()
{}

Processor::processor_action_t
AmIResponsible::process(RequestContext& context)
{
   DebugLog(<< "Monkey handling request: " << *this 
            << "; reqcontext = " << context);

   resip::SipMessage& request = context.getOriginalRequest();

   // This call is placed after the DigestAuthenticator, so that we only account for
   // authenticated sessions.
   context.getProxy().doSessionAccounting(request, true /* received */, context);

   // There should be no Routes on the request at this point, if there was a route, then
   // the StrictRouteFixup monkey would have routed to it already
   resip_assert (!request.exists(h_Routes) || 
           request.header(h_Routes).empty());
  
   // Topmost route had a flow-token; this is our problem
   if(!context.getTopRoute().uri().user().empty())
   {
      resip::Tuple dest(Tuple::makeTupleFromBinaryToken(context.getTopRoute().uri().user().base64decode(), Proxy::FlowTokenSalt));
      if(!(dest==resip::Tuple()))
      {
         // .bwc. Valid flow token
         std::auto_ptr<Target> target(new Target(request.header(h_RequestLine).uri()));
         target->rec().mReceivedFrom = dest;
         target->rec().mUseFlowRouting = true;
         context.getResponseContext().addTarget(target);
         return SkipThisChain;
      }
   }

   // this if is just to be safe
   if (!request.exists(h_Routes) || 
       request.header(h_Routes).empty())
   {
      // !RjS! - Jason - check the RURI to see if the domain is
      // something this request is responsible for. If yes, then
      // just return Continue. If no make this call below.
      const Uri& uri = request.header(h_RequestLine).uri();
      if (!context.getProxy().isMyUri(uri))
      {
         // if this is not for a domain for which the proxy is responsible,
         // check that this sender is in our domain and send to the Request URI

         // Ensure To header is well formed
         if(!request.header(h_To).isWellFormed())
         {
            resip::SipMessage response;
            InfoLog(<<"Garbage in To header: needed for relay check.");
            Helper::makeResponse(response,context.getOriginalRequest(), 400, "Malformed To: header");
            context.sendResponse(response);
            return Processor::SkipThisChain;
         }

         // only perform relay check for out-of-dialog requests
         // !bwc! Um, then all anyone has to do to get us to be their relay
         //       is throw in a spurious to-tag...
         //       This smells funny. I am commenting it out.
         // .slg. Putting the old check back in and clarifying the funny smell...
         //       We only want to do this check for out of dialog requests, since 
         //       mid-dialog requests could be 403'd otherwise.  Consider
         //       an INVITE request from a repro domain user to a user in 
         //       another domain.  The resulting ACK/200, BYE or any other
         //       mid-dialog request coming from the remote domain, will contain
         //       the repro users contact address in the RequestUri and a 
         //       foreign domain in the from header.  We want to ensure these
         //       requests are not 403'd.  Byron's comment about an endpoint getting
         //       us to relay by placing a spurious to tag in the request still 
         //       stands. Perhaps we ought to be checking the To header domain in 
         //       this case - however that is also weak, since the To header is not
         //       used in routing and can easily be set to a URI in our domain to
         //       trick repro into forwarding.  Note:  From header domain checking is
         //       stronger than To header domain checking, since if the domain is 
         //       ours, then it must pass Digest Authentication (at least for non 
         //       ACK and BYE requests).
         // .bwc. I think that the only real way to solve the 
         //       I-don't-want-to-be-used-as-a-relay problem is crypto; specifically 
         //       Record-Route with crypto that states "Yeah, I set up this dialog, 
         //       let it through".
         if (!request.header(h_To).exists(p_tag) && !mAlwaysAllowRelaying)
         {
            // Ensure From header is well formed
            if(!request.header(h_From).isWellFormed())
            {
               resip::SipMessage response;
               InfoLog(<<"Garbage in From header: needed for relay check.");
               Helper::makeResponse(response,context.getOriginalRequest(), 400, "Malformed From: header");
               context.sendResponse(response);
               return Processor::SkipThisChain;
            }

            // !rwm! verify the AuthenticatioInfo object here.
            
            // !rwm! TODO check some kind of relay list here
            // for now, just see if the sender claims to be from one of our domains
            // send a 403 if not on the list

            // .slg. Allow trusted nodes to relay
            if (!context.getKeyValueStore().getBoolValue(IsTrustedNode::mFromTrustedNodeKey) && 
                !context.getProxy().isMyUri(request.header(h_From).uri()) &&
                !request.hasForceTarget())
            {
               // make 403, send, dispose of memory
               resip::SipMessage response;
               InfoLog (<< *this << ": will not relay to " << uri << " from " 
                        << request.header(h_From).uri() << ", send 403");
               Helper::makeResponse(response, context.getOriginalRequest(), 403, "Relaying Forbidden"); 
               context.sendResponse(response);
               return Processor::SkipThisChain;
            }
         }
         
         std::auto_ptr<Target> target(new Target(uri));
         context.getResponseContext().addTarget(target);

         InfoLog (<< "Sending to requri: " << uri);
         // skip the rest of the monkeys
         return Processor::SkipThisChain;	
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
