
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

#include "repro/monkeys/CertificateAuthenticator.hxx"
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

KeyValueStore::Key CertificateAuthenticator::mCertificateVerifiedKey = Proxy::allocateRequestKeyValueStoreKey();

CertificateAuthenticator::CertificateAuthenticator(ProxyConfig& config,
                                                   resip::SipStack* stack,
                                                   AclStore& aclStore,
                                                   bool thirdPartyRequiresCertificate) :
   Processor("CertificateAuthenticator"),
   mAclStore(aclStore),
   mThirdPartyRequiresCertificate(thirdPartyRequiresCertificate)
{
}

CertificateAuthenticator::CertificateAuthenticator(ProxyConfig& config,
                                                   resip::SipStack* stack,
                                                   AclStore& aclStore,
                                                   bool thirdPartyRequiresCertificate,
                                                   CommonNameMappings& commonNameMappings) :
   Processor("CertificateAuthenticator"),
   mAclStore(aclStore),
   mThirdPartyRequiresCertificate(thirdPartyRequiresCertificate),
   mCommonNameMappings(commonNameMappings)
{
}

CertificateAuthenticator::~CertificateAuthenticator()
{
}

repro::Processor::processor_action_t
CertificateAuthenticator::process(repro::RequestContext &rc)
{
   DebugLog(<< "Monkey handling request: " << *this << "; reqcontext = " << rc);
   
   Message *message = rc.getCurrentEvent();
   
   SipMessage *sipMessage = dynamic_cast<SipMessage*>(message);
   Proxy &proxy = rc.getProxy();
   
   if (sipMessage)
   {
      if (sipMessage->method() == ACK ||
            sipMessage->method() == BYE)
      {
         return Continue;
      }

      // if there was no Proxy-Auth header already, and the request is purportedly From
      // one of our domains, send a challenge, unless this is from a trusted node in one
      // of "our" domains (ex: from a gateway).
      //
      // Note that other monkeys can still challenge the request later if needed 
      // for other reasons (for example, the StaticRoute monkey)
      if(!sipMessage->header(h_From).isWellFormed() ||
         sipMessage->header(h_From).isAllContacts() )
      {
         InfoLog(<<"Malformed From header: cannot verify against any certificate. Rejecting.");
         rc.sendResponse(*auto_ptr<SipMessage>
                         (Helper::makeResponse(*sipMessage, 400, "Malformed From header")));
         return SkipAllChains;         
      }

      // Get the certificate from the peer
      if(sipMessage->isExternal() && !isSecure(sipMessage->getSource().getType()))
      {
         DebugLog(<<"Can't validate certificate on non-TLS connection");
         return Continue;
      }
      
      const std::list<resip::Data> &peerNames = sipMessage->getTlsPeerNames();

      if(isTrustedSource(peerNames))
      {
         DebugLog(<< "Matched trusted peer by certificate in ACL");
         rc.getKeyValueStore().setBoolValue(CertificateAuthenticator::mCertificateVerifiedKey, true);
         // Simulate the behavior of IsTrustedNode monkey:
         rc.getKeyValueStore().setBoolValue(IsTrustedNode::mFromTrustedNodeKey, true);
         return Continue;
      }

      if (proxy.isMyDomain(sipMessage->header(h_From).uri().host()))
      {
         if (!rc.getKeyValueStore().getBoolValue(IsTrustedNode::mFromTrustedNodeKey))
         {
            // peerNames is empty if client certificate mode is `optional'
            // or if the message didn't come in on TLS transport
            if(peerNames.empty())
               return Continue;
            if(authorizedForThisIdentity(rc, peerNames, sipMessage->header(h_From).uri()))
            {
               rc.getKeyValueStore().setBoolValue(CertificateAuthenticator::mCertificateVerifiedKey, true);
               return Continue;
            }
            rc.sendResponse(*auto_ptr<SipMessage>
                            (Helper::makeResponse(*sipMessage, 403, "Authentication Failed for peer cert")));
            return SkipAllChains;
         }
         else
            return Continue;
      }
      else
      {
         // peerNames is empty if client certificate mode is `optional'
         // or if the message didn't come in on TLS transport
         if(peerNames.empty())
         {
            if(mThirdPartyRequiresCertificate)
            {
               rc.sendResponse(*auto_ptr<SipMessage>
                            (Helper::makeResponse(*sipMessage, 403, "Mutual TLS required to handle that message")));
               return SkipAllChains;
            }
            else
               return Continue;
         }
         if(authorizedForThisIdentity(rc, peerNames, sipMessage->header(h_From).uri()))
         {
            rc.getKeyValueStore().setBoolValue(CertificateAuthenticator::mCertificateVerifiedKey, true);
            return Continue;
         }
         rc.sendResponse(*auto_ptr<SipMessage>
                            (Helper::makeResponse(*sipMessage, 403, "Authentication Failed for peer cert")));
         return SkipAllChains;
      }
   }

   return Continue;
}

bool
CertificateAuthenticator::isTrustedSource(const std::list<Data>& peerNames)
{
   return mAclStore.isTlsPeerNameTrusted(peerNames);
}

bool
CertificateAuthenticator::authorizedForThisIdentity(RequestContext& context, const std::list<Data>& peerNames,
                                                resip::Uri &fromUri)
{
   Data aor = fromUri.getAorNoPort();
   Data domain = fromUri.host();

   std::list<Data>::const_iterator it = peerNames.begin();
   for(; it != peerNames.end(); ++it)
   {
      const Data& i = *it;
      if(i == aor)
      {
         DebugLog(<< "Matched certificate name " << i << " against full AoR " << aor);
         return true;
      }
      if(i == domain)
      {
         DebugLog(<< "Matched certificate name " << i << " against domain " << domain);
         return true;
      }
      CommonNameMappings::iterator _mapping =
         mCommonNameMappings.find(i);
      if(_mapping != mCommonNameMappings.end())
      {
         DebugLog(<< "CN mapping(s) exist for the certificate " << i);
         PermittedFromAddresses& permitted = _mapping->second;
         if(permitted.find(aor) != permitted.end())
         {
            DebugLog(<< "Matched certificate name " << i << " against full AoR " << aor << " by common name mappings");
            return true;
         }
         if(permitted.find(domain) != permitted.end())
         {
            DebugLog(<< "Matched certificate name " << i << " against domain " << domain << " by common name mappings");
            return true;
         }
      }
      DebugLog(<< "Certificate name " << i << " doesn't match AoR " << aor << " or domain " << domain);
   }

   // catch-all: access denied
   return false;
}

void
CertificateAuthenticator::dump(EncodeStream &os) const
{
   os << "CertificateAuthentication monkey" << std::endl;
}

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
