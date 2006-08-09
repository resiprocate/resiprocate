#include "resiprocate/Helper.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/SipFrag.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/ClientSubscription.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/SubscriptionHandler.hxx"
#include "resiprocate/dum/SubscriptionCreator.hxx"
#include "resiprocate/dum/UsageUseException.hxx"

#include "resiprocate/dum/AppDialogSet.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM


ClientSubscription::ClientSubscription(DialogUsageManager& dum, Dialog& dialog, const SipMessage& request)
   : BaseSubscription(dum, dialog, request),
     mOnNewSubscriptionCalled(mEventType == "refer"),  // don't call onNewSubscription for Refer subscriptions
     mEnded(false),
     mExpires(0)
{
   mDialog.makeRequest(mLastRequest, SUBSCRIBE);
}

ClientSubscription::~ClientSubscription()
{
   mDialog.mClientSubscriptions.remove(this);
}

ClientSubscriptionHandle 
ClientSubscription::getHandle()
{
   return ClientSubscriptionHandle(mDum, getBaseHandle().getId());
}

void
ClientSubscription::dispatch(const SipMessage& msg)
{
   // asserts are checks the correctness of Dialog::dispatch
   if (msg.isRequest())
   {
      assert( msg.header(h_RequestLine).getMethod() == NOTIFY );

      processRequest(msg);
   }
   else
   {
      processResponse(msg);
   }
}

void
ClientSubscription::processRequest(const SipMessage& msg)
{
   ClientSubscriptionHandler* handler = mDum.getClientSubscriptionHandler(mEventType);
   assert(handler);

   // !dlb! 481 NOTIFY iff state is dead?

   //!dcm! -- heavy, should just store enough information to make response
   mLastNotify = msg;
   mLastNotify.setContents( NULL );

   if (!mOnNewSubscriptionCalled && !getAppDialogSet()->isReUsed())
   {
      InfoLog (<< "[ClientSubscription] " << mLastRequest.header(h_To));
      mDialog.mRemoteTarget = msg.header(h_Contacts).front();
      handler->onNewSubscription(getHandle(), msg);
      mOnNewSubscriptionCalled = true;
   }         

   int expires = 0;      
   //default to 3600 seconds so non-compliant endpoints don't result in leaked usages
   if (msg.exists(h_SubscriptionState) && msg.header(h_SubscriptionState).exists(p_expires))
   {
      expires = msg.header(h_SubscriptionState).param(p_expires);
   }
   else
   {
      expires = 3600;
   }

   if (!mLastRequest.exists(h_Expires))
   {
      mLastRequest.header(h_Expires).value() = expires;
   }

   //if no subscription state header, treat as an extension. Only allow for
   //refer to handle non-compliant implementations
   if (!msg.exists(h_SubscriptionState))
   {
      if (msg.exists(h_Event) && msg.header(h_Event).value() == "refer")
      {
         SipFrag* frag  = dynamic_cast<SipFrag*>(msg.getContents());
         if (frag)
         {
            if (frag->message().isResponse())
            {
               int code = frag->message().header(h_StatusLine).statusCode();
               if (code < 200)
               {
                  handler->onUpdateExtension(getHandle(), msg);
               }
               else
               {
                  acceptUpdate();                     
                  handler->onTerminated(getHandle(), msg);
                  delete this;
               }
            }
         }
         else
         {
            acceptUpdate();
            handler->onTerminated(getHandle(), msg);
            delete this;
         }
      }
      else
      {            
         SipMessage lastResponse;
         mDialog.makeResponse(lastResponse, msg, 400);
         lastResponse.header(h_StatusLine).reason() = "Missing Subscription-State header";
         send(lastResponse);
         handler->onTerminated(getHandle(), msg);
         delete this;
      }
      return;
   }

   unsigned long refreshInterval = 0;
   UInt64 now = Timer::getTimeMs() / 1000;

   if (mExpires == 0 || now + expires < mExpires)
   {
      refreshInterval = Helper::smallerThan((unsigned long)expires);
      mExpires = now + refreshInterval;
   }

   if (!mEnded && msg.header(h_SubscriptionState).value() == "active")
   {
      if (refreshInterval)
      {
         mDum.addTimer(DumTimeout::Subscription, refreshInterval, getBaseHandle(), ++mTimerSeq);
         InfoLog(<< "[ClientSubscription] reSUBSCRIBE in " << refreshInterval);
      }

      handler->onUpdateActive(getHandle(), msg);
   }
   else if (!mEnded && msg.header(h_SubscriptionState).value() == "pending")
   {
      if (refreshInterval)
      {
         mDum.addTimer(DumTimeout::Subscription, refreshInterval, getBaseHandle(), ++mTimerSeq);
         InfoLog(<< "[ClientSubscription] reSUBSCRIBE in " << refreshInterval);
      }

      handler->onUpdatePending(getHandle(), msg);
   }
   else if (msg.header(h_SubscriptionState).value() == "terminated")
   {
      acceptUpdate();
      InfoLog(<< "[ClientSubscription] " << mLastRequest.header(h_To) << "[ClientSubscription] Terminated");
      handler->onTerminated(getHandle(), msg);
      delete this;
      return;
   }
   else if (!mEnded)
   {
      handler->onUpdateExtension(getHandle(), msg);
   }
}

