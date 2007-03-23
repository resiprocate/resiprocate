#include <queue>

#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/SipFrag.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/dum/ClientSubscription.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/SubscriptionHandler.hxx"
#include "resip/dum/SubscriptionCreator.hxx"
#include "resip/dum/UsageUseException.hxx"

#include "resip/dum/AppDialogSet.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM


ClientSubscription::ClientSubscription(DialogUsageManager& dum, Dialog& dialog,
                                       const SipMessage& request, UInt32 defaultSubExpiration)
   : BaseSubscription(dum, dialog, request),
     mOnNewSubscriptionCalled(mEventType == "refer"),  // don't call onNewSubscription for Refer subscriptions
     mEnded(false),
     mExpires(0),
     mDefaultExpires(defaultSubExpiration),
     mRefreshing(false),
     mHaveQueuedRefresh(false),
     mQueuedRefreshInterval(-1),
     mLargestNotifyCSeq(0)
{
   DebugLog (<< "ClientSubscription::ClientSubscription from " << request.brief());   
   mDialog.makeRequest(*mLastRequest, SUBSCRIBE);
}

ClientSubscription::~ClientSubscription()
{
   mDialog.mClientSubscriptions.remove(this);

   while (!mQueuedNotifies.empty())
   {
      delete mQueuedNotifies.front();
      mQueuedNotifies.pop_front();
   }

   clearDustbin();
}

ClientSubscriptionHandle 
ClientSubscription::getHandle()
{
   return ClientSubscriptionHandle(mDum, getBaseHandle().getId());
}

void
ClientSubscription::dispatch(const SipMessage& msg)
{
   DebugLog (<< "ClientSubscription::dispatch " << msg.brief());
   
   ClientSubscriptionHandler* handler = mDum.getClientSubscriptionHandler(mEventType);
   assert(handler);

   clearDustbin();

   // asserts are checks the correctness of Dialog::dispatch
   if (msg.isRequest() )
   {
      assert( msg.header(h_RequestLine).getMethod() == NOTIFY );
      mRefreshing = false;


      // !dlb! 481 NOTIFY iff state is dead?

      //!dcm! -- heavy, should just store enough information to make response
      //mLastNotify = msg;

      if (!mOnNewSubscriptionCalled && !getAppDialogSet()->isReUsed())
      {
         InfoLog (<< "[ClientSubscription] " << mLastRequest->header(h_To));
         if (msg.exists(h_Contacts))
         {
            mDialog.mRemoteTarget = msg.header(h_Contacts).front();
         }
         
         handler->onNewSubscription(getHandle(), msg);
         mOnNewSubscriptionCalled = true;
      }         

      bool outOfOrder = mLargestNotifyCSeq > msg.header(h_CSeq).sequence();
      if (!outOfOrder)
      {
         mLargestNotifyCSeq = msg.header(h_CSeq).sequence();
      }
      else
      {
         DebugLog(<< "received out of order notify");
      }

      mQueuedNotifies.push_back(new QueuedNotify(msg, outOfOrder));
      if (mQueuedNotifies.size() == 1)
      {
         DebugLog(<< "no queued notify");
         processNextNotify();
         return;
      }
      else
      {
         DebugLog(<< "Notify gets queued");
      }
   }
   else
   {
      DebugLog(<< "processing client subscription response");
      processResponse(msg);
   }
}

