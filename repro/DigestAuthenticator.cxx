#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/Auth.hxx"
#include "repro/DigestAuthenticator.hxx"



using namespace resip;
using namespace repro;
using namespace std;

processor_action_t
DigestAuthenticator::handleRequest(repro::RequestContext &rc)
{
  Message *message = getCurrentEvent();

  SipMessage *sipMessage = dynamic_cast<SipMessage*>(message);
  UserAuthInfo *userAuthInfo = dynamic_cast<UserAuthInfo*>(message);

  if (sipMessage)
  {
    if (!sipMessage.exists(h_ProxyAuthorizations))
    {
      challengeRequest(rc);
      return SkipAllChains;
    }
    else
    {
      requestUserAuthInfo(rc);
      return WaitingForEvent;
    }
  }
  else if (userAuthInfo)
  {
    // Handle response from user authentication database
    sipMessage = rc.getOriginalMessage();
    Helper::AuthResult result =
      Helper::authenticateRequest(sipMessage, a1, 2);

    switch (result)
    {
      case Failed:
        // XXX Send 403
        return SkipAllChains;

        // !abr! Eventually, this should just append a counter to
        // the nonce, and increment it on each challenge. 
        // If this count is smaller than some reasonable limit,
        // then we re-challenge; otherwise, we send a 403 instead.

      case Authenticated:
        // XXX Add user info to request context
        return Continue;

      case Expired:
        challengeRequest(rc);
        return SkipAllChains;

      case BadlyFormed:
        // XXX Send 403
        return SkipAllChains;
    }
  }

  return Continue;
}

void
DigestAuthenticator::challengeRequest(repro::RequestContext &rc)
{
  Data realm = sipMessage.
  Helper::makeProxyChallenge();
  rc.getProxy().sendResponse(challenge);
}

void
DigestAuthenticator::requestUserAuthInfo(repro::RequestContext &rc)
{
  UserDB &database = rc.getProxy().getUserDB();
  Auth &authorizationHeader = sipMessage.header(h_ProxyAuthorizations);
  Data user;
  Data realm;
  database.requestUserAuthInfo(user, realm);
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
