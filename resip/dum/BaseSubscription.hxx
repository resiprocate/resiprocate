#if !defined(RESIP_BASESUBSCRIPTION_HXX)
#define RESIP_BASESUBSCRIPTION_HXX

#include "resip/dum/DialogUsage.hxx"
#include "resip/dum/SubscriptionState.hxx"
#include "resip/stack/SipMessage.hxx"

#include <memory>

namespace resip
{

class DialogUsageManager;

//!dcm! -- update contact in dialog if required

class BaseSubscription: public DialogUsage
{
   public:
      BaseSubscription(const BaseSubscription&) = delete;
      BaseSubscription(BaseSubscription&&) = delete;

      BaseSubscription& operator=(const BaseSubscription&) = delete;
      BaseSubscription& operator=(BaseSubscription&&) = delete;

      bool matches(const SipMessage& subOrNotify);
      const Data& getDocumentKey() const noexcept { return mDocumentKey; }
      const Data& getEventType() const noexcept { return mEventType; }
      const Data& getId() const noexcept { return mSubscriptionId; }

   protected:
      friend class Dialog;

      enum SubDlgState
      {
         SubDlgInitial,
         SubDlgEstablished,    
         SubDlgTerminating
      };
      
      // state of the dialog that carries the subscription
      // this is similar to the DialogSet::State, but different
      // set of states
      SubDlgState mSubDlgState;

      BaseSubscription(DialogUsageManager& dum, Dialog& dialog, const SipMessage& request);

      SubscriptionState getSubscriptionState() const noexcept;      

      virtual ~BaseSubscription() = default;

      std::shared_ptr<SipMessage> mLastRequest;
      std::shared_ptr<SipMessage> mLastResponse;
      
      Data mDocumentKey;
      Data mEventType;
      Data mSubscriptionId;
      unsigned int mTimerSeq;      
      SubscriptionState mSubscriptionState;
};
 
}

#endif

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
