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


ClientSubscription::ClientSubscription(DialogUsageManager& dum, Dialog& dialog, const SipMessage& request)
   : BaseSubscription(dum, dialog, request),
     mOnNewSubscriptionCalled(mEventType == "refer"),  // don't call onNewSubscription for Refer subscriptions
     mEnded(false),
     mNextRefreshSecs(0),
     mLastSubSecs(Timer::getTimeSecs()), // Not exactly, but more forgiving
     mSubscribed(false),
     mRefreshing(false),
     mHaveQueuedRefresh(false),
     mQueuedRefreshInterval(-1),
     mLargestNotifyCSeq(0)
{
   DebugLog (<< "ClientSubscription::ClientSubscription from " << request.brief() << ": " << this);   
   if(request.method() == SUBSCRIBE)
   {
      *mLastRequest = request;
   }
   else
   {
      // If a NOTIFY request is use to make this ClientSubscription, then create the implied SUBSCRIBE 
      // request as the mLastRequest
      mDialog.makeRequest(*mLastRequest, SUBSCRIBE);
   }
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
   DebugLog(<< "ClientSubscription::~ClientSubscription: " << this);
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
   resip_assert(handler);

   clearDustbin();

   // asserts are checks the correctness of Dialog::dispatch
   if (msg.isRequest() )
   {
      resip_assert( msg.header(h_RequestLine).getMethod() == NOTIFY );
      mRefreshing = false;
      mSubscribed = true;   // If we got a NOTIFY then we are subscribed

      // !dlb! 481 NOTIFY iff state is dead?

      //!dcm! -- heavy, should just store enough information to make response
      //mLastNotify = msg;

      //!fj! There is a bug that prevents onNewSubscription from being called
      //     when, for example, the UAS sends a 408 back and
      //     ClientSubscriptionHandler::onRequestRetry returns 0. A fix was
      //     attempted in revision 10128 but it created a more important
      //     regression. See
      //     http://list.resiprocate.org/archive/resiprocate-devel/thrd83.html#08362
      //     for more details.
      if (!mOnNewSubscriptionCalled && !getAppDialogSet()->isReUsed())
      {
         mOnNewSubscriptionCalled = true;
         InfoLog(<< "[ClientSubscription] " << mLastRequest->header(h_To));
         handler->onNewSubscription(getHandle(), msg);
         if (mEnded) return;
      }

      bool outOfOrder = mLargestNotifyCSeq > msg.header(h_CSeq).sequence();
      if (!outOfOrder)
      {
         mLargestNotifyCSeq = msg.header(h_CSeq).sequence();
         // If not out of order, then allow NOTIFY to do a target refresh - RFC6665
         if (msg.exists(h_Contacts))
         {
             mDialog.mRemoteTarget = msg.header(h_Contacts).front();
         }
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
   resip_assert(handler);

   mRefreshing = false;
   int statusCode = msg.header(h_StatusLine).statusCode();

   if (statusCode >= 200 && statusCode <300)
   {
      mSubscribed = true;   // If we got a 200 response then we are subscribed
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

      if(!mOnNewSubscriptionCalled)
      {
         mOnNewSubscriptionCalled = true;
         handler->onNewSubscription(getHandle(), msg);
         if (!mEnded)
         {
            // Timer for initial NOTIFY; since we don't know when the initial
            // SUBSCRIBE is sent, we have to set the timer when the 200 comes in, if
            // it beats the NOTIFY.
            mDum.addTimerMs(DumTimeout::WaitForNotify,
                64 * Timer::T1,
                getBaseHandle(),
                ++mTimerSeq);
         }
      }
      else if (!mEnded)
      {
         sendQueuedRefreshRequest();
      }
   }
   else if (!mEnded &&
            statusCode == 481 &&
            msg.exists(h_Expires) && msg.header(h_Expires).value() > 0)
   {
      InfoLog (<< "Received 481 to SUBSCRIBE, reSUBSCRIBEing (presence server probably restarted) "
               << mLastRequest->header(h_To));

      reSubscribe();  // will delete "this"
      return;
   }
   else if (!mEnded &&
            (statusCode == 408 ||
             (statusCode == 503 && !msg.isFromWire()) ||
             ((statusCode == 413 ||
               statusCode == 480 ||
               statusCode == 486 ||
               statusCode == 500 ||
               statusCode == 503 ||
               statusCode == 600 ||
               statusCode == 603) &&
              msg.exists(h_RetryAfter))))
   {
      int retry;
      int retryAfter = 0;
      if(msg.exists(h_RetryAfter))
      {
         retryAfter = msg.header(h_RetryAfter).value();
      }

      InfoLog (<< "Received " << statusCode << " to SUBSCRIBE "
               << mLastRequest->header(h_To));
      retry = handler->onRequestRetry(getHandle(), retryAfter, msg);

      if (retry < 0)
      {
         DebugLog(<< "Application requested failure on Retry-After");
         mEnded = true;
         handler->onTerminated(getHandle(), &msg);
         delete this;
         return;
      }
      else if (retry == 0)
      {
         DebugLog(<< "Application requested immediate retry on Retry-After");

         if (mOnNewSubscriptionCalled)
         {
            // If we already have a dialog, then just refresh again
            requestRefresh();
         }
         else
         {
            reSubscribe();  // will delete "this"
            return;
         }
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
         handler->onTerminated(getHandle(), &msg);
         delete this;
         return;
      }
   }
}

void 
ClientSubscription::processNextNotify()
{
   //!dcm! There is a timing issue in this code which can cause this to be
   //!called when there are no queued NOTIFY messages. Probably a subscription
   //!teardown/timer crossover.
   //assert(!mQueuedNotifies.empty());
   if (mQueuedNotifies.empty())
   {
      return;
   }

   QueuedNotify* qn = mQueuedNotifies.front();
   ClientSubscriptionHandler* handler = mDum.getClientSubscriptionHandler(mEventType);
   resip_assert(handler);

   unsigned long refreshInterval = 0;
   bool setRefreshTimer=false; 
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
      else
      {
         /* if we haven't gotten an expires value from:
            1. the subscription state from this notify
            2. the last request (may have came from the 2xx in response)
            then use some reasonable value.
          */
         expires = 3600;
      }
      
      if (!mLastRequest->exists(h_Expires))
      {
         DebugLog(<< "No expires header in last request, set to " << expires);
         mLastRequest->header(h_Expires).value() = expires;
      }

      if(!qn->notify().exists(h_SubscriptionState) || 
         !isEqualNoCase(qn->notify().header(h_SubscriptionState).value(), Symbols::Terminated))
      {
         // Don't do this stuff for a NOTIFY terminated.
         UInt64 now = Timer::getTimeSecs();
         refreshInterval = Helper::aBitSmallerThan((signed long)expires);
         
         if (mNextRefreshSecs == 0 || now + refreshInterval < mNextRefreshSecs)
         {
            mNextRefreshSecs = now + refreshInterval;
            setRefreshTimer = true;
         }
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
                  handler->onTerminated(getHandle(), &qn->notify());
                  delete this;
               }
            }
            else
            {
               acceptUpdate();
               mEnded = true;
               handler->onTerminated(getHandle(), &qn->notify());
               delete this;
            }
         }
         else
         {
            acceptUpdate();
            mEnded = true;
            handler->onTerminated(getHandle(), &qn->notify());
            delete this;
         }
      }
      else
      {            
         mDialog.makeResponse(*mLastResponse, qn->notify(), 400);
         mLastResponse->header(h_StatusLine).reason() = "Missing Subscription-State header";
         send(mLastResponse);
         mEnded = true;
         handler->onTerminated(getHandle(), &qn->notify());
         delete this;
      }
      return;
   }

   if (!mEnded && isEqualNoCase(qn->notify().header(h_SubscriptionState).value(), Symbols::Active))
   {
      if (setRefreshTimer)
      {
         scheduleRefresh(refreshInterval);
      }
         
      handler->onUpdateActive(getHandle(), qn->notify(), qn->outOfOrder());
   }
   else if (!mEnded && isEqualNoCase(qn->notify().header(h_SubscriptionState).value(), Symbols::Pending))
   {
      if (setRefreshTimer)
      {
         scheduleRefresh(refreshInterval);
      }

      handler->onUpdatePending(getHandle(), qn->notify(), qn->outOfOrder());
   }
   else if (isEqualNoCase(qn->notify().header(h_SubscriptionState).value(), Symbols::Terminated))
   {
      if (mLastRequest->header(h_Expires).value() != 0 &&
         isEqualNoCase(qn->notify().header(h_SubscriptionState).param(p_reason), "timeout"))
      {
         // Unexpected timeout of some sort. Look closer.
         if(mNextRefreshSecs==0)
         {
            // No refresh scheduled; maybe we are trying to avoid a tight SUB/
            // NOT loop here?
            if(Helper::aBitSmallerThan((signed long)(Timer::getTimeSecs() - mLastSubSecs)) < 2)
            {
               acceptUpdate(200, "I just sent a refresh, what more do you want from me?");
            }
            else
            {
               acceptUpdate(200, "Why didn't I refresh here?");
            }
         }
         else
         {
            acceptUpdate(200, "You terminated my subscription early! What gives?");
         }
      }
      else
      {
         acceptUpdate();
      }
      mEnded = true;
      handler->onTerminated(getHandle(), &qn->notify());
      DebugLog (<< "[ClientSubscription] " << mLastRequest->header(h_To) << "[ClientSubscription] Terminated");                   
      delete this;
      return;
   }
   else if (!mEnded)
   {
      if (setRefreshTimer)
      {
         scheduleRefresh(refreshInterval);
      }

      handler->onUpdateExtension(getHandle(), qn->notify(), qn->outOfOrder());
   }
   else if (mEnded)
   {
      // We received a NOTIFY message when we thought the subscription was
      // ended. This can happen, for example, when a previously sent NOTIFY gets
      // resent while we (ClientSubscription) are trying to terminate the
      // subscription. If we don't accept/reject this NOTIFY, it will stay into
      // the mQueuedNotifies queue and we'll never terminate the subscription
      // even if the server sends a NOTIFY/terminated. All received NOTIFY would
      // get piled up on mQueuedNotifies and they will never get processed.
      //
      // Note that if that NOTIFY is in fact the terminated one, it will get
      // caught by another if statement above and acted upon appropriately.
      //
      //!fjoanis! Is 481 a proper error code in this case?
      InfoLog(<< "[ClientSubscription] received NOTIFY when subscription was ended, rejecting it...");
      rejectUpdate(481);
   }
}

