#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/ClientSubscription.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/SubscriptionHandler.hxx"

using namespace resip;

ClientSubscription::ClientSubscription(DialogUsageManager& dum, Dialog& dialog, const SipMessage& request)
   : BaseSubscription(dum, dialog, request)
{
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
//
// move check to dialog
//       if (!msg.exists(h_SubscriptionState))
//       {         
//          //!dcm! -- appropriate 4xx response?
//          return;         
//       }

      mDialog.makeResponse(mLastResponse, msg, 200);
      send(mLastResponse);

      if (msg.header(h_SubscriptionState).value() == "active")
      {
         handler->onUpdateActive(getHandle(), msg);
      }
      else if (msg.header(h_SubscriptionState).value() == "pending")
      {
         handler->onUpdatePending(getHandle(), msg);
      }
      else if (msg.header(h_SubscriptionState).value() == "terminated")
      {
         handler->onTerminated(getHandle(), msg);
         delete this;
         return;
      }
      else
      {
         handler->onUpdateExtension(getHandle(), msg);         
      }
   }
   else
   {
      int code = msg.header(h_StatusLine).statusCode();
      if(code > 100 && code < 300)
      {
         if(msg.exists(h_Expires))
         {
            unsigned long t = Helper::aBitSmallerThan((unsigned long)(mLastRequest.header(h_Expires).value()));
            mDum.addTimer(DumTimeout::Subscription, t, getBaseHandle(), ++mTimerSeq);
         }
      }
      else
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
   mLastRequest.header(h_CSeq).sequence()++;
   //!dcm! -- need a mechanism to retrieve this for the event package...part of
   //the map that stores the handlers, or part of the handler API
   mLastRequest.header(h_Expires).value() = 300;   
   send(mLastRequest);
}

void  
ClientSubscription::end()
{
   mLastRequest.header(h_CSeq).sequence()++;
   mLastRequest.header(h_Expires).value() = 0;   
   send(mLastRequest);
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
