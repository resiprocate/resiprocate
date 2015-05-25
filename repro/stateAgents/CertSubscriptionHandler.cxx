#include "resip/stack/ssl/Security.hxx"
#include "resip/stack/X509Contents.hxx"
#include "repro/stateAgents/CertSubscriptionHandler.hxx"

using namespace repro;
using namespace resip;

CertSubscriptionHandler::CertSubscriptionHandler(Security& security) : mSecurity(security)
{
}

void 
CertSubscriptionHandler::onNewSubscription(ServerSubscriptionHandle h, const SipMessage& sub)
{
   if (!mSecurity.hasUserCert(h->getDocumentKey()))
   {
      // !jf! really need to do this async. send neutral state in the meantime,
      // blah blah blah
      mSecurity.generateUserCert(h->getDocumentKey());
   }

   if (mSecurity.hasUserCert(h->getDocumentKey()))
   {
      h->setSubscriptionState(Active);
      h->send(h->accept(200));

      X509Contents x509(mSecurity.getUserCertDER(h->getDocumentKey()));
      h->send(h->update(&x509));
   }
   else
   {
      h->send(h->reject(404));
   }
}

void 
CertSubscriptionHandler::onPublished(ServerSubscriptionHandle associated, 
                                     ServerPublicationHandle publication, 
                                     const Contents* contents,
                                     const SecurityAttributes* attrs)
{
   associated->send(associated->update(contents));
}


void 
CertSubscriptionHandler::onTerminated(ServerSubscriptionHandle)
{
}

void 
CertSubscriptionHandler::onError(ServerSubscriptionHandle, const SipMessage& msg)
{
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

