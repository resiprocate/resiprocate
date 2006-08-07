#include "resip/dum/AppDialog.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/ServerSubscription.hxx"
#include "resip/dum/SubscriptionHandler.hxx"
#include "resip/dum/UsageUseException.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"

#include <time.h>

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

ServerSubscriptionHandle 
ServerSubscription::getHandle()
{
   return ServerSubscriptionHandle(mDum, getBaseHandle().getId());
}

ServerSubscription::ServerSubscription(DialogUsageManager& dum,
                                       Dialog& dialog,
                                       const SipMessage& req)
   : BaseSubscription(dum, dialog, req),
     mSubscriber(req.header(h_From).uri().getAor()),
     mExpires(60),
     mAbsoluteExpiry(0)
{
   Data key = getEventType() + getDocumentKey();
   mDum.mServerSubscriptions.insert(DialogUsageManager::ServerSubscriptions::value_type(key, this));
   //mDum.mServerSubscriptions.insert(std::make_pair(key, this));
}

ServerSubscription::~ServerSubscription()
{
   DebugLog(<< "ServerSubscription::~ServerSubscription");
   
   Data key = getEventType() + getDocumentKey();

   std::pair<DialogUsageManager::ServerSubscriptions::iterator,DialogUsageManager::ServerSubscriptions::iterator> subs;
   subs = mDum.mServerSubscriptions.equal_range(key);
   for (DialogUsageManager::ServerSubscriptions::iterator i=subs.first; i!=subs.second; ++i)
   {
      if (i->second == this)
      {
         mDum.mServerSubscriptions.erase(i);
         break;
      }
   }
   
   mDialog.mServerSubscriptions.remove(this);
}

int
ServerSubscription::getTimeLeft()
{
   int timeleft =  int(mAbsoluteExpiry - time(0));
   if (timeleft < 0)
   {
      return 0;
   }
   else
   {
      return timeleft;
   }
}

SharedPtr<SipMessage>
ServerSubscription::accept(int statusCode)
{
   mDialog.makeResponse(*mLastResponse, mLastSubscribe, statusCode);
   mLastResponse->header(h_Expires).value() = mExpires;
   return mLastResponse;
}

SharedPtr<SipMessage>
ServerSubscription::reject(int statusCode)
{
   if (statusCode < 400)
   {
      throw UsageUseException("Must reject with a 4xx", __FILE__, __LINE__);
   }
   mDialog.makeResponse(*mLastResponse, mLastSubscribe, statusCode);
   return mLastResponse;
}


void 
ServerSubscription::send(SharedPtr<SipMessage> msg)
{
   ServerSubscriptionHandler* handler = mDum.getServerSubscriptionHandler(mEventType);
   assert(handler);   
   if (msg->isResponse())
   {
      int code = msg->header(h_StatusLine).statusCode();
      if (code < 200)
      {
         DialogUsage::send(msg);
      }
      else if (code < 300)
      {
         if(msg->exists(h_Expires))
         {
            mDum.addTimer(DumTimeout::Subscription, msg->header(h_Expires).value(), getBaseHandle(), ++mTimerSeq);
            DialogUsage::send(msg);
            mAbsoluteExpiry = time(0) + msg->header(h_Expires).value();            
            mState = Established;            
         }
         else
         {
            throw UsageUseException("2xx to a Subscribe MUST contain an Expires header", __FILE__, __LINE__);
         }
      }
      else if (code < 400)
      {
         DialogUsage::send(msg);
         handler->onTerminated(getHandle());
         delete this;
         return;
      }
      else
      {
         if (shouldDestroyAfterSendingFailure(*msg))
         {
            DialogUsage::send(msg);
            handler->onTerminated(getHandle());
            delete this;
            return;
         }
         else
         {
            DialogUsage::send(msg);
         }
      }
   }
   else
   {
      DialogUsage::send(msg);
      if (mSubscriptionState == Terminated)
      {
         handler->onTerminated(getHandle());
         delete this;
      }
   }
}

bool 
ServerSubscription::shouldDestroyAfterSendingFailure(const SipMessage& msg)
{
   int code = msg.header(h_StatusLine).statusCode();
   switch(mState)
   {
      case Initial:
         return true;
      case Terminated: //terminated state not using in ServerSubscription
         assert(0);
         return true;
      case Established:
      {
         if (code == 405)
         {
            return true;
         }
         switch (Helper::determineFailureMessageEffect(*mLastResponse))
         {
            case Helper::TransactionTermination:
            case Helper::RetryAfter:
               break;
            case Helper::OptionalRetryAfter:
            case Helper::ApplicationDependant: 
               throw UsageUseException("Not a reasonable code to reject a SUBSCIRBE(refresh) inside a dialog.", 
                                       __FILE__, __LINE__);
               break;            
            case Helper::DialogTermination: //?dcm? -- throw or destroy this?
            case Helper::UsageTermination:
               return true;
         }
      }
      default: // !jf!
         break;
         
   }
   return false;   
}

void 
ServerSubscription::setSubscriptionState(SubscriptionState state)
{
   mSubscriptionState = state;
}