void 
ClientSubscription::processResponse(const SipMessage& msg)
{
   ClientSubscriptionHandler* handler = mDum.getClientSubscriptionHandler(mEventType);
   assert(handler);

   mRefreshing = false;

   if (msg.header(h_StatusLine).statusCode() >= 200 && msg.header(h_StatusLine).statusCode() <300)
   {
      if (msg.exists(h_Expires))
      {
         // grab the expires from the 2xx in case there is not one on the NOTIFY .mjf.
         UInt32 expires = msg.header(h_Expires).value();
         UInt32 lastExpires = mLastRequest->header(h_Expires).value();
         if (expires < lastExpires)
         {
            mLastRequest->header(h_Expires).value() = expires;
         }
      }
      sendQueuedRefreshRequest();
   }
   else if (msg.header(h_StatusLine).statusCode() == 481 &&
            msg.exists(h_Expires) && msg.header(h_Expires).value() > 0)
   {
      InfoLog (<< "Received 481 to SUBSCRIBE, reSUBSCRIBEing (presence server probably restarted) "
               << mDialog.mRemoteTarget);

      SharedPtr<SipMessage> sub = mDum.makeSubscription(mDialog.mRemoteTarget, getEventType(), getAppDialogSet()->reuse());
      mDum.send(sub);

      delete this;
      return;
   }
   else if (msg.header(h_StatusLine).statusCode() == 408 ||
            ((msg.header(h_StatusLine).statusCode() == 413 ||
              msg.header(h_StatusLine).statusCode() == 480 ||
              msg.header(h_StatusLine).statusCode() == 486 ||
              msg.header(h_StatusLine).statusCode() == 500 ||
              msg.header(h_StatusLine).statusCode() == 503 ||
              msg.header(h_StatusLine).statusCode() == 600 ||
              msg.header(h_StatusLine).statusCode() == 603) &&
             msg.exists(h_RetryAfter)))
   {
      int retry;

      if (msg.header(h_StatusLine).statusCode() == 408)
      {
         InfoLog (<< "Received 408 to SUBSCRIBE "
                  << mLastRequest->header(h_To));
         retry = handler->onRequestRetry(getHandle(), 0, msg);
      }
      else
      {
         InfoLog (<< "Received non-408 retriable to SUBSCRIBE "
                  << mLastRequest->header(h_To));
         retry = handler->onRequestRetry(getHandle(), msg.header(h_RetryAfter).value(), msg);
      }

      if (retry < 0)
      {
         DebugLog(<< "Application requested failure on Retry-After");
         mEnded = true;
         handler->onTerminated(getHandle(), msg);
         delete this;
         return;
      }
      else if (retry == 0)
      {
         DebugLog(<< "Application requested immediate retry on Retry-After");
         //!dcm! -- why isn't this just a refresh--retry after might be
         //middle element and not indicate dialog destruction       
         if (mDialog.mRemoteTarget.uri().host().empty())
         {
            SharedPtr<SipMessage> sub = mDum.makeSubscription(mLastRequest->header(h_To), getEventType());
            mDum.send(sub);
         }
         else
         {
            SharedPtr<SipMessage> sub = mDum.makeSubscription(mDialog.mRemoteTarget, getEventType(), getAppDialogSet()->reuse());
            mDum.send(sub);
         }
         //!dcm! -- new sub created above, when does this usage get destroyed?
         //return;
      }
      else 
      {
         // leave the usage around until the timeout
         // !dlb! would be nice to set the state to something dead, but not used
         mDum.addTimer(DumTimeout::SubscriptionRetry, 
                       retry, 
                       getBaseHandle(),
                       ++mTimerSeq);
         // leave the usage around until the timeout
         return;
      }
            
      delete this;
      return;
   }
   else if (msg.header(h_StatusLine).statusCode() >= 300)
   {
      if (msg.header(h_StatusLine).statusCode() == 423 
          && msg.exists(h_MinExpires))
      {
         requestRefresh(msg.header(h_MinExpires).value());            
      }
      else
      {
         mEnded = true;
         handler->onTerminated(getHandle(), msg);
         delete this;
         return;
      }
   }
}

