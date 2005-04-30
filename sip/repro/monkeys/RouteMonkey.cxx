#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/SipMessage.hxx"
#include "repro/monkeys/RouteMonkey.hxx"
#include "repro/RequestContext.hxx"

#include "resiprocate/os/Logger.hxx"
#include "repro/RouteStore.hxx"


#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO


using namespace resip;
using namespace repro;
using namespace std;


RouteMonkey::RouteMonkey(RouteStore& store) :
   mRouteStore(store)
{}


RouteMonkey::~RouteMonkey()
{}


RequestProcessor::processor_action_t
RouteMonkey::handleRequest(RequestContext& context)
{
   DebugLog(<< "Monkey handling request: " << *this 
            << "; reqcontext = " << context);
   
   SipMessage& msg = context.getOriginalRequest();
   
   Uri ruri(msg.header(h_RequestLine).uri());
   Data method(getMethodName(msg.header(h_RequestLine).method()));
   Data event;
   if ( msg.exists(h_Event) )
   {
      event = msg.header(h_Event).value() ;
   }
   
   RouteStore::UriList targets(mRouteStore.process( ruri,
                                                    method,
                                                    event));
   
   for ( RouteStore::UriList::const_iterator i = targets.begin();
         i != targets.end(); i++ )
   {
      InfoLog(<< "Adding target " << *i );
      context.addTarget(NameAddr(*i));
   }
   
   return RequestProcessor::Continue;
}


void
RouteMonkey::dump(std::ostream &os) const
{
   os << "Route Monkey" << std::endl;
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
