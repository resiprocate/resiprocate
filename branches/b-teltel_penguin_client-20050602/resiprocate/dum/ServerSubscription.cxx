#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/ServerSubscription.hxx"
#include "resiprocate/dum/SubscriptionHandler.hxx"
#include "resiprocate/dum/UsageUseException.hxx"
#include "resiprocate/dum/InternalServerSubscriptionMessage.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/os/Logger.hxx"

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
   mDum.mServerSubscriptions.insert(std::make_pair(key, this));
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
   
   mDum.mServerSubscriptions.erase(key);
   mDialog.mServerSubscriptions.remove(this);
}

void 
ServerSubscription::endAsync()
{
   InfoLog(<< "End server subscription Async");
   mDum.post(new InternalServerSubscriptionMessage_End(getHandle()));
}

void 
ServerSubscription::sendAsync(const SipMessage& msg)
{
   InfoLog(<< "Send server subscription msg Async");
   mDum.post(new InternalServerSubscriptionMessage_Send(getHandle(), msg));
}

void 
ServerSubscription::sendAsync(bool accept, int statusCode)
{
   InfoLog(<< "Send server subscription msg Async " << statusCode);
   mDum.post(new InternalServerSubscriptionMessage_Send(getHandle(), accept, statusCode));
}

void 
ServerSubscription::sendAsync(std::auto_ptr<Contents> updateContents)
{
   InfoLog(<< "Send update content server subscription msg Async");
   mDum.post(new InternalServerSubscriptionMessage_Send(getHandle(), updateContents));
}

void 
ServerSubscription::dispatchAsync(const SipMessage& msg)
{
   InfoLog(<< "Dispatch server subscription msg Async");
   mDum.post(new InternalServerSubscriptionMessage_DispatchSipMsg(getHandle(), msg));
}

void 
ServerSubscription::dispatchAsync(const DumTimeout& timer)
{
   InfoLog(<< "Dispatch server subscription msg timer Async: " << timer);
   mDum.post(new InternalServerSubscriptionMessage_DispatchTimeoutMsg(getHandle(), timer));
}

void 
ServerSubscription::endAsync(TerminateReason reason, std::auto_ptr<Contents> document)
{
   InfoLog(<< "End server subscription Async: " << reason);
   mDum.post(new InternalServerSubscriptionMessage_EndReason(getHandle(), reason, document));
}

int
ServerSubscription::getTimeLeft()
{
   int timeleft =  mAbsoluteExpiry - time(0);
   if (timeleft < 0)
   {
      return 0;
   }
   else
   {
      return timeleft;
   }
}

SipMessage& 
ServerSubscription::accept(int statusCode)
{
   mDialog.makeResponse(mLastResponse, mLastRequest, statusCode);
   mLastResponse.header(h_Expires).value() = mExpires;
   return mLastResponse;
}

SipMessage& 
ServerSubscription::reject(int statusCode)
{
   if (statusCode < 400)
   {
      throw UsageUseException("Must reject with a 4xx", __FILE__, __LINE__);
   }
   mDialog.makeResponse(mLastResponse, mLastRequest, statusCode);
   return mLastResponse;
}


void 
ServerSubscription::send(SipMessage& msg)
{
   ServerSubscriptionHandler* handler = mDum.getServerSubscriptionHandler(mEventType);
   assert(handler);   
   if (msg.isResponse())
   {
      int code = msg.header(h_StatusLine).statusCode();
      if (code < 200)
      {
         mDum.send(msg);
      }
      else if (code < 300)
      {
         if(msg.exists(h_Expires))
         {
            mDum.addTimer(DumTimeout::Subscription, msg.header(h_Expires).value(), getBaseHandle(), ++mTimerSeq);
            mDum.send(msg);
            mAbsoluteExpiry = time(0) + msg.header(h_Expires).value();            
            mState = Established;            
         }
         else
         {
            throw UsageUseException("2xx to a Subscribe MUST contain an Expires header", __FILE__, __LINE__);
         }
      }
      else if (code < 400)
      {
         mDum.send(msg);
         handler->onTerminated(getHandle());
         delete this;
         return;
      }
      else
      {
         if (shouldDestroyAfterSendingFailure(msg))
         {
            mDum.send(msg);
            handler->onTerminated(getHandle());
            delete this;
            return;
         }
         else
         {
            mDum.send(msg);
         }
      }
   }
   else
   {
      mDum.send(msg);
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
         switch (Helper::determineFailureMessageEffect(mLastResponse))
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
      mLastRequest = msg;      
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
      if (mExpires == 0)
      {
         makeNotifyExpires();
         handler->onExpiredByClient(getHandle(), msg, mLastNotify);
         
         mDialog.makeResponse(mLastResponse, mLastRequest, 200);
         mLastResponse.header(h_Expires).value() = mExpires;
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
            handler->onNewSubscription(getHandle(), msg);
         }
      }
      else
      {
         handler->onRefresh(getHandle(), msg);            
      }
   }
   else
   {     
      //.dcm. - will need to change if retry-afters are reaching here
      mLastNotify.releaseContents();
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
   mLastNotify.header(h_SubscriptionState).param(p_reason) = getTerminateReasonString(Timeout);   
}

void
ServerSubscription::makeNotify()
{
   mDialog.makeRequest(mLastNotify, NOTIFY);
   mLastNotify.header(h_SubscriptionState).value() = getSubscriptionStateString(mSubscriptionState);
   if (mSubscriptionState == Terminated)
   {
      mLastNotify.header(h_SubscriptionState).remove(p_expires);      
   }
   else
   {
      mLastNotify.header(h_SubscriptionState).param(p_expires) = getTimeLeft();
   }
   
   mLastNotify.header(h_Event).value() = mEventType;   
   if (!mSubscriptionId.empty())
   {
      mLastNotify.header(h_Event).param(p_id) = mSubscriptionId;
   }
}


void
ServerSubscription::end(TerminateReason reason, const Contents* document)
{
   mSubscriptionState = Terminated;
   makeNotify();
   mLastNotify.header(h_SubscriptionState).param(p_reason) = getTerminateReasonString(reason);   
   if (document)
   {
      mLastNotify.setContents(document);
   }
   send(mLastNotify);
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
      handler->onExpired(getHandle(), mLastNotify);
      send(mLastNotify);
   }
}

SipMessage& 
ServerSubscription::update(const Contents* document)
{
   makeNotify();
   mLastNotify.setContents(document);
   return mLastNotify;
}

SipMessage& 
ServerSubscription::neutralNotify()
{
   makeNotify();
   mLastNotify.releaseContents();   
   return mLastNotify;
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