void 
ClientSubscription::processNextNotify()
{
   assert(!mQueuedNotifies.empty());

   QueuedNotify* qn = mQueuedNotifies.front();
   ClientSubscriptionHandler* handler = mDum.getClientSubscriptionHandler(mEventType);
   assert(handler);

   unsigned long refreshInterval = 0;
   if (!qn->outOfOrder())
   {
      UInt32 expires = 0;
      //default to 3600 seconds so non-compliant endpoints don't result in leaked usages
      if (qn->notify().exists(h_SubscriptionState) && qn->notify().header(h_SubscriptionState).exists(p_expires))
      {
         expires = qn->notify().header(h_SubscriptionState).param(p_expires);
      }
      else if (mLastRequest->exists(h_Expires))
      {
         expires = mLastRequest->header(h_Expires).value();
      }
      else if (mDefaultExpires)
      {
         /* if we haven't gotten an expires value from:
            1. the subscription state from this notify
            2. the last request
            then use the default expires (meaning it came from the 2xx in response
            to the initial SUBSCRIBE). .mjf.
          */
         expires = mDefaultExpires;
      }
      else
      {
         expires = 3600;
      }
      
      if (!mLastRequest->exists(h_Expires))
      {
         DebugLog(<< "No expires header in last request, set to " << expires);
         mLastRequest->header(h_Expires).value() = expires;
      }
      UInt64 now = Timer::getTimeMs() / 1000;
      
      if (mExpires == 0 || now + expires < mExpires)
      {
         refreshInterval = Helper::aBitSmallerThan((unsigned long)expires);
         mExpires = now + refreshInterval;
      }
   }
   //if no subscription state header, treat as an extension. Only allow for
   //refer to handle non-compliant implementations
   if (!qn->notify().exists(h_SubscriptionState))
   {
      if (qn->notify().exists(h_Event) && qn->notify().header(h_Event).value() == "refer")
      {
         SipFrag* frag  = dynamic_cast<SipFrag*>(qn->notify().getContents());
         if (frag)
         {
            if (frag->message().isResponse())
            {
               int code = frag->message().header(h_StatusLine).statusCode();
               if (code < 200)
               {
                  handler->onUpdateExtension(getHandle(), qn->notify(), qn->outOfOrder());
               }
               else
               {
                  acceptUpdate();
                  mEnded = true;                     
                  handler->onTerminated(getHandle(), qn->notify());
                  delete this;
               }
            }
         }
         else
         {
            acceptUpdate();
            mEnded = true;
            handler->onTerminated(getHandle(), qn->notify());
            delete this;
         }
      }
      else
      {            
         mDialog.makeResponse(*mLastResponse, qn->notify(), 400);
         mLastResponse->header(h_StatusLine).reason() = "Missing Subscription-State header";
         send(mLastResponse);
         mEnded = true;
         handler->onTerminated(getHandle(), qn->notify());
         delete this;
      }
      return;
   }

   if (!mEnded && isEqualNoCase(qn->notify().header(h_SubscriptionState).value(), Symbols::Active))
   {
      if (refreshInterval)
      {
         mDum.addTimer(DumTimeout::Subscription, refreshInterval, getBaseHandle(), ++mTimerSeq);
         InfoLog (<< "[ClientSubscription] reSUBSCRIBE in " << refreshInterval);
      }
         
      handler->onUpdateActive(getHandle(), qn->notify(), qn->outOfOrder());
   }
   else if (!mEnded && isEqualNoCase(qn->notify().header(h_SubscriptionState).value(), Symbols::Pending))
   {
      if (refreshInterval)
      {
         mDum.addTimer(DumTimeout::Subscription, refreshInterval, getBaseHandle(), ++mTimerSeq);
         InfoLog (<< "[ClientSubscription] reSUBSCRIBE in " << refreshInterval);
      }

      handler->onUpdatePending(getHandle(), qn->notify(), qn->outOfOrder());
   }
   else if (isEqualNoCase(qn->notify().header(h_SubscriptionState).value(), Symbols::Terminated))
   {
      acceptUpdate();
      mEnded = true;
      handler->onTerminated(getHandle(), qn->notify());
      DebugLog (<< "[ClientSubscription] " << mLastRequest->header(h_To) << "[ClientSubscription] Terminated");                   
      delete this;
      return;
   }
   else if (!mEnded)
   {
      handler->onUpdateExtension(getHandle(), qn->notify(), qn->outOfOrder());
   }
}

void
ClientSubscription::dispatch(const DumTimeout& timer)
{
   if (timer.seq() == mTimerSeq)
   {
      if (timer.type() == DumTimeout::SubscriptionRetry)
      {
         // this indicates that the ClientSubscription was created by a 408
         if (mOnNewSubscriptionCalled)
         {
            InfoLog(<< "ClientSubscription: application retry refresh");
            requestRefresh();
         }
         else
         {
            InfoLog(<< "ClientSubscription: application retry new request");
  
            if (mDialog.mRemoteTarget.uri().host().empty())
            {
               SharedPtr<SipMessage> sub = mDum.makeSubscription(mLastRequest->header(h_To), getEventType());
               mDum.send(sub);
            }
            else
            {
               SharedPtr<SipMessage> sub = mDum.makeSubscription(mDialog.mRemoteTarget, getEventType(), getAppDialogSet()->reuse());
               mDum.send(sub);
            }
            
            delete this;
         }
      }
      else
      {
         requestRefresh();
      }
   }
   else
   {
      if (timer.type() == DumTimeout::SendNextNotify)
      {
         DebugLog(<< "got DumTimeout::SendNextNotify");
         processNextNotify();
      }
   }
}