void
ClientSubscription::dispatch(const DumTimeout& timer)
{
   if (timer.seq() == mTimerSeq)
   {
      if(timer.type() == DumTimeout::WaitForNotify)
      {
         ClientSubscriptionHandler* handler = mDum.getClientSubscriptionHandler(mEventType);
         if(mEnded)
         {
            InfoLog(<< "ClientSubscription: received NOTIFY timeout when trying to end, terminating...");
            // NOTIFY terminated didn't come in
            handler->onTerminated(getHandle(),0);
            delete this;
            return;
         }

         // Initial NOTIFY never came in; let app decide what to do
         handler->onNotifyNotReceived(getHandle());
      }
      else if (timer.type() == DumTimeout::SubscriptionRetry)
      {
         // Ensure someone hasn't called end() while we were waiting
         if (!mEnded)
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
               reSubscribe();  // will delete "this"
               return;
            }
         }
      }
      else if(timer.type() == DumTimeout::Subscription)
      {
         requestRefresh();
      }
   }
   else if(timer.seq() == 0 && timer.type() == DumTimeout::SendNextNotify)
   {
      DebugLog(<< "got DumTimeout::SendNextNotify");
      processNextNotify();
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
      mNextRefreshSecs = 0;
      InfoLog (<< "Refresh subscription: " << mLastRequest->header(h_Contacts).front());
      mRefreshing = true;
      mLastSubSecs = Timer::getTimeSecs();
      send(mLastRequest);
      // Timer for reSUB NOTIFY.
      mDum.addTimerMs(DumTimeout::WaitForNotify, 
              64*Timer::T1, 
              getBaseHandle(),
              ++mTimerSeq);
   }
}

