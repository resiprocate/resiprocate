#include "resiprocate/Helper.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/SipFrag.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/ClientSubscription.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/SubscriptionHandler.hxx"
#include "resiprocate/dum/SubscriptionCreator.hxx"


using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM


ClientSubscription::ClientSubscription(DialogUsageManager& dum, Dialog& dialog, const SipMessage& request)
   : BaseSubscription(dum, dialog, request),
     mOnNewSubscriptionCalled(mEventType == "refer"),
     mEnded(false)
{
   mLastRequest = request;
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
   ClientSubscriptionHandler* handler = mDum.getClientSubscriptionHandler(mEventType);
   assert(handler);   

   // asserts are checks the correctness of Dialog::dispatch
   if (msg.isRequest() )
   {
      assert( msg.header(h_RequestLine).getMethod() == NOTIFY );
      mDialog.makeResponse(mLastResponse, msg, 200);
      send(mLastResponse);

      if (!mOnNewSubscriptionCalled)
      {
         InfoLog (<< "[ClientSubscription] " << mLastRequest.header(h_To));
         handler->onNewSubscription(getHandle(), msg);
         mOnNewSubscriptionCalled = true;
      }         
      int expires = 0;      
      //default to 60 seconds so non-compliant endpoints don't result in leaked usages
      if (msg.exists(h_SubscriptionState) && msg.header(h_SubscriptionState).exists(p_expires))
      {
         expires = msg.header(h_SubscriptionState).param(p_expires);
      }
      else
      {
         expires = 60;
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
                     handler->onTerminated(getHandle(), msg);
                     delete this;
                  }
               }
            }
            else
            {
               handler->onTerminated(getHandle(), msg);
               delete this;
            }
         }
         else
         {
            handler->onTerminated(getHandle(), msg);
            delete this;
         }
         return;
      }

      SubscriptionCreator* creator = dynamic_cast<SubscriptionCreator*> (mDialog.mDialogSet.getCreator());
      
      int refreshInterval = 0;
      if (expires)
      {      
         if (creator && creator->hasRefreshInterval() && creator->getRefreshInterval() <  expires)
         {
            refreshInterval = creator->getRefreshInterval();
         }
         else
         {
            refreshInterval = Helper::aBitSmallerThan((unsigned long)expires);
         }
      }
         
      if (!mEnded && msg.header(h_SubscriptionState).value() == "active")
      {
         if (refreshInterval)
         {
            unsigned long t = refreshInterval;            
            mDum.addTimer(DumTimeout::Subscription, t, getBaseHandle(), ++mTimerSeq);
            DebugLog (<< "[ClientSubscription] reSUBSCRIBE in " << t);
         }
         
         handler->onUpdateActive(getHandle(), msg);
      }
      else if (!mEnded && msg.header(h_SubscriptionState).value() == "pending")
      {
         if (refreshInterval)
         {
            unsigned long t = refreshInterval;
            mDum.addTimer(DumTimeout::Subscription, t, getBaseHandle(), ++mTimerSeq);
            DebugLog (<< "[ClientSubscription] reSUBSCRIBE in " << t);
         }

         handler->onUpdatePending(getHandle(), msg);
      }
      else if (msg.header(h_SubscriptionState).value() == "terminated")
      {
         handler->onTerminated(getHandle(), msg);
         DebugLog (<< "[ClientSubscription] " << mLastRequest.header(h_To) << "[ClientSubscription] Terminated");                   
         delete this;
         return;
      }
      else if (!mEnded)
      {
         handler->onUpdateExtension(getHandle(), msg);         
      }
   }
   else
   {
      // !jf! might get an expiration in the 202 but not in the NOTIFY - we're going
      // to ignore this case
      if (msg.header(h_StatusLine).statusCode() == 481)
      {
         InfoLog (<< "Received 481 to SUBSCRIBE, reSUBSCRIBEing (presence server probably restarted) " 
                  << mDialog.mRemoteTarget);
         handler->onTerminated(getHandle(), msg);
         SipMessage& sub = mDum.makeSubscription(mDialog.mRemoteTarget, getEventType());
         mDum.send(sub);
         
         delete this;
         return;
      }
      else if (msg.header(h_StatusLine).statusCode() >= 300)
      {
         handler->onTerminated(getHandle(), msg);
         delete this;
         return;
      }
   }
}

void 
ClientSubscription::dispatch(const DumTimeout& timer)
{
   if (timer.seq() == mTimerSeq)
   {
      requestRefresh();
   }
}

void  
ClientSubscription::requestRefresh()
{
   if (!mEnded)
   {
      mDialog.makeRequest(mLastRequest, SUBSCRIBE);
      //!dcm! -- need a mechanism to retrieve this for the event package...part of
      //the map that stores the handlers, or part of the handler API
      //mLastRequest.header(h_Expires).value() = 300;   
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
