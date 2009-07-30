#include "UserAgent.hxx"
#include "ReconSubsystem.hxx"
#include "UserAgentClientSubscription.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/ClientSubscription.hxx>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

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

SubscriptionHandle 
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
UserAgentClientSubscription::onTerminated(ClientSubscriptionHandle h, const SipMessage* msg)
{
   unsigned int statusCode = 0;
   if(msg)
   {
      InfoLog(<< "onTerminated(ClientSubscriptionHandle): handle=" << mSubscriptionHandle << ", " << msg->brief());
      if(msg->isResponse())
      {
         statusCode = msg->header(h_StatusLine).responseCode();
      }
      else
      {
         if(msg->getContents())
         {
            notifyReceived(msg->getContents()->getBodyData());
         }
      }
   }
   else
   {
      InfoLog(<< "onTerminated(ClientSubscriptionHandle): handle=" << mSubscriptionHandle);
      statusCode = 408;  // timedout waiting for notify after subscribe
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

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