class ClientSubscriptionRefreshCommand : public DumCommandAdapter
{
public:
   ClientSubscriptionRefreshCommand(const ClientSubscriptionHandle& clientSubscriptionHandle, UInt32 expires)
      : mClientSubscriptionHandle(clientSubscriptionHandle),
        mExpires(expires)
   {

   }
   virtual void executeCommand()
   {
      if(mClientSubscriptionHandle.isValid())
      {
         mClientSubscriptionHandle->requestRefresh(mExpires);
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "ClientSubscriptionRefreshCommand";
   }
private:
   ClientSubscriptionHandle mClientSubscriptionHandle;
   UInt32 mExpires;
};

void
ClientSubscription::requestRefreshCommand(UInt32 expires)
{
   mDum.post(new ClientSubscriptionRefreshCommand(getHandle(), expires));
}

void
ClientSubscription::end()
{
    end(false /* immediate? */);
}

void
ClientSubscription::end(bool immediate)
{
   if (!mEnded)
   {
      if(!immediate && mSubscribed)
      {
         InfoLog(<< "End subscription: " << mLastRequest->header(h_RequestLine).uri());
         mDialog.makeRequest(*mLastRequest, SUBSCRIBE);
         mLastRequest->header(h_Expires).value() = 0;
         mEnded = true;
         send(mLastRequest);
         // Timer for NOTIFY terminated
         mDum.addTimerMs(DumTimeout::WaitForNotify, 
                 64*Timer::T1, 
                 getBaseHandle(),
                 ++mTimerSeq);
      }
      else
      {
         InfoLog(<< "End subscription immediately: " << mLastRequest->header(h_RequestLine).uri());
         delete this;
         return;
      }
   }
   else
   {
      InfoLog(<< "End subscription called but already ended: " << mLastRequest->header(h_RequestLine).uri());
   }
}

class ClientSubscriptionEndCommand : public DumCommandAdapter
{
public:
   ClientSubscriptionEndCommand(const ClientSubscriptionHandle& clientSubscriptionHandle, bool immediate)
      :mClientSubscriptionHandle(clientSubscriptionHandle), mImmediate(immediate)
   {

   }

