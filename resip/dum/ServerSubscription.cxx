#include "resip/dum/AppDialog.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/ServerSubscription.hxx"
#include "resip/dum/SubscriptionHandler.hxx"
#include "resip/dum/UsageUseException.hxx"
#include "resip/dum/MasterProfile.hxx"
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
   if (req.header(h_RequestLine).method() == REFER && req.header(h_To).exists(p_tag))
   {
      // If this is an in-dialog REFER, then use a subscription id
      mSubscriptionId = Data(req.header(h_CSeq).sequence());
   }   
   Data key = getEventType() + getDocumentKey();
   mDum.mServerSubscriptions.insert(DialogUsageManager::ServerSubscriptions::value_type(key, this));
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

UInt32
ServerSubscription::getTimeLeft()
{
   UInt32 timeleft =  UInt32(mAbsoluteExpiry - Timer::getTimeSecs());
   if (timeleft < 0) // .kw. this can NEVER happen since unsigned!
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
   // Response is built in dispatch when request arrives, just need to adjust the status code here
   mLastResponse->header(h_StatusLine).responseCode() = statusCode;
   mLastResponse->header(h_Expires).value() = mExpires;
   return mLastResponse;
}

SharedPtr<SipMessage>
ServerSubscription::reject(int statusCode)
{
   if (statusCode < 300)
   {
      throw UsageUseException("Must reject with a code greater than or equal to 300", __FILE__, __LINE__);
   }
   // Response is built in dispatch when request arrives, just need to adjust the status code here
   mLastResponse->header(h_StatusLine).responseCode() = statusCode;
   mLastResponse->remove(h_Contacts);  // Remove any contact header for non-success response
   return mLastResponse;
}

void ServerSubscription::terminateSubscription(ServerSubscriptionHandler* handler)
{
   handler->onTerminated(getHandle());
   delete this;
}

void 
ServerSubscription::send(SharedPtr<SipMessage> msg)
{
   ServerSubscriptionHandler* handler = mDum.getServerSubscriptionHandler(mEventType);
   resip_assert(handler);   
   if (msg->isResponse())
   {
      mLastResponse.reset();  // Release ref count on memory - so message goes away when send is done
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
            mAbsoluteExpiry = Timer::getTimeSecs() + msg->header(h_Expires).value();            
            mSubDlgState = SubDlgEstablished;            
         }
         else
         {
            throw UsageUseException("2xx to a Subscribe MUST contain an Expires header", __FILE__, __LINE__);
         }
      }
      else if (code < 400)
      {
         DialogUsage::send(msg);
         terminateSubscription(handler);
         return;
      }
      else
      {
         if (shouldDestroyAfterSendingFailure(*msg))
         {
            DialogUsage::send(msg);
            terminateSubscription(handler);
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
         terminateSubscription(handler);
      }
   }
}

bool 
ServerSubscription::shouldDestroyAfterSendingFailure(const SipMessage& msg)
{
   int code = msg.header(h_StatusLine).statusCode();
   switch(mSubDlgState)
   {
      case SubDlgInitial:
         return true;
      case SubDlgTerminating: //terminated state not using in ServerSubscription
         resip_assert(0);
         return true;
      case SubDlgEstablished:
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
               // .bwc. Uh, no. ApplicationDependent should imply that the 
               // app-writer has decided what to do. We don't decide here. And 
               // OptionalRetryAfter certainly doesn't mean we should tear the 
               // Usage down.
//               throw UsageUseException("Not a reasonable code to reject a SUBSCIRBE(refresh) inside a dialog.", 
//                                       __FILE__, __LINE__);
               break;            
            case Helper::DialogTermination: //?dcm? -- throw or destroy this?
            case Helper::UsageTermination:
               return true;
         }
         break;
      }
      default: // !jf!
         resip_assert(0);
         break;
   }
   return false;   
}

void 
ServerSubscription::setSubscriptionState(SubscriptionState state)
{
   // Don't allow a transition out of Terminated state
   if (mSubscriptionState != Terminated)
   {
      mSubscriptionState = state;
   }
}

