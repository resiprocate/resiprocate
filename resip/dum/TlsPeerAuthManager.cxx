#include <cassert>

#include "resip/dum/DumFeature.hxx"
#include "resip/dum/DumFeatureChain.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/TargetCommand.hxx"
#include "resip/dum/TlsPeerAuthManager.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

TlsPeerAuthManager::TlsPeerAuthManager(DialogUsageManager& dum, TargetCommand::Target& target, std::set<Data>& trustedPeers, bool thirdPartyRequiresCertificate) :
   DumFeature(dum, target),
   mTrustedPeers(trustedPeers),
   mThirdPartyRequiresCertificate(thirdPartyRequiresCertificate)
{
}


TlsPeerAuthManager::~TlsPeerAuthManager()
{
   InfoLog(<< "~TlsPeerAuthManager");
}

// !bwc! We absolutely, positively, MUST NOT throw here. This is because in
// DialogUsageManager::process(), we do not know if a DumFeature has taken
// ownership of msg until we get a return. If we throw, the ownership of msg
// is unknown. This is unacceptable.
DumFeature::ProcessingResult 
TlsPeerAuthManager::process(Message* msg)
{
   SipMessage* sipMessage = dynamic_cast<SipMessage*>(msg);

   if (sipMessage)
   {
      //!dcm! -- unecessary happens in handle
      switch ( handle(sipMessage) )
      {
         case TlsPeerAuthManager::Rejected:
            InfoLog(<< "TlsPeerAuth rejected request " << sipMessage->brief());
            return DumFeature::ChainDoneAndEventDone;            
         default:   // includes Authorized, Skipped
            return DumFeature::FeatureDone;            
      }
   }

   // Catch-all (handles something that was not a SipMessage)
   return FeatureDone;   
}

bool
TlsPeerAuthManager::authorizedForThisIdentity(
   const std::list<resip::Data> &peerNames,
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
      if(mTrustedPeers.find(i) != mTrustedPeers.end())
      {
         DebugLog(<< "Matched certificate name " << i << " is a trusted peer, not checking against From URI");
         return true;
      }
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

// return true if request has been consumed 
TlsPeerAuthManager::Result
TlsPeerAuthManager::handle(SipMessage* sipMessage)
{
   //InfoLog( << "trying to do auth" );
   if (!sipMessage->isRequest() ||
       sipMessage->header(h_RequestLine).method() == ACK ||
       sipMessage->header(h_RequestLine).method() == CANCEL)
   {
      // Do not inspect ACKs or CANCELs
      return Skipped;
   }

   if(!sipMessage->header(h_From).isWellFormed() ||
      sipMessage->header(h_From).isAllContacts() )
   {
      InfoLog(<<"Malformed From header: cannot verify against any certificate. Rejecting.");
      SharedPtr<SipMessage> response(new SipMessage);
      Helper::makeResponse(*response, *sipMessage, 400, "Malformed From header");
      mDum.send(response);
      return Rejected;
   }

   // We are only concerned with connections over TLS
   if(!sipMessage->isExternal() || sipMessage->getSource().getType() != TLS)
   {
      DebugLog(<<"Can't validate certificate on non-TLS connection");
      return Skipped;
   }

   const std::list<resip::Data> &peerNames = sipMessage->getTlsPeerNames();
   if (mDum.isMyDomain(sipMessage->header(h_From).uri().host()))
   {
      if (requiresAuthorization(*sipMessage))
      {
         if(authorizedForThisIdentity(peerNames, sipMessage->header(h_From).uri()))
            return Authorized;
         SharedPtr<SipMessage> response(new SipMessage);
         Helper::makeResponse(*response, *sipMessage, 403, "Authorization Failed for peer cert");
         mDum.send(response);
         return Rejected;
      }
      else
         return Skipped;
   }
   else
   {
      if(mThirdPartyRequiresCertificate && peerNames.size() == 0)
      {
         SharedPtr<SipMessage> response(new SipMessage);
         Helper::makeResponse(*response, *sipMessage, 403, "Mutual TLS required to handle that message");
         mDum.send(response);
         return Rejected;
      }
      if(authorizedForThisIdentity(peerNames, sipMessage->header(h_From).uri()))
         return Authorized;
      SharedPtr<SipMessage> response(new SipMessage);
      Helper::makeResponse(*response, *sipMessage, 403, "Authorization Failed for peer cert");
      mDum.send(response);
      return Rejected;
   }

   InfoLog(<< "Skipping some message that we didn't explicitly handle");
   return Skipped;
}

bool
TlsPeerAuthManager::requiresAuthorization(const SipMessage& msg)
{
   // everything must be authorized, over-ride this method
   // to implement some other policy
   return true;
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