   virtual void executeCommand()
   {
      if(mClientSubscriptionHandle.isValid())
      {
         mClientSubscriptionHandle->end(mImmediate);
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "ClientSubscriptionEndCommand";
   }
private:
   ClientSubscriptionHandle mClientSubscriptionHandle;
   bool mImmediate;
};

void
ClientSubscription::endCommand(bool immediate)
{
   mDum.post(new ClientSubscriptionEndCommand(getHandle(), immediate));
}

void 
ClientSubscription::acceptUpdate(int statusCode, const char* reason)
{
   resip_assert(!mQueuedNotifies.empty());
   if (mQueuedNotifies.empty())
   {
      InfoLog(<< "No queued notify to accept");
      return;
   }

   QueuedNotify* qn = mQueuedNotifies.front();
   mQueuedNotifies.pop_front();
   mDustbin.push_back(qn);
   mDialog.makeResponse(*mLastResponse, qn->notify(), statusCode);
   if(reason)
   {
      mLastResponse->header(h_StatusLine).reason()=reason;
   }
   send(mLastResponse);
}

class ClientSubscriptionAcceptUpdateCommand : public DumCommandAdapter
{
public:
   ClientSubscriptionAcceptUpdateCommand(const ClientSubscriptionHandle& clientSubscriptionHandle, int statusCode, const char* reason)
      : mClientSubscriptionHandle(clientSubscriptionHandle),
        mStatusCode(statusCode),
        mReason(reason ? Data(reason) : Data::Empty)
   {

   }

