
#ifndef __RADIUSServerAuthManager_h
#define __RADIUSServerAuthManager_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_RADIUS_CLIENT

#include "rutil/RADIUSDigestAuthenticator.hxx"
#include "resip/dum/ServerAuthManager.hxx"

namespace resip
{

class RADIUSServerAuthManager : public resip::ServerAuthManager
{
   private:
      resip::DialogUsageManager& dum;

   public:
      RADIUSServerAuthManager(resip::DialogUsageManager& dum,
                              TargetCommand::Target& target,
                              const Data& configurationFile,
                              bool challengeThirdParties = true,
                              const Data& staticRealm = "");
      virtual ~RADIUSServerAuthManager();

   protected:
      void requestCredential(const resip::Data& user, const resip::Data& realm,
         const resip::SipMessage& msg, const resip::Auth& auth,
         const resip::Data& transactionId);
      bool useAuthInt() const;

      void onAuthSuccess(const resip::SipMessage& msg);
      void onAuthFailure(resip::ServerAuthManager::AuthFailureReason reason, const resip::SipMessage& msg);
};

class MyRADIUSDigestAuthListener : public RADIUSDigestAuthListener
{
   private:
      resip::Data user;
      resip::Data realm;
      resip::TransactionUser& tu;
      resip::Data transactionId;
   public:
      MyRADIUSDigestAuthListener(const resip::Data& user, const resip::Data& realm,
         resip::TransactionUser& tu, const resip::Data& transactionId);
      virtual ~MyRADIUSDigestAuthListener();
      void onSuccess(const resip::Data& rpid);
      void onAccessDenied();
      void onError();
};

}

#endif

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

