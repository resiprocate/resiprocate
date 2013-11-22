#if !defined(RESIP_CLIENTINVITESESSION_HXX)
#define RESIP_CLIENTINVITESESSION_HXX

#include "resip/dum/InviteSession.hxx"
#include "resip/dum/Handles.hxx"

namespace resip
{
class SipMessage;
class Contents;

class ClientInviteSession : public InviteSession
{
   public:
      ClientInviteSession(DialogUsageManager& dum,
                          Dialog& dialog,
                          SharedPtr<SipMessage> request,
                          const Contents* initialOffer,
                          DialogUsageManager::EncryptionLevel level,
                          ServerSubscriptionHandle serverSub = ServerSubscriptionHandle::NotValid());

      ClientInviteSessionHandle getHandle();

   public:
      /** Called to set the offer that will be used in the next message that
          sends an offer.  For a UAC in an early dialog, this can be used to send
          an UPDATE request with an offer. */
      virtual void provideOffer (const Contents& offer);
      virtual void provideOffer(const Contents& offer, DialogUsageManager::EncryptionLevel level, const Contents* alternative);

      /** Similar to provideOffer - called to set the answer to be signalled to
          the peer. May result in message being sent synchronously depending on
          the state. */
      virtual void provideAnswer (const Contents& answer);

      /** Makes the specific dialog end. Will send a BYE (not a CANCEL) */
      virtual void end(const Data& userReason);
      virtual void end(EndReason reason);
      virtual void end();

      /** Rejects an offer at the SIP level.  For a UAC in an early dialog 
          this typically only makes sense, when rejecting an UPDATE request
          that contains an offer in an early dialog. */
      virtual void reject (int statusCode, WarningCategory *warning = 0);

      const Contents& getEarlyMedia() const;
      
   private:
      virtual void dispatch(const SipMessage& msg);
      virtual void dispatch(const DumTimeout& timer);

      void dispatchStart (const SipMessage& msg);
      void dispatchEarly (const SipMessage& msg);
      void dispatchEarlyWithOffer (const SipMessage& msg);
      void dispatchEarlyWithAnswer (const SipMessage& msg);
      void dispatchAnswered (const SipMessage& msg);
      void dispatchSentUpdateEarly (const SipMessage& msg);
      void dispatchSentUpdateEarlyGlare (const SipMessage& msg);
      void dispatchReceivedUpdateEarly (const SipMessage& msg);
      void dispatchSentAnswer (const SipMessage& msg);
      void dispatchQueuedUpdate (const SipMessage& msg);
      void dispatchCancelled (const SipMessage& msg);

      void handleRedirect (const SipMessage& msg);
      void handleProvisional (const SipMessage& msg);
      void handleFinalResponse (const SipMessage& msg);
      void handle1xxOffer (const SipMessage& msg, const Contents& offer);
      void handle1xxAnswer (const SipMessage& msg, const Contents& answer);
      void sendPrackIfNeeded(const SipMessage& msg);
      void sendPrack(const Contents& offerAnswer, DialogUsageManager::EncryptionLevel encryptionLevel);
      
      // Called by the DialogSet (friend) when the app has CANCELed the request
      void cancel();

      // Called by the DialogSet when it receives a 2xx response
      void onForkAccepted();

      bool isBadRseq(const SipMessage& msg);
   private:
      void startCancelTimer();
      void startStaleCallTimer();
      void sendSipFrag(const SipMessage& response);

      void onConnectedAspect(ClientInviteSessionHandle h, const SipMessage& msg);
      void onProvisionalAspect(ClientInviteSessionHandle c, const SipMessage& msg);
      void onFailureAspect(ClientInviteSessionHandle c, const SipMessage& msg);

      std::auto_ptr<Contents> mEarlyMedia;

      RAckCategory mRelRespInfo;
      unsigned int mStaleCallTimerSeq;
      unsigned int mCancelledTimerSeq;
      ServerSubscriptionHandle mServerSub;
      bool mAllowOfferInPrack;

      // disabled
      ClientInviteSession(const ClientInviteSession&);
      ClientInviteSession& operator=(const ClientInviteSession&);

      friend class Dialog;
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
