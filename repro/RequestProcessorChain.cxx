#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include <iostream>

#include "resiprocate/SipMessage.hxx"
#include "repro/RequestProcessorChain.hxx"
#include "repro/RequestContext.hxx"

#include "rutil/Logger.hxx"
#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

repro::RequestProcessorChain::RequestProcessorChain()
{
   DebugLog(<< "Instantiating new monkey chain");
}

repro::RequestProcessorChain::~RequestProcessorChain()
{
   Chain::iterator i;
   for (i = chain.begin(); i != chain.end(); i++)
   {
      delete *i;
   }
}

void
repro::RequestProcessorChain::addProcessor(auto_ptr<RequestProcessor> rp)
{
   DebugLog(<< "Adding new monkey to chain: " << *(rp.get()));
   chain.push_back(rp.release());
}

repro::RequestProcessor::processor_action_t
repro::RequestProcessorChain::handleRequest(RequestContext &rc)
{
   //DebugLog(<< "Monkey handling request: " << *this << "; reqcontext = " << rc);

   Chain::iterator i;
   processor_action_t action;

   if (rc.chainIteratorStackIsEmpty())
   {
      i = chain.begin();
   }
   else
   {
      i = rc.popChainIterator();
   }

   for (; i != chain.end(); i++)
   {
      DebugLog(<< "Chain invoking monkey: " << **i);

      action = (**i).handleRequest(rc);

      if (action == SkipAllChains)
      {
         DebugLog(<< "Monkey aborted all chains: " << **i);
         return SkipAllChains;
      }

      if (action == WaitingForEvent)
      {
         DebugLog(<< "Monkey waiting for async response: " << **i);
         rc.pushChainIterator(i);
         return WaitingForEvent;
      }

      if (action == SkipThisChain)
      {
         DebugLog(<< "Monkey skipping current chain: " << **i);
         return Continue;
      }

   }
   //DebugLog(<< "Monkey done processing: " << **i);
   return Continue;
}

void
RequestProcessorChain::dump(std::ostream &os) const
{
   os << "Monkey Chain!" << std::endl;
   // !abr! Dump monkey chain here
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