   virtual void executeCommand()
   {
      if(mClientSubscriptionHandle.isValid())
      {
         mClientSubscriptionHandle->acceptUpdate(mStatusCode, mReason.c_str());
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "ClientSubscriptionAcceptUpdateCommand";
   }
private:
   ClientSubscriptionHandle mClientSubscriptionHandle;
   int mStatusCode;
   Data mReason;
};

void 
ClientSubscription::acceptUpdateCommand(int statusCode, const char* reason)
{
   mDum.post(new ClientSubscriptionAcceptUpdateCommand(getHandle(), statusCode, reason));
}

void
ClientSubscription::reSubscribe()
{
   NameAddr target(mLastRequest->header(h_To));
   target.remove(p_tag);  // ensure To tag is removed
   SharedPtr<SipMessage> sub = mDum.makeSubscription(target, getUserProfile(), getEventType(), getAppDialogSet()->reuse());
   mDum.send(sub);

   delete this;
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
   resip_assert(handler);   
   resip_assert(!mQueuedNotifies.empty());
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
         // If we are already "ended" then we are likely here because we are rejecting an inbound 
         // NOTIFY with a 481 that crossed with our un-subscribe request (due to end()).  In this case we don't 
         // want to call onTerminated or delete this - we wait for end() request to run it's course.
         if (!mEnded)
         {
            mEnded = true;
            handler->onTerminated(getHandle(), mLastResponse.get());
            delete this;
         }
         break;
   }
}

class ClientSubscriptionRejectUpdateCommand : public DumCommandAdapter
{
public:
   ClientSubscriptionRejectUpdateCommand(const ClientSubscriptionHandle& clientSubscriptionHandle, int statusCode, const Data& reasonPhrase)
      : mClientSubscriptionHandle(clientSubscriptionHandle),
        mStatusCode(statusCode),
        mReasonPhrase(reasonPhrase)
   {
   }

   virtual void executeCommand()
   {
      if(mClientSubscriptionHandle.isValid())
      {
         mClientSubscriptionHandle->rejectUpdate(mStatusCode, mReasonPhrase);
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "ClientSubscriptionRejectUpdateCommand";
   }
private:
   ClientSubscriptionHandle mClientSubscriptionHandle;
   int mStatusCode;
   Data mReasonPhrase;
};

void 
ClientSubscription::rejectUpdateCommand(int statusCode, const Data& reasonPhrase)
{
   mDum.post(new ClientSubscriptionRejectUpdateCommand(getHandle(), statusCode, reasonPhrase));
}

EncodeStream&
ClientSubscription::dump(EncodeStream& strm) const
{
   strm << "ClientSubscription " << mLastRequest->header(h_From).uri();
   return strm;
}

void 
ClientSubscription::onReadyToSend(SipMessage& msg)
{
   ClientSubscriptionHandler* handler = mDum.getClientSubscriptionHandler(mEventType);
   resip_assert(handler);
   handler->onReadyToSend(getHandle(), msg);
}

void
ClientSubscription::flowTerminated()
{
   // notify handler
   ClientSubscriptionHandler* handler = mDum.getClientSubscriptionHandler(mEventType);
   resip_assert(handler);
   handler->onFlowTerminated(getHandle());
}

void
ClientSubscription::sendQueuedRefreshRequest()
{
   resip_assert(!mRefreshing);

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

void 
ClientSubscription::scheduleRefresh(unsigned long refreshInterval)
{
   if(mNextRefreshSecs-mLastSubSecs < 2)
   {
      // Server is using an unreasonably short expiry; we sent a SUB
      // very recently, and the server has told us to refresh almost 
      // immediately. By the time our refresh timer pops, less than two 
      // seconds will have elapsed since our last SUBSCRIBE. This is 
      // unacceptable. Just let the subscription end.
      // It is also possible that our refresh SUB has crossed an update NOTIFY 
      // on the wire; in this case, the right thing to do is to wait until a 
      // NOTIFY for our refresh SUB comes in, which is exactly what this code 
      // ends up doing in this case.
      // ?bwc? Make this minimum inter-SUBSCRIBE time configurable?
      WarningLog(<< "Server is using an unacceptably short expiry. "
                  "Letting the subscription end so we don't get in a"
                  " tight SUB/NOT loop.");
      mNextRefreshSecs=0;
   }
   else
   {
      mDum.addTimer(DumTimeout::Subscription, refreshInterval, getBaseHandle(), ++mTimerSeq);
      InfoLog (<< "[ClientSubscription] reSUBSCRIBE in " << refreshInterval);
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
