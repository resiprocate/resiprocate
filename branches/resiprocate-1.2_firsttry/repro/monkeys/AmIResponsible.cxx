#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "repro/monkeys/AmIResponsible.hxx"
#include "repro/RequestContext.hxx"
#include "repro/Proxy.hxx"
#include <ostream>

#include "rutil/Logger.hxx"
#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

AmIResponsible::AmIResponsible()
{}

AmIResponsible::~AmIResponsible()
{}

Processor::processor_action_t
AmIResponsible::process(RequestContext& context)
{
   DebugLog(<< "Monkey handling request: " << *this 
            << "; reqcontext = " << context);

   resip::SipMessage& request = context.getOriginalRequest();

   assert (!request.exists(h_Routes) || 
           request.header(h_Routes).empty());
  
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
         // check that we relay from this sender and send to the Request URI
         
         if(!request.header(h_From).isWellFormed())
         {
            InfoLog(<<"Garbage in From header: needed for relay check.");
            resip::SipMessage response;
            Helper::makeResponse(response,context.getOriginalRequest(), 400, "Malformed From: header");
            context.sendResponse(response);
            return Processor::SkipThisChain;            
         }
         
         // !rwm! verify the AuthenticatioInfo object here.
         
         // !rwm! TODO check some kind of relay list here
         // for now, just see if the sender claims to be from one of our domains
         // send a 403 if not on the list      
         // .slg. Allow trusted nodes to relay
         if (!context.fromTrustedNode() && !context.getProxy().isMyUri(request.header(h_From).uri()))
         {
            // make 403, send, dispose of memory
            InfoLog (<< *this << ": will not relay to " << uri << " from " 
                     << request.header(h_From).uri() << ", send 403");
            resip::SipMessage response;
            Helper::makeResponse(response, context.getOriginalRequest(), 403, "Relaying Forbidden"); 
            context.sendResponse(response);
            return Processor::SkipThisChain;
         }

         // only perform relay check for out-of-dialog requests
         // !bwc! Um, then all anyone has to do to get us to be their relay
         // is throw in a spurious to-tag...
         // This smells funny. I am commenting it out.
/*         if (!request.header(h_To).exists(p_tag))
         {         
            // !rwm! verify the AuthenticatioInfo object here.
            
            // !rwm! TODO check some kind of relay list here
            // for now, just see if the sender claims to be from one of our domains
            // send a 403 if not on the list
            if(!request.header(h_From).isWellFormed())
            {
               resip::SipMessage response;
               InfoLog(<<"Garbage in From header: needed for relay check.");
               Helper::makeResponse(response,context.getOriginalRequest(), 400, "Malformed From: header"));
               context.sendResponse(response);
               return Processor::SkipThisChain;
            }

            if (!context.fromTrustedNode() && !context.getProxy().isMyUri(request.header(h_From).uri()))
            {
               // make 403, send, dispose of memory
               resip::SipMessage response;
               InfoLog (<< *this << ": will not relay to " << uri << " from " 
                        << request.header(h_From).uri() << ", send 403");
               Helper::makeResponse(response, context.getOriginalRequest(), 403, "Relaying Forbidden"); 
               context.sendResponse(response);
               return Processor::SkipThisChain;
            }
         }*/
         
         context.getResponseContext().addTarget(NameAddr(request.header(h_RequestLine).uri()));
         InfoLog (<< "Sending to requri: " << request.header(h_RequestLine).uri());
         // skip the rest of the monkeys
         return Processor::SkipThisChain;	
      }
   }
   return Processor::Continue;
}

void
AmIResponsible::dump(std::ostream &os) const
{
  os << "AmIResponsible monkey" << std::endl;
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
