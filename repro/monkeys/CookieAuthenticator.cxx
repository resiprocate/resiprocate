
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "rutil/DnsUtil.hxx"
#include "resip/stack/Message.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Auth.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/stun/Stun.hxx"

#include "repro/monkeys/CookieAuthenticator.hxx"
#include "repro/monkeys/IsTrustedNode.hxx"
#include "repro/RequestContext.hxx"
#include "repro/Proxy.hxx"
#include "repro/UserInfoMessage.hxx"
#include "repro/UserStore.hxx"
#include "repro/Dispatcher.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

#include <time.h>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;


CookieAuthenticator::CookieAuthenticator(const Data& wsCookieAuthSharedSecret,
                                         const Data& wsCookieExtraHeaderName,
                                         resip::SipStack* stack) :
   Processor("CookieAuthenticator"),
   mWsCookieExtraHeader(wsCookieExtraHeaderName.empty() ? 0 : new resip::ExtensionHeader(wsCookieExtraHeaderName))
{
}

CookieAuthenticator::~CookieAuthenticator()
{
}

repro::Processor::processor_action_t
CookieAuthenticator::process(repro::RequestContext &rc)
{
   DebugLog(<< "Monkey handling request: " << *this << "; reqcontext = " << rc);

   Message *message = rc.getCurrentEvent();

   SipMessage *sipMessage = dynamic_cast<SipMessage*>(message);
   Proxy &proxy = rc.getProxy();

   if (sipMessage)
   {
      // Only check message coming over WebSockets
      if(!isWebSocket(sipMessage->getReceivedTransportTuple().getType()))
      {
         return Continue;
      }

      if (sipMessage->method() == ACK ||
          sipMessage->method() == BYE)
      {
         return Continue;
      }

      if(!sipMessage->header(h_From).isWellFormed() ||
         sipMessage->header(h_From).isAllContacts() )
      {
         InfoLog(<<"Malformed From header: cannot verify against cookie. Rejecting.");
         rc.sendResponse(*auto_ptr<SipMessage>
                         (Helper::makeResponse(*sipMessage, 400, "Malformed From header")));
         return SkipAllChains;
      }

      const WsCookieContext &wsCookieContext = *(sipMessage->getWsCookieContext());
      if (proxy.isMyDomain(sipMessage->header(h_From).uri().host()))
      {
         if(authorizedForThisIdentity(sipMessage->header(h_RequestLine).method(), wsCookieContext, sipMessage->header(h_From).uri(), sipMessage->header(h_To).uri()))
         {
            if(mWsCookieExtraHeader.get() && sipMessage->exists(*mWsCookieExtraHeader))
            {
               ParserContainer<StringCategory>& extra = sipMessage->header(*mWsCookieExtraHeader);
               StringCategory& sc = extra.front();
               if(sc.value() == wsCookieContext.getWsSessionExtra())
               {
                  return Continue;
               }
               else
               {
                  WarningLog(<<"mWsCookieExtraHeader does not match wsCookieContext value");
               }
            }
            else
            {
               return Continue;
            }
         }
         rc.sendResponse(*auto_ptr<SipMessage>
                           (Helper::makeResponse(*sipMessage, 403, "Authentication against cookie failed")));
         return SkipAllChains;
      }
      else
      {
         rc.sendResponse(*auto_ptr<SipMessage>
                           (Helper::makeResponse(*sipMessage, 403, "Authentication against cookie failed")));
         return SkipAllChains;
      }
   }

   return Continue;
}

bool
CookieAuthenticator::cookieUriMatch(const resip::Uri &first, const resip::Uri &second)
{
   return(
      (isEqualNoCase(first.user(), second.user()) || first.user() == "*") &&
      (isEqualNoCase(first.host(), second.host()) || first.host() == "*")
         );
}

bool
CookieAuthenticator::authorizedForThisIdentity(const MethodTypes method,
                                                const WsCookieContext& wsCookieContext,
                                                resip::Uri &fromUri,
                                                resip::Uri &toUri)
{
   if(difftime(wsCookieContext.getExpiresTime(), time(NULL)) < 0)
   {
      WarningLog(<< "Received expired cookie");
      return false;
   }

   Uri wsFromUri = wsCookieContext.getWsFromUri();
   Uri wsDestUri = wsCookieContext.getWsDestUri();
   if(cookieUriMatch(wsFromUri, fromUri))
   {
      DebugLog(<< "Matched cookie source URI field" << wsFromUri << " against request From header field URI " << fromUri);
      // When registering, "From" URI == "To" URI, so we can ignore the
      // "To" URI restriction from the cookie when processing REGISTER
      if(method == REGISTER && isEqualNoCase(fromUri.user(), toUri.user()) && isEqualNoCase(fromUri.host(), toUri.host()))
      {
         return true;
      }
      if(cookieUriMatch(wsDestUri, toUri))
      {
         DebugLog(<< "Matched cookie destination URI field" << wsDestUri << " against request To header field URI " << toUri);
         return true;
      }
   }

   // catch-all: access denied
   return false;
}

void
CookieAuthenticator::dump(EncodeStream &os) const
{
   os << "CookieAuthentication monkey" << std::endl;
}

/* ====================================================================
 * BSD License
 *
 * Copyright (c) 2013 Catalin Constantin Usurelu  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