void
ClientSubscription::requestRefresh(UInt32 expires)
{
   if (!mEnded)
   {
      if (mRefreshing)
      {
         DebugLog(<< "queue up refresh request");
         mHaveQueuedRefresh = true;
         mQueuedRefreshInterval = expires;
         return;
      }

      mDialog.makeRequest(*mLastRequest, SUBSCRIBE);
      //!dcm! -- need a mechanism to retrieve this for the event package...part of
      //the map that stores the handlers, or part of the handler API
      if(expires > 0)
      {
         mLastRequest->header(h_Expires).value() = expires;
      }
      mExpires = 0;
      InfoLog (<< "Refresh subscription: " << mLastRequest->header(h_Contacts).front());
      mRefreshing = true;
      send(mLastRequest);
   }
}

void
ClientSubscription::end()
{
   InfoLog (<< "End subscription: " << mLastRequest->header(h_RequestLine).uri());

   if (!mEnded)
   {
      mDialog.makeRequest(*mLastRequest, SUBSCRIBE);
      mLastRequest->header(h_Expires).value() = 0;
      mEnded = true;
      send(mLastRequest);
   }
}

void 
ClientSubscription::acceptUpdate(int statusCode)
{
   assert(!mQueuedNotifies.empty());
   if (mQueuedNotifies.empty())
   {
      InfoLog(<< "No queued notify to accept");
      return;
   }

   QueuedNotify* qn = mQueuedNotifies.front();
   mQueuedNotifies.pop_front();
   mDustbin.push_back(qn);
   mDialog.makeResponse(*mLastResponse, qn->notify(), statusCode);
   send(mLastResponse);
}

void 
ClientSubscription::send(SharedPtr<SipMessage> msg)
{
   DialogUsage::send(msg);

   if (!mEnded)
   {
      if (!mQueuedNotifies.empty() && msg->isResponse())
      {
         mDum.addTimer(DumTimeout::SendNextNotify, 
                       0, 
                       getBaseHandle(),
                       0);
      }
   }

}

void 
ClientSubscription::rejectUpdate(int statusCode, const Data& reasonPhrase)
{
   ClientSubscriptionHandler* handler = mDum.getClientSubscriptionHandler(mEventType);
   assert(handler);   
   assert(!mQueuedNotifies.empty());
   if (mQueuedNotifies.empty())
   {
      InfoLog(<< "No queued notify to reject");
      return;
   }

   QueuedNotify* qn = mQueuedNotifies.front();
   mQueuedNotifies.pop_front();
   mDustbin.push_back(qn);

   mDialog.makeResponse(*mLastResponse, qn->notify(), statusCode);
   if (!reasonPhrase.empty())
   {
      mLastResponse->header(h_StatusLine).reason() = reasonPhrase;
   }
   
   send(mLastResponse);
   switch (Helper::determineFailureMessageEffect(*mLastResponse))
   {
      case Helper::TransactionTermination:
      case Helper::RetryAfter:
         break;            
      case Helper::OptionalRetryAfter:
      case Helper::ApplicationDependant: 
         throw UsageUseException("Not a reasonable code to reject a NOTIFY with inside an established dialog.", 
                                 __FILE__, __LINE__);
         break;            
      case Helper::DialogTermination: //?dcm? -- throw or destroy this?
      case Helper::UsageTermination:
         mEnded = true;
         handler->onTerminated(getHandle(), *mLastResponse);
         delete this;
         break;
   }
}

void ClientSubscription::dialogDestroyed(const SipMessage& msg)
{
   ClientSubscriptionHandler* handler = mDum.getClientSubscriptionHandler(mEventType);
   assert(handler);   
   mEnded = true;
   handler->onTerminated(getHandle(), msg);
   delete this;   
}

std::ostream&
ClientSubscription::dump(std::ostream& strm) const
{
   strm << "ClientSubscription " << mLastRequest->header(h_From).uri();
   return strm;
}

void ClientSubscription::onReadyToSend(SipMessage& msg)
{
   ClientSubscriptionHandler* handler = mDum.getClientSubscriptionHandler(mEventType);
   assert(handler);
   handler->onReadyToSend(getHandle(), msg);
}

void
ClientSubscription::sendQueuedRefreshRequest()
{
   assert(!mRefreshing);

   if (mHaveQueuedRefresh)
   {
      DebugLog(<< "send queued refresh request");
      mHaveQueuedRefresh = false;
      requestRefresh(mQueuedRefreshInterval);
   }
}

void
ClientSubscription::clearDustbin()
{
   for (Dustbin::iterator it = mDustbin.begin(); it != mDustbin.end(); ++it)
   {
      delete *it;
   }

   mDustbin.clear();

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
