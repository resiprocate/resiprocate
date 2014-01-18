#if !defined(RESIP_REPROAUTHENTICATORFACTORY_HXX)
#define RESIP_REPROAUTHENTICATORFACTORY_HXX 

#include <memory>
#include <set>

#include "rutil/Data.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/TlsPeerAuthManager.hxx"
#include "AuthenticatorFactory.hxx"
#include "ProxyConfig.hxx"

namespace repro
{

class ReproAuthenticatorFactory : public AuthenticatorFactory
{
public:
   ReproAuthenticatorFactory(ProxyConfig& proxyConfig, resip::SipStack& sipStack, resip::DialogUsageManager* dum);
   virtual ~ReproAuthenticatorFactory();

   virtual void setDum(resip::DialogUsageManager* dum) { mDum = dum; };

   virtual bool certificateAuthEnabled() { return mEnableCertAuth; };

   virtual resip::SharedPtr<resip::DumFeature> getCertificateAuthManager();
   virtual std::auto_ptr<Processor> getCertificateAuthenticator();

   virtual bool digestAuthEnabled() { return mEnableDigestAuth; };

   virtual resip::SharedPtr<resip::ServerAuthManager> getServerAuthManager();
   virtual std::auto_ptr<Processor> getDigestAuthenticator();

   virtual Dispatcher* getDispatcher();

private:
   void init();
   void loadCommonNameMappings();

   ProxyConfig& mProxyConfig;
   resip::SipStack& mSipStack;
   resip::DialogUsageManager* mDum;

   bool mEnableCertAuth;
   bool mEnableDigestAuth;
   bool mEnableRADIUS;

   resip::Data mRADIUSConfiguration;

   resip::Data mStaticRealm;

   // Maintains existing behavior for non-TLS cert auth users
   bool mDigestChallengeThirdParties;

   resip::CommonNameMappings mCommonNameMappings;

   std::auto_ptr<Dispatcher> mAuthRequestDispatcher;

   resip::SharedPtr<resip::DumFeature> mCertificateAuthManager;
   resip::SharedPtr<resip::ServerAuthManager> mServerAuthManager;
};

}

#endif

/* ====================================================================
 * BSD License
 *
 * Copyright (c) 2013 Daniel Pocock http://danielpocock.com All rights reserved.
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

