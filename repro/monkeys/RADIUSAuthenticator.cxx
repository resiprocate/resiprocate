#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "rutil/DnsUtil.hxx"
#include "resip/stack/Message.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Auth.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"

#include "repro/monkeys/RADIUSAuthenticator.hxx"
#include "repro/monkeys/IsTrustedNode.hxx"
#include "repro/RequestContext.hxx"
#include "repro/Proxy.hxx"
#include "repro/UserInfoMessage.hxx"
#include "repro/UserStore.hxx"
#include "repro/Dispatcher.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

#ifdef USE_RADIUS_CLIENT

RADIUSAuthenticator::RADIUSAuthenticator(ProxyConfig& config, const resip::Data& configurationFile, const Data& staticRealm) :
   DigestAuthenticator(config, 0, staticRealm)
{
   RADIUSDigestAuthenticator::init(
      configurationFile.empty() ? 0 : configurationFile.c_str());
}

RADIUSAuthenticator::~RADIUSAuthenticator()
{
}

// Start a RADIUS query to check the response supplied by the user
// Based on the code in resip/dum/RADIUSServerAuthManager.cxx
Processor::processor_action_t
RADIUSAuthenticator::requestUserAuthInfo(RequestContext &rc, const Auth& auth, UserInfoMessage *userInfo)
{
   ReproRADIUSDigestAuthListener *radiusListener = 0;
   Message *message = rc.getCurrentEvent();
   SipMessage *sipMessage = dynamic_cast<SipMessage*>(message);
   resip_assert(sipMessage);

   try
   {
      radiusListener = new ReproRADIUSDigestAuthListener(userInfo, rc.getProxy());
      const Data& user = userInfo->user();
      const Data& realm = userInfo->realm();
      Data radiusUser = user;
      DebugLog(<< "radiusUser = " << radiusUser.c_str() << ", " << "user = " << user.c_str());
      resip_assert(sipMessage->isRequest());
      Data reqUri = auth.param(p_uri);
      Data reqMethod = resip::getMethodName(sipMessage->header(h_RequestLine).getMethod());
      RADIUSDigestAuthenticator *radius = NULL;
      if(auth.exists(p_qop))
      {
         if(auth.param(p_qop) == Symbols::auth)
         {
            Data myQop("auth");
            radius = new RADIUSDigestAuthenticator(
               radiusUser, user, realm, auth.param(p_nonce),
               reqUri, reqMethod, myQop, auth.param(p_nc), auth.param(p_cnonce),
               auth.param(p_response),
               radiusListener);
         }
         else if(auth.param(p_qop) == Symbols::authInt)
         {
            Data myQop("auth-int");
            radius = new RADIUSDigestAuthenticator(
               radiusUser, user, realm, auth.param(p_nonce),
               reqUri, reqMethod, myQop, auth.param(p_nc), auth.param(p_cnonce),
               auth.param(p_opaque),
               auth.param(p_response),
               radiusListener);
         }
      }
      if(radius == NULL)
      {
         radius = new RADIUSDigestAuthenticator(
            radiusUser, user, realm, auth.param(p_nonce),
            reqUri, reqMethod,
            auth.param(p_response),
            radiusListener);
      }
      int result = radius->doRADIUSCheck();
      if(result < 0)
      {
         ErrLog(<<"RADIUSServerAuthManager::requestCredential, uri = " << reqUri <<" failed to start thread, error = " << result);
         rc.sendResponse(*auto_ptr<SipMessage>
                         (Helper::makeResponse(*sipMessage, 500, "Auth failed")));
         return SkipAllChains;
      }
   }
   catch(...)
   {
      WarningLog(<<"RADIUSServerAuthManager::requestCredential, unexpected exception thrown");
      delete radiusListener;
      rc.sendResponse(*auto_ptr<SipMessage>
                      (Helper::makeResponse(*sipMessage, 500, "Auth failed")));
      return SkipAllChains;
   }
   return WaitingForEvent;
}

ReproRADIUSDigestAuthListener::ReproRADIUSDigestAuthListener(
   UserInfoMessage *userInfo, resip::TransactionUser& tu) :
   mUserInfo(userInfo),
   mTu(tu)
{
}

ReproRADIUSDigestAuthListener::~ReproRADIUSDigestAuthListener()
{
}

void
ReproRADIUSDigestAuthListener::onSuccess(const resip::Data& rpid)
{
   DebugLog(<<"ReproRADIUSDigestAuthListener::onSuccess");
   if(!rpid.empty())
   {
      DebugLog(<<"ReproRADIUSDigestAuthListener::onSuccess rpid = " << rpid.c_str());
   }
   else
   {
      DebugLog(<<"ReproRADIUSDigestAuthListener::onSuccess, no rpid");
   }
   mUserInfo->setMode(UserAuthInfo::DigestAccepted);
   mTu.post(mUserInfo);
}

void
ReproRADIUSDigestAuthListener::onAccessDenied()
{
   DebugLog(<<"ReproRADIUSDigestAuthListener::onAccessDenied");
   mUserInfo->setMode(UserAuthInfo::DigestNotAccepted);
   mTu.post(mUserInfo);
}

void
ReproRADIUSDigestAuthListener::onError()
{
   WarningLog(<<"ReproRADIUSDigestAuthListener::onError");
   mUserInfo->setMode(UserAuthInfo::Error);
   mTu.post(mUserInfo);
}

#endif

/* ====================================================================
 *
 * Copyright 2013 Daniel Pocock http://danielpocock.com
 * All rights reserved.
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
 *
 */

