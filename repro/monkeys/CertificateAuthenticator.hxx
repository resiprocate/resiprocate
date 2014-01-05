#if !defined(RESIP_CERTIFICATE_AUTHENTICATOR_HXX)
#define RESIP_CERTIFICATE_AUTHENTICATOR_HXX 

#include <set>

#include "rutil/Data.hxx"
#include "rutil/KeyValueStore.hxx"
#include "resip/dum/TlsPeerAuthManager.hxx"
#include "repro/AclStore.hxx"
#include "repro/Processor.hxx"
#include "repro/Dispatcher.hxx"
#include "repro/ProxyConfig.hxx"

namespace resip
{
   class SipStack;
}

namespace repro
{
  class CertificateAuthenticator : public Processor
  {
    public:
      static resip::KeyValueStore::Key mCertificateVerifiedKey;

      CertificateAuthenticator(ProxyConfig& config, resip::SipStack* stack, AclStore& aclStore, bool thirdPartyRequiresCertificate = true);
      CertificateAuthenticator(ProxyConfig& config, resip::SipStack* stack, AclStore& aclStore, bool thirdPartyRequiresCertificate, resip::CommonNameMappings& commonNameMappings);
      ~CertificateAuthenticator();

      virtual processor_action_t process(RequestContext &);
      virtual void dump(EncodeStream &os) const;

    private:
      bool isTrustedSource(const std::list<resip::Data>& peerNames);
      bool authorizedForThisIdentity(RequestContext& context, const std::list<resip::Data>& peerNames, resip::Uri &fromUri);

      AclStore& mAclStore;
      bool mThirdPartyRequiresCertificate;
      resip::CommonNameMappings mCommonNameMappings;
  };
  
}
#endif

/* ====================================================================
 *
 * Copyright (c) 2012 Daniel Pocock  All rights reserved.
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

