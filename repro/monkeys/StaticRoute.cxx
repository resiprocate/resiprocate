#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "repro/Proxy.hxx"
#include "repro/monkeys/CertificateAuthenticator.hxx"
#include "repro/monkeys/StaticRoute.hxx"
#include "repro/monkeys/IsTrustedNode.hxx"
#include "repro/RequestContext.hxx"
#include "repro/QValueTarget.hxx"

#include "rutil/Logger.hxx"
#include "repro/RouteStore.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

StaticRoute::StaticRoute(ProxyConfig& config) :
   Processor("StaticRoute"),
   mRouteStore(config.getDataStore()->mRouteStore),
   mNoChallenge(config.getConfigBool("DisableAuth", false) || !config.getConfigBool("ChallengeThirdPartiesCallingLocalDomains", true)),
   mParallelForkStaticRoutes(config.getConfigBool("ParallelForkStaticRoutes", false)),
   mContinueProcessingAfterRoutesFound(config.getConfigBool("ContinueProcessingAfterRoutesFound", false)),
   mUseAuthInt(!config.getConfigBool("DisableAuthInt", false))
{}

StaticRoute::~StaticRoute()
{}

Processor::processor_action_t
StaticRoute::process(RequestContext& context)
{
   DebugLog(<< "Monkey handling request: " << *this 
            << "; reqcontext = " << context);
   Proxy &proxy = context.getProxy();
   
   SipMessage& msg = context.getOriginalRequest();
   
   Uri ruri(msg.header(h_RequestLine).uri());
   Data method(getMethodName(msg.header(h_RequestLine).method()));
   Data event;
   if ( msg.exists(h_Event) && msg.header(h_Event).isWellFormed())
   {
      event = msg.header(h_Event).value() ;
   }
   
   RouteStore::UriList targets(mRouteStore.process( ruri,
                                                    method,
                                                    event));
   bool requireAuth = false;
   bool externalTarget = false;
   if(!context.getKeyValueStore().getBoolValue(IsTrustedNode::mFromTrustedNodeKey) && 
      msg.method() != ACK &&  // Don't challenge ACK and BYE requests
      msg.method() != BYE)
   {
      requireAuth = !mNoChallenge;
      for ( RouteStore::UriList::const_iterator i = targets.begin();
            i != targets.end(); i++ )
      {
         const Data& targetDomain = i->host(); 
         if(!proxy.isMyDomain(targetDomain))
         {
            DebugLog(<<"target domain " << targetDomain << " is not local");
            externalTarget = true;
            requireAuth |= !mNoChallenge;
         }
      }
   }

   // Allow third-parties verified by their domain certificate, just as long
   // as they don't try to use us as a relay to external targets
   // not in our local domain list
   if(context.getKeyValueStore().getBoolValue(CertificateAuthenticator::mCertificateVerifiedKey)
      && !externalTarget && requireAuth)
   {
      DebugLog(<< "overriding requireAuth because certificate validation succeeded and no targets are external");
      requireAuth = false;
   }

   if (requireAuth && context.getDigestIdentity().empty())
   {
      // !rwm! TODO do we need anything more sophisticated to figure out the realm?
      Data realm = msg.header(h_RequestLine).uri().host();
      
      challengeRequest(context, realm);
      return Processor::SkipAllChains;
   }
   else
   {
      TargetPtrList parallelBatch;
      for (RouteStore::UriList::const_iterator i = targets.begin();
           i != targets.end(); i++ )
      {
         //Targets are only added after authentication
         InfoLog(<< "Adding target " << *i );

         if(mParallelForkStaticRoutes)
         {
            Target* target = new Target(*i);
            parallelBatch.push_back(target);
         }
         else
         {
            // Add Simple Target
            context.getResponseContext().addTarget(NameAddr(*i));
         }
      }
      if(parallelBatch.size() > 0)
      {
         context.getResponseContext().addTargetBatch(parallelBatch, false /* highPriority */);
      }

      if(!targets.empty() && !mContinueProcessingAfterRoutesFound)
      {
         return Processor::SkipThisChain;
      }
   }
   
   return Processor::Continue;
}

void
StaticRoute::challengeRequest(repro::RequestContext &rc, resip::Data &realm)
{
   SipMessage& sipMessage = rc.getOriginalRequest();

   SipMessage *challenge = Helper::makeProxyChallenge(sipMessage, realm, mUseAuthInt /*auth-int*/, false /*stale*/);
   rc.sendResponse(*challenge);

   delete challenge;
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
