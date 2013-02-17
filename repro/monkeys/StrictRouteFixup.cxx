#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/SipMessage.hxx"
#include "repro/monkeys/StrictRouteFixup.hxx"
#include "repro/RequestContext.hxx"
#include "repro/Proxy.hxx"

#include "resip/stack/Helper.hxx"

#include "rutil/Logger.hxx"
#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;


StrictRouteFixup::StrictRouteFixup() :
   Processor("StrictRouteFixup")
{}

StrictRouteFixup::~StrictRouteFixup()
{}

/** @brief This monkey looks to see if the request has
 *        a Route header (the RequestContext has already
 *        done preprocessing, and has removed any topmost
 *        route that was self). If there is, the previous
 *        hop was a strict routing proxy and the candidate
 *        set is exactly the RURI of the received request
 *        (after the above preprocessing).
 */
Processor::processor_action_t
StrictRouteFixup::process(RequestContext& context)
{
   DebugLog(<< "Monkey handling request: " << *this 
            << "; reqcontext = " << context);

   resip::SipMessage& request = context.getOriginalRequest();

   if (request.exists(h_Routes) &&
       !request.header(h_Routes).empty())
   {
      if(!request.header(h_Routes).front().isWellFormed())
      {
         // Garbage topmost Route, reject
         SipMessage resp;
         resip::Helper::makeResponse(resp, request, 400, "Garbage Route Header.");
         context.sendResponse(resp);
         return SkipAllChains;
      }

      // Do any required session accounting
      context.getProxy().doSessionAccounting(request, true /* received */, context);

      //Will cancel any active transactions (ideally there should be none)
      //and terminate any pending transactions.
      context.getResponseContext().cancelAllClientTransactions();
      std::auto_ptr<Target> target(new Target(request.header(h_RequestLine).uri()));
      if(!context.getTopRoute().uri().user().empty())
      {
         resip::Tuple source(Tuple::makeTupleFromBinaryToken(context.getTopRoute().uri().user().base64decode(), Proxy::FlowTokenSalt));
         if(!(source==resip::Tuple()))
         {
            // valid flow token
            target->rec().mReceivedFrom = source;
            target->rec().mUseFlowRouting = true;
         }
      }
      context.getResponseContext().addTarget(target);
      return Processor::SkipThisChain;
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