void 
ServerSubscription::dispatch(const SipMessage& msg)
{
   DebugLog( << "ServerSubscription::dispatch: " << msg.brief());

   ServerSubscriptionHandler* handler = mDum.getServerSubscriptionHandler(mEventType);
   resip_assert(handler);

   if (msg.isRequest())
   {      
      //!dcm! -- need to have a mechanism to retrieve default & acceptable
      //expiration times for an event package--part of handler API?
      //added to handler for now.
      if (mLastResponse.get() == 0)
      {
          mLastResponse.reset(new SipMessage);
      }
      mDialog.makeResponse(*mLastResponse, msg, 200);  // Generate response now and wait for user to accept or reject, then adjust status code
   
      int errorResponseCode = 0;
      handler->getExpires(msg,mExpires,errorResponseCode);
      if (errorResponseCode >= 400)
      {
         handler->onError(getHandle(), msg);
         SharedPtr<SipMessage> response = reject(errorResponseCode);

         if (errorResponseCode == 423 && handler->hasMinExpires())
         {
            response->header(h_MinExpires).value() = handler->getMinExpires();		   
         }
         send(response);
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
            // Move to terminated state - application is not allowed to switch out of this state.  This
            // allows applications to treat polling requests just like normal subscriptions.  Any attempt
            // to call setSubscriptionState will NoOp.
            mSubscriptionState = Terminated;

            if (mEventType != "refer" )
            {
               handler->onNewSubscription(getHandle(), msg);
            }
            else if (!invSession.isValid())
            {
               handler->onNewSubscriptionFromRefer(getHandle(), msg);
            }
            // note:  it might be nice to call onExpiredByClient here, but it's dangerous, since inline calls 
            //        to reject or the inline sending of a Notify in the onNewSubscription handler will cause 
            //        'this' to be deleted
            return;
         }

         makeNotifyExpires();  // builds a NOTIFY message into mLastRequest
         handler->onExpiredByClient(getHandle(), msg, *mLastRequest);

         // Send 200 response to sender
         mLastResponse->header(h_Expires).value() = mExpires;
         send(mLastResponse);

         // Send Notify Expires
         send(mLastRequest);
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
      //mLastRequest->releaseContents();
      mLastRequest.reset();   // Release ref count on memory - so message goes away when send is done
      int code = msg.header(h_StatusLine).statusCode();

      if(code < 200)
      {
         return;
      }
      else if (code < 300)
      {
         handler->onNotifyAccepted(getHandle(), msg);
         return;
      }
      else if (code < 400)
      {
         //in dialog NOTIFY got redirected? Bizarre...
         handler->onError(getHandle(), msg);
         terminateSubscription(handler);
      }
      else
      {
         switch(Helper::determineFailureMessageEffect(msg,
             (mDum.getMasterProfile()->additionalTransactionTerminatingResponsesEnabled()) ?
             &mDum.getMasterProfile()->getAdditionalTransactionTerminatingResponses() : NULL))
         {
            case Helper::TransactionTermination:
               DebugLog( << "ServerSubscription::TransactionTermination: " << msg.brief());
               handler->onNotifyRejected(getHandle(), msg);
               break;
            case Helper::UsageTermination:
            case Helper::RetryAfter:
            case Helper::OptionalRetryAfter:
            case Helper::ApplicationDependant: 
            case Helper::DialogTermination:
               DebugLog( << "ServerSubscription::UsageTermination: " << msg.brief());
               handler->onError(getHandle(), msg);
               terminateSubscription(handler);
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
   if (mLastRequest.get() == 0)
   {
      mLastRequest.reset(new SipMessage);
   }
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
ServerSubscription::end(TerminateReason reason, const Contents* document, int retryAfter)
{
   mSubscriptionState = Terminated;
   makeNotify();
   mLastRequest->header(h_SubscriptionState).param(p_reason) = getTerminateReasonString(reason);   
   if (document)
   {
      mLastRequest->setContents(document);
   }
   if (retryAfter != 0)
   {
        mLastRequest->header(h_SubscriptionState).param(p_retryAfter) = retryAfter;
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
   resip_assert(timeout.type() == DumTimeout::Subscription);
   if (timeout.seq() == mTimerSeq)
   {
      ServerSubscriptionHandler* handler = mDum.getServerSubscriptionHandler(mEventType);
      resip_assert(handler);
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
ServerSubscription::onReadyToSend(SipMessage& msg)
{
   ServerSubscriptionHandler* handler = mDum.getServerSubscriptionHandler(mEventType);
   resip_assert(handler);
   handler->onReadyToSend(getHandle(), msg);
}

void
ServerSubscription::flowTerminated()
{
   // notify handler
   ServerSubscriptionHandler* handler = mDum.getServerSubscriptionHandler(mEventType);
   resip_assert(handler);
   handler->onFlowTerminated(getHandle());
}

EncodeStream& 
ServerSubscription::dump(EncodeStream& strm) const
{
   strm << "ServerSubscription " << mSubscriber;
   return strm;
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
