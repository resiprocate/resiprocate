#include "UserAgent.hxx"
#include "UserAgentSubsystem.hxx"
#include "UserAgentClientSubscription.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/ClientSubscription.hxx>

using namespace useragent;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM UserAgentSubsystem::USERAGENT

UserAgentClientSubscription::UserAgentClientSubscription(UserAgent& userAgent, DialogUsageManager& dum, unsigned int handle)
: AppDialogSet(dum),
  mUserAgent(userAgent),
  mDum(dum),
  mSubscriptionHandle(handle),
  mLastNotifyHash(0),
  mEnded(false)
{
   mUserAgent.registerSubscription(this);
}

UserAgentClientSubscription::~UserAgentClientSubscription()
{
   mUserAgent.unregisterSubscription(this);
}

UserAgent::SubscriptionHandle 
UserAgentClientSubscription::getSubscriptionHandle()
{
   return mSubscriptionHandle;
}

void 
UserAgentClientSubscription::end()
{
   if(!mEnded)
   {
      mEnded = true;
      AppDialogSet::end();
   }
}

void
UserAgentClientSubscription::notifyReceived(const Data& notifyData)
{
   size_t hash = notifyData.hash();
   if(hash != mLastNotifyHash)  // only call callback if body changed from last time
   {
      mLastNotifyHash = hash;
      mUserAgent.onSubscriptionNotify(mSubscriptionHandle, notifyData);
   }
}

////////////////////////////////////////////////////////////////////////////////
// ClientSubscriptionHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
UserAgentClientSubscription::onUpdatePending(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   InfoLog(<< "onUpdatePending(ClientSubscriptionHandle): handle=" << mSubscriptionHandle << ", " << msg.brief());
   h->acceptUpdate();
   if(mEnded)
   {
      h->end();
   }
   else if(msg.getContents())
   {
      const Data& bodyData = msg.getContents()->getBodyData();
      notifyReceived(bodyData);
   }
}

void
UserAgentClientSubscription::onUpdateActive(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   InfoLog(<< "onUpdateActive(ClientSubscriptionHandle): handle=" << mSubscriptionHandle << ", " << msg.brief());
   h->acceptUpdate();
   if(mEnded)
   {
      h->end();
   }
   else if(msg.getContents())
   {
      const Data& bodyData = msg.getContents()->getBodyData();
      notifyReceived(bodyData);
   }
}

void
UserAgentClientSubscription::onUpdateExtension(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   InfoLog(<< "onUpdateExtension(ClientSubscriptionHandle): handle=" << mSubscriptionHandle << ", " << msg.brief());
   h->acceptUpdate();
   if(mEnded)
   {
      h->end();
   }
   else if(msg.getContents())
   {
      const Data& bodyData = msg.getContents()->getBodyData();
      notifyReceived(bodyData);
   }
}

void
UserAgentClientSubscription::onTerminated(ClientSubscriptionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onTerminated(ClientSubscriptionHandle): handle=" << mSubscriptionHandle << ", " << msg.brief());
   unsigned int statusCode = 0;
   if(msg.isResponse())
   {
      statusCode = msg.header(h_StatusLine).responseCode();
   }
   else
   {
      if(msg.getContents())
      {
         notifyReceived(msg.getContents()->getBodyData());
      }
   }
   mUserAgent.onSubscriptionTerminated(mSubscriptionHandle, statusCode);
}

void
UserAgentClientSubscription::onNewSubscription(ClientSubscriptionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onNewSubscription(ClientSubscriptionHandle): handle=" << mSubscriptionHandle << ", " << msg.brief());
   // Note:  The notify here, will also be passed in an onUpdateXXXX callback, so no need to do anything with this callback
}

int 
UserAgentClientSubscription::onRequestRetry(ClientSubscriptionHandle h, int retryMinimum, const SipMessage& msg)
{
   return min(retryMinimum, mUserAgent.getUserAgentMasterProfile()->subscriptionRetryInterval());
}


/* ====================================================================

 Original contribution Copyright (C) 2008 Plantronics, Inc.
 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */
