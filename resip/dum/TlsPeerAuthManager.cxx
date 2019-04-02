#include "rutil/ResipAssert.h"

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

TlsPeerAuthManager::TlsPeerAuthManager(DialogUsageManager& dum, TargetCommand::Target& target, const std::set<Data>& trustedPeers, bool thirdPartyRequiresCertificate) :
   DumFeature(dum, target),
   mTrustedPeers(trustedPeers),
   mThirdPartyRequiresCertificate(thirdPartyRequiresCertificate)
{
}

TlsPeerAuthManager::TlsPeerAuthManager(DialogUsageManager& dum, TargetCommand::Target& target, const std::set<Data>& trustedPeers, bool thirdPartyRequiresCertificate, CommonNameMappings& commonNameMappings) :
   DumFeature(dum, target),
   mTrustedPeers(trustedPeers),
   mThirdPartyRequiresCertificate(thirdPartyRequiresCertificate),
   mCommonNameMappings(commonNameMappings)
{
}

TlsPeerAuthManager::~TlsPeerAuthManager()
{
   InfoLog(<< "~TlsPeerAuthManager:  " << mMessages.size() << " messages in memory when destroying.");
}

// !bwc! We absolutely, positively, MUST NOT throw here. This is because in
// DialogUsageManager::process(), we do not know if a DumFeature has taken
// ownership of msg until we get a return. If we throw, the ownership of msg
// is unknown. This is unacceptable.
DumFeature::ProcessingResult 
TlsPeerAuthManager::process(Message* msg)
{
   SipMessage* sipMessage = dynamic_cast<SipMessage*>(msg);
   TlsPeerIdentityInfoMessage* tpiMessage = dynamic_cast<TlsPeerIdentityInfoMessage*>(msg);

   if (sipMessage)
   {
      //!dcm! -- unecessary happens in handle
      switch ( handle(sipMessage) )
      {
         case TlsPeerAuthManager::Rejected:
            InfoLog(<< "TlsPeerAuth rejected request " << sipMessage->brief());
            return DumFeature::ChainDoneAndEventDone;            
         case TlsPeerAuthManager::RequestedInfo:
            return DumFeature::EventTaken;
         default:   // includes Authorized, Skipped
            return DumFeature::FeatureDone;            
      }
   }

   if (tpiMessage)
   {
      Message* result = handleTlsPeerIdentityInfo(tpiMessage);
      if (result)
      {
         postCommand(auto_ptr<Message>(result));
         return FeatureDoneAndEventDone;
      }
      else
      {
         InfoLog(<< "TlsPeerAuth rejected request " << *tpiMessage);
         return ChainDoneAndEventDone;
      }
   }

   // Catch-all (handles something that was not a SipMessage)
   return FeatureDone;   
}

