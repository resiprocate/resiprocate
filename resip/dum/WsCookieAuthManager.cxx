#include "rutil/ResipAssert.h"

#include "resip/dum/DumFeature.hxx"
#include "resip/dum/DumFeatureChain.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/TargetCommand.hxx"
#include "resip/dum/WsCookieAuthManager.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

WsCookieAuthManager::WsCookieAuthManager(DialogUsageManager& dum, TargetCommand::Target& target) :
   DumFeature(dum, target)
{
}

WsCookieAuthManager::~WsCookieAuthManager()
{
   InfoLog(<< "~WsCookieAuthManager");
}

// !bwc! We absolutely, positively, MUST NOT throw here. This is because in
// DialogUsageManager::process(), we do not know if a DumFeature has taken
// ownership of msg until we get a return. If we throw, the ownership of msg
// is unknown. This is unacceptable.
DumFeature::ProcessingResult
WsCookieAuthManager::process(Message* msg)
{
   SipMessage* sipMessage = dynamic_cast<SipMessage*>(msg);

   if (sipMessage)
   {
      //!dcm! -- unecessary happens in handle
      switch ( handle(sipMessage) )
      {
         case WsCookieAuthManager::Rejected:
            InfoLog(<< "WsCookieAuth rejected request " << sipMessage->brief());
            return DumFeature::ChainDoneAndEventDone;
         default:   // includes Authorized, Skipped
            return DumFeature::FeatureDone;
      }
   }

   // Catch-all (handles something that was not a SipMessage)
   return FeatureDone;
}

bool
WsCookieAuthManager::cookieUriMatch(const resip::Uri &first, const resip::Uri &second)
{
   return(
      (isEqualNoCase(first.user(), second.user()) || first.user() == "*") &&
      (isEqualNoCase(first.host(), second.host()) || first.host() == "*")
         );
}

bool
WsCookieAuthManager::authorizedForThisIdentity(
   const MethodTypes method,
   const WsCookieContext &wsCookieContext,
   const resip::Uri &fromUri,
   const resip::Uri &toUri)
{
   if(difftime(wsCookieContext.getExpiresTime(), time(NULL)) < 0)
   {
      WarningLog(<< "Received expired cookie");
      return false;
   }

   Uri wsFromUri = wsCookieContext.getWsFromUri();
   Uri wsDestUri = wsCookieContext.getWsDestUri();
   if(cookieUriMatch(wsFromUri, fromUri))
   {
      DebugLog(<< "Matched cookie source URI field" << wsFromUri << " against request From header field URI " << fromUri);
      // When registering, "From" URI == "To" URI, so we can ignore the
      // "To" URI restriction from the cookie when processing REGISTER
      if(method == REGISTER && isEqualNoCase(fromUri.user(), toUri.user()) && isEqualNoCase(fromUri.host(), toUri.host()))
      {
         return true;
      }
      if(cookieUriMatch(wsDestUri, toUri))
      {
          DebugLog(<< "Matched cookie destination URI field" << wsDestUri << " against request To header field URI " << toUri);
          return true;
      }
   }

   // catch-all: access denied
   return false;
}

// return true if request has been consumed
WsCookieAuthManager::Result
WsCookieAuthManager::handle(SipMessage* sipMessage)
{
   // Only check message coming over WebSockets
   if(!isWebSocket(sipMessage->getReceivedTransportTuple().getType()))
   {
      return Skipped;
   }
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
      InfoLog(<<"Malformed From header: cannot verify against cookie. Rejecting.");
      SharedPtr<SipMessage> response(new SipMessage);
      Helper::makeResponse(*response, *sipMessage, 400, "Malformed From header");
      mDum.send(response);
      return Rejected;
   }

   const WsCookieContext &wsCookieContext = *(sipMessage->getWsCookieContext());
   if (mDum.isMyDomain(sipMessage->header(h_From).uri().host()))
   {
      if (requiresAuthorization(*sipMessage))
      {
         if(authorizedForThisIdentity(sipMessage->header(h_RequestLine).method(), wsCookieContext, sipMessage->header(h_From).uri(), sipMessage->header(h_To).uri()))
         {
            return Authorized;
         }
         SharedPtr<SipMessage> response(new SipMessage);
         Helper::makeResponse(*response, *sipMessage, 403, "Cookie-based authorization failed");
         mDum.send(response);
         return Rejected;
      }
      else
      {
         return Skipped;
      }
   }
   else
   {
      SharedPtr<SipMessage> response(new SipMessage);
      Helper::makeResponse(*response, *sipMessage, 403, "Cookie-based authorization failed");
      mDum.send(response);
      return Rejected;
   }

   InfoLog(<< "Skipping some message that we didn't explicitly handle");
   return Skipped;
}

bool
WsCookieAuthManager::requiresAuthorization(const SipMessage& msg)
{
   // everything must be authorized, over-ride this method
   // to implement some other policy
   return true;
}


/* ====================================================================
 * BSD License
 *
 * Copyright (c) 2013 Catalin Constantin Usurelu  All rights reserved.
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
