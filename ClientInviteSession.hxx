#if !defined(RESIP_CLIENTINVITESESSION_HXX)
#define RESIP_CLIENTINVITESESSION_HXX

#include "resiprocate/dum/InviteSession.hxx"
#include "resiprocate/dum/Handles.hxx"

namespace resip
{
class SipMessage;
class SdpContents;

class ClientInviteSession : public InviteSession
{
   public:
      ClientInviteSession(DialogUsageManager& dum,
                          Dialog& dialog,
                          const SipMessage& request,
                          const SdpContents* initialOffer,
                          ServerSubscriptionHandle serverSub = ServerSubscriptionHandle::NotValid());

      ClientInviteSessionHandle getHandle();

    // !kh! ==========
    public:
        //  !kh! These comments are copy-and-paste-ed, not sure if they
        //  are correct here.

        /// Called to set the offer that will be used in the next messages that
        /// sends and offer. Does not send an offer
        virtual void provideOffer (const SdpContents& offer);

        /// Called to set the answer that will be used in the next messages that
        /// sends an offer. Does not send an answer
        virtual void provideAnswer (const SdpContents& answer);

        /// Makes the specific dialog end. Will send a BYE (not a CANCEL)
        virtual void end ();

        /// Rejects an offer at the SIP level. So this can send a 488 to a
        /// reINVITE or UPDATE
        virtual void reject (int statusCode);

        //accept a re-invite, etc.  Always 200?
        //this is only applicable to the UAS
        //virtual void accept (int statusCode=200);

        // will resend the current sdp in an UPDATE or reINVITE
        virtual void targetRefresh (const NameAddr& localUri);

        // Following methods are for sending requests within a dialog
        virtual void refer (const NameAddr& referTo);
        virtual void refer (const NameAddr& referTo, InviteSessionHandle sessionToReplace);
        virtual void info (const Contents& contents);
        void cancel ();


    private:

        void dispatchStart (const SipMessage& msg, const SdpContents* sdp);
        void dispatchEarly (const SipMessage& msg, const SdpContents* sdp);
        void dispatchEarlyWithOffer (const SipMessage& msg, const SdpContents* sdp);
        void dispatchEarlyWithAnswer (const SipMessage& msg, const SdpContents* sdp);
        void dispatchWaitingForAnswerFromApp (const SipMessage& msg, const SdpContents* sdp);
        void dispatchConnected (const SipMessage& msg, const SdpContents* sdp);
        void dispatchTerminated (const SipMessage& msg, const SdpContents* sdp);
        void dispatchSentUpdateEarly (const SipMessage& msg, const SdpContents* sdp);
        void dispatchReceivedUpdateEarly (const SipMessage& msg, const SdpContents* sdp);
        void dispatchPrackAnswerWait (const SipMessage& msg, const SdpContents* sdp);
        void dispatchCanceled (const SipMessage& msg, const SdpContents* sdp);

        enum State
        {
            UAC_Start,
            UAC_Early,
            UAC_EarlyWithOffer,
            UAC_EarlyWithAnswer,
            UAC_WaitingForAnswerFromApp,
            UAC_Terminated,
            UAC_SentUpdateEarly,
            UAC_ReceivedUpdateEarly,
            UAC_PrackAnswerWait,
            UAC_Canceled,
        };

    // !kh! ==========

   private:
      void redirected(const SipMessage& msg);

      // Called by the DialogSet (friend) when the app has CANCELed the request
      virtual void cancel();

      friend class Dialog;

      virtual void dispatch(const SipMessage& msg);
      virtual void dispatch(const DumTimeout& timer);

      void sendSipFrag(const SipMessage& response);
      void handlePrackResponse(const SipMessage& response);
      void sendPrack(const SipMessage& response);
//      void sendAck(const SipMessage& ok);


      int lastReceivedRSeq;
      int lastExpectedRSeq;
      int mStaleCallTimerSeq;

      ServerSubscriptionHandle mServerSub;

      // disabled
      ClientInviteSession(const ClientInviteSession&);
      ClientInviteSession& operator=(const ClientInviteSession&);
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