void
ClientSubscription::sendNewSubscription()
{
   bool reuseAppDialogSet = true;
   NameAddr remoteTarget(mDialog.mRemoteTarget);
   if (mDialog.mRemoteTarget.uri().host().empty())
   {
      remoteTarget = mLastRequest.header(h_To);
   }
   remoteTarget.remove(p_tag);
   if (reuseAppDialogSet)
   {
      SipMessage& sub = mDum.makeSubscription(remoteTarget, getEventType(), getAppDialogSet()->reuse());
      mDum.send(sub);
   }
   else
   {
      SipMessage& sub = mDum.makeSubscription(remoteTarget, getEventType());
      mDum.send(sub);
   }
}

void
ClientSubscription::processResponse(const SipMessage& msg)
{
   ClientSubscriptionHandler* handler = mDum.getClientSubscriptionHandler(mEventType);
   assert(handler);

   // !jf! might get an expiration in the 202 but not in the NOTIFY - we're going
   // to ignore this case

   if (msg.header(h_StatusLine).statusCode() == 481)
   {
      InfoLog(<< "Received 481 to SUBSCRIBE, reSUBSCRIBEing (presence server probably restarted) "
         << mDialog.mRemoteTarget);
      if ( !(msg.exists(h_Expires) && msg.header(h_Expires).value() == 0))
      {
         sendNewSubscription();
      }
      handler->onTerminated(getHandle(), msg);
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
         InfoLog(<< "Received 408 to SUBSCRIBE "
            << mLastRequest.header(h_To));
         retry = handler->onRequestRetry(getHandle(), 0, msg);
      }
      else
      {
         InfoLog(<< "Received non-408 retriable to SUBSCRIBE "
            << mLastRequest.header(h_To));
         retry = handler->onRequestRetry(getHandle(), msg.header(h_RetryAfter).value(), msg);
      }

      if (retry < 0)
      {
         DebugLog(<< "Application requested failure on Retry-After");
         handler->onTerminated(getHandle(), msg);
         delete this;
         return;
      }
      else if (retry == 0)
      {
         DebugLog(<< "Application requested immediate retry on Retry-After");
         sendNewSubscription();
         handler->onTerminated(getHandle(), msg);
         delete this;
         return;
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
      if (msg.header(h_StatusLine).statusCode() == 423 &&
         msg.exists(h_MinExpires))
      {
         requestRefresh(msg.header(h_MinExpires).value());
      }
      else
      {
#if 1         
         InfoLog(<< "Received " << msg.header(h_StatusLine).statusCode() << " to SUBSCRIBE "
            << mLastRequest.header(h_To) << " : resubscribe in 20 seconds");

         mDum.addTimer(DumTimeout::SubscriptionRetry, 
            20, 
            getBaseHandle(),
            ++mTimerSeq);
#else      
         InfoLog(<< "Received " << msg.header(h_StatusLine).statusCode() << " to SUBSCRIBE "
            << mLastRequest.header(h_To) << " : simply terminate dialog and send New SUBSUSCRIBE to recover it");
         sendNewSubscription();
         handler->onTerminated(getHandle(), msg);
         delete this;
#endif         
         return;
      }
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
               SipMessage& sub = mDum.makeSubscription(mLastRequest.header(h_To), getEventType());
               mDum.send(sub);
            }
            else
            {
               SipMessage& sub = mDum.makeSubscription(mDialog.mRemoteTarget, getEventType(), getAppDialogSet()->reuse());
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
}

void
ClientSubscription::requestRefresh(int expires)
{
   if (!mEnded)
   {
      mDialog.makeRequest(mLastRequest, SUBSCRIBE);
      //!dcm! -- need a mechanism to retrieve this for the event package...part of
      //the map that stores the handlers, or part of the handler API
      if(expires > 0)
      {
         mLastRequest.header(h_Expires).value() = expires;
      }
      mExpires = 0;
      InfoLog (<< "Refresh subscription: " << mLastRequest.header(h_Contacts).front());
      send(mLastRequest);
   }
}

void
ClientSubscription::end()
{
   InfoLog (<< "End subscription: " << mLastRequest.header(h_RequestLine).uri());

   if (!mEnded)
   {
      mDialog.makeRequest(mLastRequest, SUBSCRIBE);
      mLastRequest.header(h_Expires).value() = 0;
      mEnded = true;
      send(mLastRequest);
   }
}

void 
ClientSubscription::acceptUpdate(int statusCode)
{
   SipMessage lastResponse;
   mDialog.makeResponse(lastResponse, mLastNotify, statusCode);
   send(lastResponse);
}

void 
ClientSubscription::rejectUpdate(int statusCode, const Data& reasonPhrase)
{
   ClientSubscriptionHandler* handler = mDum.getClientSubscriptionHandler(mEventType);
   assert(handler);  

   SipMessage lastResponse;

   mDialog.makeResponse(lastResponse, mLastNotify, statusCode);
   if (!reasonPhrase.empty())
   {
      lastResponse.header(h_StatusLine).reason() = reasonPhrase;
   }
   
   send(lastResponse);
   switch (Helper::determineFailureMessageEffect(lastResponse))
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
         handler->onTerminated(getHandle(), lastResponse);
         delete this;
         break;
   }
}

void ClientSubscription::dialogDestroyed(const SipMessage& msg)
{
   ClientSubscriptionHandler* handler = mDum.getClientSubscriptionHandler(mEventType);
   assert(handler);   
   handler->onTerminated(getHandle(), msg);
   delete this;   
}

std::ostream&
ClientSubscription::dump(std::ostream& strm) const
{
   strm << "ClientSubscription " << mLastRequest.header(h_From).uri();
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
