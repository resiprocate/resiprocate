#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "repro/Proxy.hxx"
#include "repro/ProxyConfig.hxx"
#include "repro/RequestContext.hxx"
#include "repro/monkeys/SimpleStaticRoute.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/NameAddr.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;


SimpleStaticRoute::SimpleStaticRoute(ProxyConfig& config) :
   Processor("SimpleStaticRoute")
{
   std::vector<Data> routeSet;
   config.getConfigValue("Routes", routeSet);

   resip::NameAddrs routes;
   for (std::vector<Data>::iterator i=routeSet.begin(); 
        i != routeSet.end(); ++i)
   {
      try
      {
         mRouteSet.push_back(NameAddr(*i));
      }
      catch(BaseException& ex)
      {
         WarningLog(<< "SimpleStaticRoute: Skipping invalid route (" << *i << "): " << ex);
      }
   }
}

SimpleStaticRoute::~SimpleStaticRoute()
{}


Processor::processor_action_t
SimpleStaticRoute::process(RequestContext& context)
{
   DebugLog(<< "Monkey handling request: " << *this 
            << "; reqcontext = " << context);
   
   resip::SipMessage& request = context.getOriginalRequest();
   const Uri& uri = request.header(h_RequestLine).uri();
   if (context.getProxy().isMyUri(uri))
   {
      const resip::NameAddrs& current = request.header(h_Routes); 
      resip::NameAddrs replace = mRouteSet;
      for (resip::NameAddrs::const_iterator i=current.begin(); i != current.end(); ++i)
      {
         replace.push_back(*i);
      }
      request.header(h_Routes) = replace;
      context.getResponseContext().addTarget(NameAddr(uri));
      
      InfoLog (<< "New route set is " << Inserter(request.header(h_Routes)));
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
 */
