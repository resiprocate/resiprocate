#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/ServerPublication.hxx"
#include "resip/dum/PublicationHandler.hxx"
#include "resip/dum/ServerSubscription.hxx"
#include "resip/dum/SubscriptionHandler.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/SecurityAttributes.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

ServerPublication::ServerPublication(DialogUsageManager& dum,  
                                     const Data& etag,
                                     const SipMessage& msg)
   : BaseUsage(dum),
     mLastResponse(new SipMessage), 
     mEtag(etag),
     mEventType(msg.header(h_Event).value()),
     mTimerSeq(0)
{
}

ServerPublication::~ServerPublication()
{
   mDum.mServerPublications.erase(getEtag());
}

ServerPublicationHandle 
ServerPublication::getHandle()
{
   return ServerPublicationHandle(mDum, getBaseHandle().getId());
}

const Data&
ServerPublication::getEtag() const
{
   return mEtag;
}

const Data&
ServerPublication::getDocumentKey() const
{
   return mDocumentKey;
}

const Data& 
ServerPublication::getPublisher() const
{
   return mLastRequest.header(h_From).uri().getAor();
}

void
ServerPublication::updateMatchingSubscriptions()
{
   Data key = mEventType + mLastRequest.header(h_RequestLine).uri().getAor();
   std::pair<DialogUsageManager::ServerSubscriptions::iterator,DialogUsageManager::ServerSubscriptions::iterator> subs;
   subs = mDum.mServerSubscriptions.equal_range(key);
   
   ServerSubscriptionHandler* handler = mDum.getServerSubscriptionHandler(mEventType);
   for (DialogUsageManager::ServerSubscriptions::iterator i=subs.first; i!=subs.second; ++i)
   {
      handler->onPublished(i->second->getHandle(), 
                           getHandle(), 
                           mLastBody.mContents.get(), 
                           mLastBody.mAttributes.get());
   }
   mLastBody.mContents.reset();
   mLastBody.mAttributes.reset();
}


SharedPtr<SipMessage>
ServerPublication::accept(int statusCode)
{
   Helper::makeResponse(*mLastResponse, mLastRequest, statusCode);
   mLastResponse->header(h_Expires).value() = mExpires;

   updateMatchingSubscriptions();

   return mLastResponse;   
}

SharedPtr<SipMessage>
ServerPublication::reject(int statusCode)
{
   Helper::makeResponse(*mLastResponse, mLastRequest, statusCode);
   mLastResponse->header(h_Expires).value() = mExpires;
   return mLastResponse;  
}

void 
ServerPublication::end()
{
   delete this;
}

void 
ServerPublication::dispatch(const SipMessage& msg)
{
   assert(msg.isRequest());
   ServerPublicationHandler* handler = mDum.getServerPublicationHandler(mEventType);
   mLastRequest = msg;
   mExpires = 3600; //bad
   if (msg.exists(h_Expires))
   {
      mExpires = msg.header(h_Expires).value();
   }
   if (msg.exists(h_SIPIfMatch))
   {      
      if (mExpires == 0)
      {
         handler->onRemoved(getHandle(), mEtag, msg, mExpires);
         Helper::makeResponse(*mLastResponse, mLastRequest, 200);
         mLastResponse->header(h_Expires).value() = mExpires;
         mDum.send(mLastResponse);
         delete this;
         return;
      }
      mLastBody = Helper::extractFromPkcs7(msg, *mDum.getSecurity());
      if (msg.getContents())
      {
         handler->onUpdate(getHandle(), mEtag, msg, 
                           mLastBody.mContents.get(), 
                           mLastBody.mAttributes.get(), 
                           mExpires);
      }
      else
      {
         handler->onRefresh(getHandle(), mEtag, msg, 
                            mLastBody.mContents.get(), 
                            mLastBody.mAttributes.get(), 
                            mExpires);
      }
   }
   else
   {
      mLastBody = Helper::extractFromPkcs7(msg, *mDum.getSecurity());
      handler->onInitial(getHandle(), mEtag, msg, 
                         mLastBody.mContents.get(), 
                         mLastBody.mAttributes.get(), 
                         mExpires);
   }
}

void
ServerPublication::dispatch(const DumTimeout& msg)
{
   if (msg.seq() == mTimerSeq)
   {
      ServerPublicationHandler* handler = mDum.getServerPublicationHandler(mEventType);
      handler->onExpired(getHandle(), mEtag);
      delete this;
   }
}

void 
ServerPublication::send(SharedPtr<SipMessage> response)
{
   assert(response->isResponse());
   response->header(h_SIPETag).value() = mEtag;
   mDum.send(response);
   if (response->header(h_StatusLine).statusCode() >= 300)
   {
      delete this;
   }
   else
   {
      mDum.addTimer(DumTimeout::Publication, response->header(h_Expires).value(), getBaseHandle(), ++mTimerSeq);
   }
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
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
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
