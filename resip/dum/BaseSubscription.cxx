#include "resip/dum/BaseSubscription.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

BaseSubscription::BaseSubscription(DialogUsageManager& dum, Dialog& dialog, const SipMessage& request) :
   DialogUsage(dum, dialog),
   mSubDlgState(SubDlgInitial),
   mLastRequest(new SipMessage),
   mLastResponse(new SipMessage),
   mDocumentKey(request.header(h_RequestLine).uri().getAor()),
   mSubscriptionId(Data::Empty),
   mTimerSeq(0),
   mSubscriptionState(Invalid)
{
   if (request.exists(h_Event))
   {
      mEventType = request.header(h_Event).value();
      if (request.header(h_Event).exists(p_id))
      {
         mSubscriptionId = request.header(h_Event).param(p_id);
      }
      mLastRequest->header(h_Event) = request.header(h_Event);      
   }
   else if (request.header(h_RequestLine).method() == REFER
            || request.header(h_RequestLine).method() == NOTIFY) 
   {
      mEventType = "refer";
      mLastRequest->header(h_Event).value() = mEventType;      
   }
}

bool
BaseSubscription::matches(const SipMessage& msg)
{
   if (msg.isResponse() && mLastRequest.get() != 0 && msg.header(h_CSeq) == mLastRequest->header(h_CSeq))
   {
      return true;
   }
   else
   {
      if (msg.exists(h_Event))
      {
         return msg.header(h_Event).value() == mEventType 
            && ( !msg.header(h_Event).exists(p_id) || 
                 msg.header(h_Event).param(p_id) == mSubscriptionId);
      }
      else
      {
         return (mEventType == "refer" && 
                 Data(msg.header(h_CSeq).sequence()) == mSubscriptionId);      
      }
   }
}

BaseSubscription::~BaseSubscription()
{
}

SubscriptionState 
BaseSubscription::getSubscriptionState()
{
   return mSubscriptionState;
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
