
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "rutil/DnsUtil.hxx"
#include "resip/stack/Message.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Auth.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"

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

CertificateAuthenticator::CertificateAuthenticator(ProxyConfig& config,
                                                   resip::SipStack* stack) :
   Processor("CertificateAuthenticator")
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
      if(sipMessage->isExternal() && sipMessage->getSource().getType() != TLS)
      {
         DebugLog(<<"Can't validate certificate on non-TLS connection");
         return Continue;
      }
      
      if (proxy.isMyDomain(sipMessage->header(h_From).uri().host()))
      {
         if (!rc.getKeyValueStore().getBoolValue(IsTrustedNode::mFromTrustedNodeKey))
         {
            if(authorizedForThisIdentity(sipMessage->getTlsPeerNames(), sipMessage->header(h_From).uri()))
               return Continue;
            rc.sendResponse(*auto_ptr<SipMessage>
                            (Helper::makeResponse(*sipMessage, 403, "Authentication Failed for peer cert")));
            return SkipAllChains;
         }
         else
            return Continue;
      }
      else
      {
         if(authorizedForThisIdentity(sipMessage->getTlsPeerNames(), sipMessage->header(h_From).uri()))
            return Continue;
         rc.sendResponse(*auto_ptr<SipMessage>
                            (Helper::makeResponse(*sipMessage, 403, "Authentication Failed for peer cert")));
         return SkipAllChains;
      }
   }

   return Continue;
}

bool
CertificateAuthenticator::authorizedForThisIdentity(const std::list<Data>& peerNames,
                                                resip::Uri &fromUri)
{
   // If we are using TLS and client certificates are optional,
   // clients who connect without a certificate won't supply
   // any values in peerNames.  As the TLS mode is `optional',
   // no further checks are done here
   if(peerNames.size() == 0)
      return true;

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
 * BSD License
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
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR
 * OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
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
 */
