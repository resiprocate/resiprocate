#if !defined(RESIP_SERVERSUBSCRIPTION_HXX)
#define RESIP_SERVERSUBSCRIPTION_HXX

#include "resip/stack/Helper.hxx"
#include "resip/dum/BaseSubscription.hxx"

namespace resip
{

class DialogUsageManager;
class ServerSubscriptionHandler;

//!dcm! -- no Subscription State expires parameter generation yet. 
class ServerSubscription : public BaseSubscription 
{
   public:
      typedef Handle<ServerSubscription> ServerSubscriptionHandle;

      ServerSubscription(const ServerSubscription&) = delete;
      ServerSubscription(ServerSubscription&&) = delete;

      ServerSubscription& operator=(const ServerSubscription&) = delete;
      ServerSubscription& operator=(ServerSubscription&&) = delete;

      ServerSubscriptionHandle getHandle() const;

      const Data& getSubscriber() const noexcept { return mSubscriber; }
      uint32_t getTimeLeft();
     
      //only 200 and 202 are permissible.  SubscriptionState is not affected.
      //currently must be called for a refresh as well as initial creation.
      std::shared_ptr<SipMessage> accept(int statusCode = 202);
      std::shared_ptr<SipMessage> reject(int responseCode);
      bool isResponsePending() { return mLastResponse.get() != 0; } // Note: mLastResponse is cleared out when send is called

      //used to accept a refresh when there is no useful state to convey to the
      //client     
      std::shared_ptr<SipMessage> neutralNotify();
      
      void setSubscriptionState(SubscriptionState state);

      std::shared_ptr<SipMessage> update(const Contents* document);
      void end(TerminateReason reason, const Contents* document = 0, int retryAfter = 0);

      void end() override;
      void send(std::shared_ptr<SipMessage> msg) override;

      void sendCommand(std::shared_ptr<SipMessage> msg) override
      {
         BaseSubscription::sendCommand(msg);
      }

//      void setTerminationState(TerminateReason reason);
//      void setCurrentEventDocument(const Contents* document);

      void dispatch(const SipMessage& msg) override;
      void dispatch(const DumTimeout& timer) override;

      EncodeStream& dump(EncodeStream& strm) const override;

   protected:
      virtual ~ServerSubscription();
      virtual void onReadyToSend(SipMessage& msg) override;
      virtual void flowTerminated();
      
   private:
      friend class Dialog;
      
      ServerSubscription(DialogUsageManager& dum, Dialog& dialog, const SipMessage& req);

      void makeNotifyExpires();
      void makeNotify();    
      
      bool shouldDestroyAfterSendingFailure(const SipMessage& msg);      

      void terminateSubscription(ServerSubscriptionHandler* handler);

      Data mSubscriber;
      uint32_t mExpires;

      uint64_t mAbsoluteExpiry;      
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
