#include "resip/stack/ssl/Security.hxx"
#include "resip/stack/X509Contents.hxx"
#include "resip/dum/ServerPublication.hxx"
#include "repro/stateAgents/CertPublicationHandler.hxx"

using namespace repro;
using namespace resip;

CertPublicationHandler::CertPublicationHandler(Security& security) : mSecurity(security)
{
}

void 
CertPublicationHandler::onInitial(ServerPublicationHandle h, 
                                  const Data& etag, 
                                  const SipMessage& pub, 
                                  const Contents* contents,
                                  const SecurityAttributes* attrs, 
                                  UInt32 expires)
{
   add(h, contents);
}

void 
CertPublicationHandler::onExpired(ServerPublicationHandle h, const Data& etag)
{
   mSecurity.removeUserCert(h->getPublisher());
}

void 
CertPublicationHandler::onRefresh(ServerPublicationHandle h, 
                                  const Data& etag, 
                                  const SipMessage& pub, 
                                  const Contents* contents,
                                  const SecurityAttributes* attrs,
                                  UInt32 expires)
{
   if (h->getDocumentKey() == h->getPublisher())
   {
      h->send(h->accept(200));
   }
   else
   {
      h->send(h->accept(403)); // !jf! is this the correct code? 
   }
}

void 
CertPublicationHandler::onUpdate(ServerPublicationHandle h, 
                                 const Data& etag, 
                                 const SipMessage& pub, 
                                 const Contents* contents,
                                 const SecurityAttributes* attrs,
                                 UInt32 expires)
{
   add(h, contents);
}

void 
CertPublicationHandler::onRemoved(ServerPublicationHandle h, const Data& etag, const SipMessage& pub, UInt32 expires)
{
   mSecurity.removeUserCert(h->getPublisher());
}

void 
CertPublicationHandler::add(ServerPublicationHandle h, const Contents* contents)
{
   if (h->getDocumentKey() == h->getPublisher())
   {
      const X509Contents* x509 = dynamic_cast<const X509Contents*>(contents);
      resip_assert(x509);
      mSecurity.addUserCertDER(h->getPublisher(), x509->getBodyData());
      h->send(h->accept(200));
   }
   else
   {
      h->send(h->accept(403)); // !jf! is this the correct code? 
   }
}

/* ====================================================================
*
* Copyright (c) 2015.  All rights reserved.
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
/*
* vi: set shiftwidth=3 expandtab:
*/

