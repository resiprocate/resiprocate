#if !defined(RESIP_AUTHENTICATORFACTORY_HXX)
#define RESIP_AUTHENTICATORFACTORY_HXX 

#include <memory>

#include "rutil/SharedPtr.hxx"
#include "resip/dum/DumFeature.hxx"
#include "resip/dum/ServerAuthManager.hxx"
#include "repro/Dispatcher.hxx"
#include "repro/Processor.hxx"

namespace repro
{

class AuthenticatorFactory
{
public:
   AuthenticatorFactory() {};
   virtual ~AuthenticatorFactory() {};

   virtual void setDum(resip::DialogUsageManager* dum) = 0;

   virtual bool certificateAuthEnabled() = 0;

   virtual resip::SharedPtr<resip::DumFeature> getCertificateAuthManager() = 0;
   virtual std::auto_ptr<Processor> getCertificateAuthenticator() = 0;

   virtual bool digestAuthEnabled() = 0;

   virtual resip::SharedPtr<resip::ServerAuthManager> getServerAuthManager() = 0;
   virtual std::auto_ptr<Processor> getDigestAuthenticator() = 0;
   virtual Dispatcher* getDispatcher() = 0;
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

