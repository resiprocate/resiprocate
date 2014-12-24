#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <sstream>

#include "resip/dum/ChallengeInfo.hxx"
#include "resip/dum/ClientInviteSession.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/UserAuthInfo.hxx"
#include "resip/dum/RADIUSServerAuthManager.hxx"
#include "resip/stack/ExtensionHeader.hxx"
#include "rutil/Data.hxx"
#include "rutil/Logger.hxx"
#include "rutil/MD5Stream.hxx"

#ifdef USE_RADIUS_CLIENT

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

RADIUSServerAuthManager::RADIUSServerAuthManager(
                            resip::DialogUsageManager& dum,
                            TargetCommand::Target& target,
                            const Data& configurationFile,
                            bool challengeThirdParties,
                            const Data& staticRealm) :
   ServerAuthManager(dum, target, challengeThirdParties, staticRealm),
   dum(dum)
{
   RADIUSDigestAuthenticator::init(
      configurationFile.empty() ? 0 : configurationFile.c_str());
}

RADIUSServerAuthManager::~RADIUSServerAuthManager()
{
}

void
RADIUSServerAuthManager::requestCredential(
   const resip::Data& user,
   const resip::Data& realm,
   const resip::SipMessage& msg,
   const resip::Auth& auth,
   const resip::Data& transactionId)
{
   DebugLog(<<"RADIUSServerAuthManager::requestCredential, uri = " << msg.header(h_RequestLine).uri() << " authUser = " << user);

   MyRADIUSDigestAuthListener *radiusListener = NULL;
   try
   {
      radiusListener = new MyRADIUSDigestAuthListener(user, realm, dum, transactionId);
      Data radiusUser = user;
      DebugLog(<< "radiusUser = " << radiusUser.c_str() << ", " << "user = " << user.c_str());

      resip_assert(msg.isRequest());
      Data reqUri = auth.param(p_uri);
      Data reqMethod = Data(resip::getMethodName(msg.header(h_RequestLine).getMethod()));

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
         ErrLog(<<"RADIUSServerAuthManager::requestCredential, uri = " << msg.header(h_RequestLine).uri() <<" failed to start thread, error = " << result);
      }
   }
   catch(...)
   {
      WarningLog(<<"RADIUSServerAuthManager::requestCredential, uri = " << msg.header(h_RequestLine).uri() <<" exception");
      delete radiusListener;
   }
}

bool
RADIUSServerAuthManager::useAuthInt() const
{
   return true;
}

void
RADIUSServerAuthManager::onAuthSuccess(const resip::SipMessage& msg)
{
}

void 
RADIUSServerAuthManager::onAuthFailure(resip::ServerAuthManager::AuthFailureReason reason, const resip::SipMessage& msg)
{
   Data failureMsg("unknown failure");
   switch(reason) {
      case InvalidRequest: 
         failureMsg = Data("InvalidRequest");
         break;
      case BadCredentials:
         failureMsg = Data("BadCredentials");
         break;
      case Error:
         failureMsg = Data("Error");
         break;
   }
   Tuple sourceTuple = msg.getSource();
   Data sourceIp = Data(inet_ntoa(sourceTuple.toGenericIPAddress().v4Address.sin_addr));
   WarningLog(<<"auth failure: " << failureMsg
              << ": src IP=" << sourceIp
              << ", uri=" << msg.header(h_RequestLine).uri().user()
              << ", from=" <<msg.header(h_From).uri().user()
              << ", to=" << msg.header(h_To).uri().user());
}

MyRADIUSDigestAuthListener::MyRADIUSDigestAuthListener(
   const resip::Data& user,
   const resip::Data& realm,
   resip::TransactionUser& tu,
   const resip::Data& transactionId) :
   user(user),
   realm(realm),
   tu(tu),
   transactionId(transactionId)
{
}

MyRADIUSDigestAuthListener::~MyRADIUSDigestAuthListener()
{
}

void
MyRADIUSDigestAuthListener::onSuccess(const resip::Data& rpid)
{
   DebugLog(<<"MyRADIUSDigestAuthListener::onSuccess");
   if(!rpid.empty())
   {
      DebugLog(<<"MyRADIUSDigestAuthListener::onSuccess rpid = " << rpid.c_str());
   }
   else
   {
      DebugLog(<<"MyRADIUSDigestAuthListener::onSuccess, no rpid");
   }
   UserAuthInfo *uai = new UserAuthInfo(user, realm, UserAuthInfo::DigestAccepted, transactionId);
   tu.post(uai);
}

void
MyRADIUSDigestAuthListener::onAccessDenied()
{
   DebugLog(<<"MyRADIUSDigestAuthListener::onAccessDenied");
   UserAuthInfo *uai = new UserAuthInfo(user, realm, UserAuthInfo::DigestNotAccepted, transactionId);
   tu.post(uai);
}

void
MyRADIUSDigestAuthListener::onError()
{
   WarningLog(<<"MyRADIUSDigestAuthListener::onError");
   UserAuthInfo *uai = new UserAuthInfo(user, realm, UserAuthInfo::Error, transactionId);
   tu.post(uai);
}

#endif

/* ====================================================================
 *
 * Copyright 2008-2013 Daniel Pocock http://danielpocock.com
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

