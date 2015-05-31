#include "resip/stack/ssl/Security.hxx"
#include "resip/stack/Pkcs8Contents.hxx"
#include "resip/stack/X509Contents.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "repro/stateAgents/CertServer.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace repro;

CertServer::CertServer(DialogUsageManager& dum) : 
   mDum(dum),
   mPrivateKeyServer(*mDum.getSecurity()),
   mPrivateKeyUpdater(*mDum.getSecurity()),
   mCertServer(*mDum.getSecurity()),
   mCertUpdater(*mDum.getSecurity())
{
   MasterProfile& profile = *mDum.getMasterProfile();
   profile.addSupportedMethod(PUBLISH);
   profile.addSupportedMethod(SUBSCRIBE);
   //profile.validateAcceptEnabled() = true;  // !slg! this causes Accept validation for registration requests as well, which is not really desired
   profile.validateContentEnabled() = true;
   profile.addSupportedMimeType(PUBLISH, Pkcs8Contents::getStaticType());
   profile.addSupportedMimeType(SUBSCRIBE, Pkcs8Contents::getStaticType());
   profile.addSupportedMimeType(PUBLISH, X509Contents::getStaticType());
   profile.addSupportedMimeType(SUBSCRIBE, X509Contents::getStaticType());
         
   mDum.addServerSubscriptionHandler(Symbols::Credential, &mPrivateKeyServer);
   mDum.addServerSubscriptionHandler(Symbols::Certificate, &mCertServer);
   mDum.addServerPublicationHandler(Symbols::Credential, &mPrivateKeyUpdater);
   mDum.addServerPublicationHandler(Symbols::Certificate, &mCertUpdater);
}

CertServer::~CertServer()
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

