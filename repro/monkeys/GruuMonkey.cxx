#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Helper.hxx"
#include "repro/monkeys/GruuMonkey.hxx"
#include "repro/RequestContext.hxx"
#include "repro/Proxy.hxx"
#include "dum/RegistrationPersistence
#include <ostream>

#include "rutil/Logger.hxx"
#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

GruuMonkey::GruuMonkey()
{}

GruuMonkey::~GruuMonkey()
{}

RequestProcessor::processor_action_t
GruuMonkey::handleRequest(RequestContext& context)
{
   DebugLog(<< "Monkey handling request: " << *this 
            << "; reqcontext = " << context);

   resip::SipMessage& request = context.getOriginalRequest();

   // This monkey looks for a Request URIs such as:
   //    sip:alice;uuid:urn:00000-000-8....@example.com;user=gruu
   //
   // Looking for a Request URI which contains ";user=gruu"
   
   Uri reqUri = request.header(h_RequestLine).uri();
   
   if (reqUri.exists(p_user) && (reqUri.param(p_user) == "gruu"))
   {
      int pos = reqUri.user().find(";");
      if (!pos)
      {
         // can't recognize GRUU, doesn't have a ";" in the userpart
         // send 404 Not Found
         resip::SipMessage response;
         InfoLog (<< *this << ": no AOR matching this GRUU found.  Gruu" << uri << ", sending 404");
         Helper::makeResponse(response, request, 404); 
         context.sendResponse(response);
         return RequestProcessor::SkipThisChain;                     
      }
      else
      {
         aor.user() = reqUri.user().substr(0,pos);
         instance   = reqUri.user().substr(pos+1);
         aor.host() = reqUri.host();

         // check if AOR is valid in this domain
         // !rwm! TODO - write this aorExists function
         if (mStore.mUserStore.aorExists(aor))
         {
            // send 404 Not Found
            resip::SipMessage response;
            InfoLog (<< *this << ": no AOR matching this GRUU found.  Gruu" << uri << ", sending 404");
            Helper::makeResponse(response, request, 404); 
            context.sendResponse(response);
            return RequestProcessor::SkipThisChain;            
         }
         
         if (mStore.isContactRegistered(aor.getAor(), instance))
         {
            // grab the grid from the Request URI
            Data grid;
            if (reqUri.exists(p_grid))
            {
               grid = reqUri.params(p_grid);
            }
         
            // walk through the entries that match the instance and aor
            RegistrationPersistenceManager::ContactList records = mStore.getRegistrationBinding(aor)->mContacts;
            RegistrationPersistenceManager::ContactList::const_iterator i;
            
            for (i = records.begin(); p != records.end(); ++i)
            {
               if (i->mInstance == instance)
               {
                  NameAddr newTarget = i->mContact.uri();
                  // strip caller prefs
                  // strip instance and flowId
                  // preserve q-values
                  if (i->mContact.exists(p_q))
                  {
                     newTarget.param(p_q) = i->mContact.param(p_q);
                  }
                  
                  // copy the grid over from the request URI
                  if (!grid.empty())
                  {
                     newTarget.uri().param(p_grid) = grid;
                  }
                  
                  // populate the targetSet with appropriate tuple (or sessionId) and URI combo
                  // !rwm! TODO add a version of addTarget that take the server session id or the Path
                  // and only uses sendOverExistingConnection 
                  if (i->mSipPath.empty())
                  {
                     assert(i->mServerSessionId)  // make sure there is either a Path or a sessionId
                     context.addTarget(newTarget, i->mServerSessionId);
                  }
                  else
                  {
                     context.addTarget(newTarget, i->mSipPath);
                  }
               }

            }
            InfoLog (<< "Sending to requri: " << request.header(h_RequestLine).uri());
            // skip the rest of the monkeys
            return RequestProcessor::SkipThisChain;	            
         }
         else  // nobody home with that GRUU
         {
            // send 480 Temporarily Unavailable
            resip::SipMessage response;
            InfoLog (<< *this << ": no contacts matching this GRUU found.  Gruu" << uri << ", sending 480");
            Helper::makeResponse(response, request, 480); 
            context.sendResponse(response);
            return RequestProcessor::SkipThisChain;
         }
      }
   }
   return RequestProcessor::Continue;
}
   

void
GruuMonkey::dump(std::ostream &os) const
{
  os << "Gruu Monkey" << std::endl;
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
