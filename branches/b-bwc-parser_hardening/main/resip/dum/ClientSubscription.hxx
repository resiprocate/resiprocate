#if !defined(RESIP_CLIENTSUBSCRIPTION_HXX)
#define RESIP_CLIENTSUBSCRIPTION_HXX

#include <deque>
#include "resip/dum/BaseSubscription.hxx"

namespace resip
{

class DialogUsageManager;

//!dcm! -- update contact in dialog if required

class ClientSubscription: public BaseSubscription
{
   public:      
      ClientSubscription(DialogUsageManager& dum, Dialog& dialog, const SipMessage& request);

      typedef Handle<ClientSubscription> ClientSubscriptionHandle;
      ClientSubscriptionHandle getHandle();
      
      //.dcm. no adornment for ease of use, can add if there is a use case
      
      void acceptUpdate(int statusCode = 200);
      void rejectUpdate(int statusCode = 400, const Data& reasonPhrase = Data::Empty);
      
      void requestRefresh(int expires = -1);  // default to using original expires value (0 is not allowed - call end() instead)
      virtual void end();
      virtual std::ostream& dump(std::ostream& strm) const;

   protected:
      virtual ~ClientSubscription();
      virtual void dialogDestroyed(const SipMessage& msg);
      virtual void onReadyToSend(SipMessage& msg);
      virtual void send(SharedPtr<SipMessage> msg);

   private:
      friend class Dialog;
      friend class InviteSession;      

      class QueuedNotify
      {
         public:
            QueuedNotify(const SipMessage& notify, bool outOfOrder)
               : mNotify(notify), mOutOfOrder(outOfOrder) {}

            SipMessage& notify() { return mNotify; }
            bool outOfOrder() { return mOutOfOrder; }

         private:
            SipMessage mNotify;
            bool mOutOfOrder;
      };

      typedef std::deque<QueuedNotify*> NotifyQueue;
      NotifyQueue mQueuedNotifies;

      typedef std::vector<QueuedNotify*> Dustbin;
      Dustbin mDustbin;

      bool mOnNewSubscriptionCalled;
      //SipMessage mLastNotify;      
      bool mEnded;
      UInt64 mExpires;

      bool mRefreshing;
      bool mHaveQueuedRefresh;
      int mQueuedRefreshInterval;

      unsigned int mLargestNotifyCSeq;

      virtual void dispatch(const SipMessage& msg);
      virtual void dispatch(const DumTimeout& timer);

      void sendQueuedRefreshRequest();
      void processNextNotify();
      void processResponse(const SipMessage& response);
      void clearDustbin();
      
      // disabled
      ClientSubscription(const ClientSubscription&);
      ClientSubscription& operator=(const ClientSubscription&);
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
