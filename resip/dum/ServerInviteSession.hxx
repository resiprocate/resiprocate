#if !defined(RESIP_SERVERINVITESESSION_HXX)
#define RESIP_SERVERINVITESESSION_HXX

#include "resip/dum/InviteSession.hxx"
#include "resip/stack/SipMessage.hxx"

#include <deque>

namespace resip
{

class ServerInviteSession: public InviteSession
{
   public:
      typedef Handle<ServerInviteSession> ServerInviteSessionHandle;
      ServerInviteSessionHandle getHandle();

      /** send a 3xx */
      void redirect(const NameAddrs& contacts, int code=302);

      /** send a 1xx - provisional response.  If you set earlyFlag to true, and have provided
          an answer, then DUM will attach the answer to the provisional response, as an 
          early media indication */
      void provisional(int code=180, bool earlyFlag=true);
      
      /** Called to set the offer that will be used in the next message that
          sends an offer. If possible, this will synchronously send the
          appropriate request or response. In some cases, the UAS might have to
          call accept in order to cause the message to be sent.
          If sendOfferAtAccept is true, no UPDATE will be sent if media is negotiated reliable,
          it will be sent at accept */
      virtual void provideOffer(const Contents& offer);
      virtual void provideOffer(const Contents& offer, DialogUsageManager::EncryptionLevel level, const Contents* alternative);
      virtual void provideOffer(const Contents& offer, bool sendOfferAtAccept);
      virtual void provideOffer(const Contents& offer, DialogUsageManager::EncryptionLevel level,
                                const Contents* alternative, bool sendOfferAtAccept);

      /** Called to request that the far end provide an offer.  This will cause a 
          reinvite with no body to be sent.  */
      virtual void requestOffer();

      /** Similar to provideOffer - called to set the answer to be signalled to
          the peer. May result in message being sent synchronously depending on
          the state. */
      virtual void provideAnswer(const Contents& answer);

      /** Makes the specific dialog end. Will send a 480. */
      virtual void end(const Data& userReason);
      virtual void end(EndReason reason);
      virtual void end();

      /** Rejects an offer at the SIP level. So this can send a 488 to a
          reINVITE or UPDATE */
      virtual void reject(int statusCode, WarningCategory *warning = 0);

      /** accept an initial invite (2xx) - this is only applicable to the UAS */
      void accept(int statusCode=200);

      /**
       * Provide asynchronous method access by using command
       */
      void redirectCommand(const NameAddrs& contacts, int code=302);
      void provisionalCommand(int code=180, bool earlyFlag=true);
      void acceptCommand(int statusCode=200);

   private:
      friend class Dialog;

      virtual void dispatch(const SipMessage& msg);
      virtual void dispatch(const DumTimeout& timer);

      void dispatchStart(const SipMessage& msg);
      void dispatchOfferOrEarly(const SipMessage& msg);
      void dispatchAccepted(const SipMessage& msg);
      void dispatchWaitingToOffer(const SipMessage& msg);
      void dispatchWaitingToRequestOffer(const SipMessage& msg);
      void dispatchAcceptedWaitingAnswer(const SipMessage& msg);
      void dispatchFirstSentOfferReliable(const SipMessage& msg);
      void dispatchOfferReliableProvidedAnswer(const SipMessage& msg);
      void dispatchFirstSentAnswerReliable(const SipMessage& msg);
      void dispatchNoAnswerReliableWaitingPrack(const SipMessage& msg);
      void dispatchSentUpdate(const SipMessage& msg);
      void dispatchSentUpdateGlare(const SipMessage& msg);
      void dispatchSentUpdateAccepted(const SipMessage& msg);
      void dispatchReceivedUpdate(const SipMessage& msg);
      void dispatchReceivedUpdateWaitingAnswer(const SipMessage& msg);
      void dispatchWaitingToHangup(const SipMessage& msg);
      void dispatchNegotiatedReliable(const SipMessage& msg);

      void dispatchCancel(const SipMessage& msg);
      void dispatchBye(const SipMessage& msg);
      void dispatchUnknown(const SipMessage& msg);

      // utilities
      void startRetransmit1xxTimer();
      void startResubmit1xxRelTimer();
      void startRetransmit1xxRelTimer();
      void sendAccept(int code, Contents* offerAnswer); // sends 2xxI
      bool sendProvisional(int code, bool earlyFlag);  // returns true if sent reliably
      void queueResponse(int code, bool earlyFlag);
      void sendUpdate(const Contents& offerAnswer);
      bool handlePrack(const SipMessage& msg); // verify that prack matches our last send reliable 1xx
      void prackCheckQueue();                             // send a queued message after prack
      void updateCheckQueue();                            // send a queued message after update

      ServerInviteSession(DialogUsageManager& dum, Dialog& dialog, const SipMessage& msg);

      // disabled
      ServerInviteSession(const ServerInviteSession&);
      ServerInviteSession& operator=(const ServerInviteSession&);

      // stores the original request
      const SipMessage mFirstRequest;
      SharedPtr<SipMessage> m1xx; // for 1xx retransmissions
      unsigned long mCurrentRetransmit1xxSeq;
      
      // UAS Prack members
      unsigned int mLocalRSeq;
      SharedPtr<SipMessage> mUnacknowledgedReliableProvisional; // We won't send a new reliable provisional until the previous one is acknowledge - used for re-transmissions
      std::deque< std::pair<int, bool> > mQueuedResponses;
      bool mAnswerSentReliably;
      SharedPtr<SipMessage> mPrackWithOffer; // for 1xx retransmissions
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
