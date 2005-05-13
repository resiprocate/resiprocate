
#ifdef WIN32
#include <db_cxx.h>
#else 
#include <db4/db_cxx.h>
#endif

#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/Message.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Auth.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/dum/UserAuthInfo.hxx"

#include "repro/UserStore.hxx"
#include "repro/monkeys/DigestAuthenticator.hxx"
#include "repro/RequestContext.hxx"
#include "repro/Proxy.hxx"


#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

DigestAuthenticator::DigestAuthenticator()
{
}

DigestAuthenticator::~DigestAuthenticator()
{
}

repro::RequestProcessor::processor_action_t
DigestAuthenticator::handleRequest(repro::RequestContext &rc)
{
   DebugLog(<< "Monkey handling request: " << *this << "; reqcontext = " << rc);
   
   Message *message = rc.getCurrentEvent();
   
   SipMessage *sipMessage = dynamic_cast<SipMessage*>(message);
   UserAuthInfo *userAuthInfo = dynamic_cast<UserAuthInfo*>(message);
   
   if (sipMessage)
   {
      if (!sipMessage->exists(h_ProxyAuthorizations))
      {
         challengeRequest(rc, false);
         return SkipAllChains;
      }
      else
      {
         return requestUserAuthInfo(rc);
      }
   }
   else if (userAuthInfo)
   {
      // Handle response from user authentication database
      sipMessage = &rc.getOriginalRequest();
      const Data& a1 = userAuthInfo->getA1();
      const Data& realm = userAuthInfo->getRealm();
      const Data& user = userAuthInfo->getUser();
      InfoLog (<< "Received user auth info for " << user << " at realm " << realm);

      pair<Helper::AuthResult,Data> result =
         Helper::advancedAuthenticateRequest(*sipMessage, realm, a1, 3000); // was 15

      switch (result.first)
      {
         case Helper::Failed:
            InfoLog (<< "Authentication failed for " << user << " at realm " << realm);
            rc.sendResponse(*auto_ptr<SipMessage>
                            (Helper::makeResponse(*sipMessage, 403)));
            return SkipAllChains;
        
            // !abr! Eventually, this should just append a counter to
            // the nonce, and increment it on each challenge. 
            // If this count is smaller than some reasonable limit,
            // then we re-challenge; otherwise, we send a 403 instead.

         case Helper::Authenticated:
            InfoLog (<< "Authentication ok for " << user);
            sipMessage->remove(h_Authorizations);
            sipMessage->remove(h_ProxyAuthorizations);
            rc.setDigestIdentity(user);
            if (sipMessage->header(h_From).uri().user() == user &&
                sipMessage->header(h_From).uri().host() == realm)
            {
               sipMessage->header(h_Identity).value() = Data::Empty;
               static Data http("http://");
               static Data post(":5080/cert?domain=");
               sipMessage->header(h_IdentityInfo).uri() = http 
                  + DnsUtil::getLocalHostName() 
                  + post + realm;
               InfoLog (<< "Identity-Info=" << sipMessage->header(h_IdentityInfo).uri());
               InfoLog (<< *sipMessage);
            }
            
            return Continue;

         case Helper::Expired:
            InfoLog (<< "Authentication expired for " << user);
            challengeRequest(rc, true);
            return SkipAllChains;

         case Helper::BadlyFormed:
            InfoLog (<< "Authentication nonce badly formed for " << user);
            rc.sendResponse(*auto_ptr<SipMessage>
                            (Helper::makeResponse(*sipMessage, 403,
                                                  "Where on earth did you get that nonce?")));
            return SkipAllChains;
      }
   }

   return Continue;
}

void
DigestAuthenticator::challengeRequest(repro::RequestContext &rc,
                                      bool stale)
{
   Message *message = rc.getCurrentEvent();
   SipMessage *sipMessage = dynamic_cast<SipMessage*>(message);
   assert(sipMessage);

   Data realm = getRealm(rc);

   SipMessage *challenge = Helper::makeProxyChallenge(*sipMessage, realm, 
                                                      true, stale);
   rc.sendResponse(*challenge);

   delete challenge;
}


repro::RequestProcessor::processor_action_t
DigestAuthenticator::requestUserAuthInfo(repro::RequestContext &rc)
{
   Message *message = rc.getCurrentEvent();
   SipMessage *sipMessage = dynamic_cast<SipMessage*>(message);
   assert(sipMessage);

   UserStore& database = rc.getProxy().getUserStore();
   Data realm = getRealm(rc);

   // Extract the user from the appropriate Proxy-Authorization header
   Auths &authorizationHeaders = sipMessage->header(h_ProxyAuthorizations); 
   Auths::iterator i;
   Data user;

   for (i = authorizationHeaders.begin();
        i != authorizationHeaders.end(); i++)
   {
      if (    i->exists(p_realm) && 
              i->param(p_realm) == realm
              &&  i->exists(p_username))
      {
         user = i->param(p_username);

         InfoLog (<< "Request user auth info for "  << user
                  << " at realm " << realm);
         break;
      }
   }

   if (!user.empty())
   {
      database.requestUserAuthInfo(user, realm, rc.getTransactionId(), rc.getProxy());
      return WaitingForEvent;
   }
   else
   {
      challengeRequest(rc, false);
      return SkipAllChains;
   }
}

resip::Data
DigestAuthenticator::getRealm(RequestContext &rc)
{
   Data realm;

   Proxy &proxy = rc.getProxy();
   Message *message = rc.getCurrentEvent();
   SipMessage *sipMessage = dynamic_cast<SipMessage*>(message);
   assert(sipMessage);

   // (1) Check Preferred Identity
   if (sipMessage->exists(h_PPreferredIdentities))
   {
      // !abr! Add this when we get a chance
   }

   // (2) Check From domain
   if (proxy.isMyDomain(sipMessage->header(h_From).uri().host()))
   {
      return sipMessage->header(h_From).uri().host();
   }

   // (3) Check Top Route Header
   if (sipMessage->exists(h_Routes))
   {
      // !abr! Add this when we get a chance
   }

   // (4) Punt: Use Request URI
   return sipMessage->header(h_RequestLine).uri().host();
}

void
DigestAuthenticator::dump(std::ostream &os) const
{
   os << "DigestAuthentication monkey" << std::endl;
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