void 
ServerSubscription::dispatch(const SipMessage& msg)
{
   DebugLog( << "ServerSubscriptionHandler::dispatch: " << msg.brief());

   ServerSubscriptionHandler* handler = mDum.getServerSubscriptionHandler(mEventType);
   assert(handler);

   if (msg.isRequest())
   {      
      //!dcm! -- need to have a mechanism to retrieve default & acceptable
      //expiration times for an event package--part of handler API?
      //added to handler for now.
      mLastSubscribe = msg;      
      if (msg.exists(h_Expires))
      {         
         mExpires = msg.header(h_Expires).value();
      }
      else if (handler->hasDefaultExpires())
      {
         mExpires = handler->getDefaultExpires();
      }
      else
      {
         handler->onError(getHandle(), msg);
         send(reject(400));
         return;
      }

      InviteSessionHandle invSession;
      if (getAppDialog().isValid())
      {
         invSession = getAppDialog()->getInviteSession();
      }

      if (mExpires == 0)
      {
         /* This is to handle the case where mExpires is zero because the client
            is attempting to poll.  In order for polling to work, the subscription
            handler needs to get the onNewSubscription call. .mjf.
          */
         if (mSubscriptionState == Invalid)
         {
            mSubscriptionState = Terminated;
            if (mEventType != "refer" )
            {
               handler->onNewSubscription(getHandle(), msg);
            }
            else if (!invSession.isValid())
            {
               handler->onNewSubscriptionFromRefer(getHandle(), msg);
            }
         }

         makeNotifyExpires();
         handler->onExpiredByClient(getHandle(), msg, *mLastRequest);
         
         mDialog.makeResponse(*mLastResponse, mLastSubscribe, 200);
         mLastResponse->header(h_Expires).value() = mExpires;
         send(mLastResponse);
         end(Timeout);
         return;
      }
      if (mSubscriptionState == Invalid)
      {
         //!dcm! -- should initial state be pending?
         mSubscriptionState = Init;
         if (mEventType != "refer")
         {
            DebugLog(<< "onNewSubscription called");
            handler->onNewSubscription(getHandle(), msg);
         }
         else if (!invSession.isValid())
         {
            DebugLog(<< "onNewSubscriptionFromRefer called");
            handler->onNewSubscriptionFromRefer(getHandle(), msg);
         }
      }
      else
      {
         DebugLog(<< "onRefresh called");
         handler->onRefresh(getHandle(), msg);            
      }
   }
   else
   {     
      //.dcm. - will need to change if retry-afters are reaching here
      mLastRequest->releaseContents();
      int code = msg.header(h_StatusLine).statusCode();
      if (code < 300)
      { 
         return;
      }
      else if (code < 400)
      {
         //in dialog NOTIFY got redirected? Bizarre...
         handler->onError(getHandle(), msg);
         handler->onTerminated(getHandle());
         delete this;         
      }
      else
      {
         switch(Helper::determineFailureMessageEffect(msg))
         {
            case Helper::TransactionTermination:
               DebugLog( << "ServerSubscriptionHandler::TransactionTermination: " << msg.brief());
               handler->onNotifyRejected(getHandle(), msg);
               break;
            case Helper::UsageTermination:
            case Helper::RetryAfter:
            case Helper::OptionalRetryAfter:
            case Helper::ApplicationDependant: 
            case Helper::DialogTermination:
               DebugLog( << "ServerSubscriptionHandler::UsageTermination: " << msg.brief());
               handler->onError(getHandle(), msg);
               handler->onTerminated(getHandle());
               delete this;
               break;
         }
      }
   }
}

void
ServerSubscription::makeNotifyExpires()
{
   mSubscriptionState = Terminated;
   makeNotify();
   mLastRequest->header(h_SubscriptionState).param(p_reason) = getTerminateReasonString(Timeout);   
}

void
ServerSubscription::makeNotify()
{
   mDialog.makeRequest(*mLastRequest, NOTIFY);
   mLastRequest->header(h_SubscriptionState).value() = getSubscriptionStateString(mSubscriptionState);
   if (mSubscriptionState == Terminated)
   {
      mLastRequest->header(h_SubscriptionState).remove(p_expires);      
   }
   else
   {
      mLastRequest->header(h_SubscriptionState).param(p_expires) = getTimeLeft();
   }
   
   mLastRequest->header(h_Event).value() = mEventType;   
   if (!mSubscriptionId.empty())
   {
      mLastRequest->header(h_Event).param(p_id) = mSubscriptionId;
   }
}


void
ServerSubscription::end(TerminateReason reason, const Contents* document)
{
   mSubscriptionState = Terminated;
   makeNotify();
   mLastRequest->header(h_SubscriptionState).param(p_reason) = getTerminateReasonString(reason);   
   if (document)
   {
      mLastRequest->setContents(document);
   }
   send(mLastRequest);
}

void
ServerSubscription::end()
{
   end(Timeout);
}

void
ServerSubscription::dispatch(const DumTimeout& timeout)
{
   assert(timeout.type() == DumTimeout::Subscription);
   if (timeout.seq() == mTimerSeq)
   {
      ServerSubscriptionHandler* handler = mDum.getServerSubscriptionHandler(mEventType);
      assert(handler);
      makeNotifyExpires();
      handler->onExpired(getHandle(), *mLastRequest);
      send(mLastRequest);
   }
}

SharedPtr<SipMessage>
ServerSubscription::update(const Contents* document)
{
   makeNotify();
   mLastRequest->setContents(document);
   return mLastRequest;
}

SharedPtr<SipMessage>
ServerSubscription::neutralNotify()
{
   makeNotify();
   mLastRequest->releaseContents();   
   return mLastRequest;
}

void 
ServerSubscription::dialogDestroyed(const SipMessage& msg)
{
   ServerSubscriptionHandler* handler = mDum.getServerSubscriptionHandler(mEventType);
   assert(handler);   
   handler->onError(getHandle(), msg);
   handler->onTerminated(getHandle());
   delete this;
}

void ServerSubscription::onReadyToSend(SipMessage& msg)
{
   ServerSubscriptionHandler* handler = mDum.getServerSubscriptionHandler(mEventType);
   assert(handler);
   handler->onReadyToSend(getHandle(), msg);
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