AsyncBool
TlsPeerAuthManager::authorizedForThisIdentity(
   const Data& transactionId,
   const std::list<resip::Data> &peerNames,
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
         return True;
      }
      if(i == domain)
      {
         DebugLog(<< "Matched certificate name " << i << " against domain " << domain);
         return True;
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
            return True;
         }
         if(permitted.find(domain) != permitted.end())
         {
            DebugLog(<< "Matched certificate name " << i << " against domain " << domain << " by common name mappings");
            return True;
         }
      }
      DebugLog(<< "Certificate name " << i << " doesn't match AoR " << aor << " or domain " << domain);
   }

   if(mCommonNameMappings.size() == 0)
   {
      DebugLog(<<"mCommonNameMappings is empty, trying async");
      TlsPeerIdentityInfoMessage* tpaInfo = new TlsPeerIdentityInfoMessage(transactionId, &mDum);
      for(it = peerNames.begin(); it != peerNames.end(); ++it)
      {
         tpaInfo->peerNames().insert(*it);
      }
      tpaInfo->identities().insert(aor);
      tpaInfo->identities().insert(domain);
      return asyncLookup(tpaInfo);
   }

   // catch-all: access denied
   DebugLog(<< "message content didn't match any peer name");
   return False;
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
   Uri claimedUri = sipMessage->header(h_From).uri();
   if(sipMessage->method() == REFER && sipMessage->exists(h_ReferredBy))
   {
      if(!sipMessage->header(h_ReferredBy).isWellFormed() ||
         sipMessage->header(h_ReferredBy).isAllContacts() )
      {
         InfoLog(<<"Malformed Referred-By header: cannot verify against any certificate. Rejecting.");
         SharedPtr<SipMessage> response(new SipMessage);
         Helper::makeResponse(*response, *sipMessage, 400, "Malformed Referred-By header");
         mDum.send(response);
         return Rejected;
      }
      // For REFER requests, we authenticate the Referred-By header
      // instead of the From header
      claimedUri = sipMessage->header(h_ReferredBy).uri();
   }

   // We are only concerned with connections over TLS
   if(!sipMessage->isExternal() || !isSecure(sipMessage->getSource().getType()))
   {
      DebugLog(<<"Can't validate certificate on non-TLS connection");
      return Skipped;
   }

   if(isTrustedSource(*sipMessage))
   {
      DebugLog(<<"from trusted node, skipping checks");
      return Authorized;
   }

   const std::list<resip::Data> &peerNames = sipMessage->getTlsPeerNames();
   if (mDum.isMyDomain(claimedUri.host()))
   {
      // peerNames is empty if client certificate mode is `optional'
      // or if the message didn't come in on TLS transport
      if (!requiresAuthorization(*sipMessage))
      {
         DebugLog(<<"authorization not required for this message");
         return Skipped;
      }

      if(peerNames.empty())
      {
         DebugLog(<<"peerNames is empty, allowing the message without further inspection");
         return Skipped;
      }

      AsyncBool _auth = authorizedForThisIdentity(sipMessage->getTransactionId(), peerNames, claimedUri);
      if(_auth == True)
      {
         DebugLog(<<"authorized");
         return Authorized;
      }
      else if(_auth == Async)
      {
         mMessages[sipMessage->getTransactionId()] = sipMessage;
         DebugLog(<<"waiting for async authorization");
         return RequestedInfo;
      }
      DebugLog(<<"not authorized");
      SharedPtr<SipMessage> response(new SipMessage);
      Helper::makeResponse(*response, *sipMessage, 403, "Authorization Failed for peer cert");
      mDum.send(response);
      return Rejected;
   }
   else
   {
      // peerNames is empty if client certificate mode is `optional'
      // or if the message didn't come in on TLS transport
      if(peerNames.empty())
      {
         if(mThirdPartyRequiresCertificate)
         {
            DebugLog(<<"third party requires certificate");
            SharedPtr<SipMessage> response(new SipMessage);
            Helper::makeResponse(*response, *sipMessage, 403, "Mutual TLS required to handle that message");
            mDum.send(response);
            return Rejected;
         }
         else
         {
            DebugLog(<<"third party does not require certificate, allowing the message without further inspection");
            return Skipped;
         }
      }
      AsyncBool _auth = authorizedForThisIdentity(sipMessage->getTransactionId(), peerNames, claimedUri);
      if(_auth == True)
      {
         DebugLog(<<"authorized");
         return Authorized;
      }
      else if(_auth == Async)
      {
         mMessages[sipMessage->getTransactionId()] = sipMessage;
         DebugLog(<<"waiting for async authorization");
         return RequestedInfo;
      }
      DebugLog(<<"not authorized");
      SharedPtr<SipMessage> response(new SipMessage);
      Helper::makeResponse(*response, *sipMessage, 403, "Authorization Failed for peer cert");
      mDum.send(response);
      return Rejected;
   }

   InfoLog(<< "Skipping some message that we didn't explicitly handle");
   return Skipped;
}

SipMessage*
TlsPeerAuthManager::handleTlsPeerIdentityInfo(TlsPeerIdentityInfoMessage *tpiInfo)
{
   resip_assert(tpiInfo);

   MessageMap::iterator it = mMessages.find(tpiInfo->getTransactionId());
   resip_assert(it != mMessages.end());
   SipMessage* request = it->second;
   mMessages.erase(it);

   if(tpiInfo->authorized())
   {
      DebugLog(<<"authorized");
      return request;
   }

   DebugLog(<<"not authorized");
   SharedPtr<SipMessage> response(new SipMessage);
   Helper::makeResponse(*response, *request, 403, "Authentication Failed for peer cert.");
   mDum.send(response);
   delete request;
   return 0;
}

AsyncBool
TlsPeerAuthManager::asyncLookup(TlsPeerIdentityInfoMessage *info)
{
   delete info;
   return False;
}

bool
TlsPeerAuthManager::requiresAuthorization(const SipMessage& msg)
{
   // everything must be authorized, over-ride this method
   // to implement some other policy
   return true;
}

bool
TlsPeerAuthManager::isTrustedSource(const SipMessage& msg)
{
   // over-ride this method to implement some other policy

   const std::list<resip::Data> &peerNames = msg.getTlsPeerNames();
   std::list<Data>::const_iterator it = peerNames.begin();
   for(; it != peerNames.end(); ++it)
   {
      const Data& i = *it;
      if(mTrustedPeers.find(i) != mTrustedPeers.end())
      {
         DebugLog(<< "Matched certificate name " << i << " is a trusted peer");
         return true;
      }
   }

   return false;
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
