#if !defined(RESIP_INVITESESSION_HXX)
#define RESIP_INVITESESSION_HXX

#include "resiprocate/dum/DialogUsage.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/SdpContents.hxx"

#include <map>

namespace resip
{

class SdpContents;

/// Base class for class ClientInviteSession and class ServerInviteSession.
/// Implements common attributes and behavior (i.e.t post connected) of the two classes.
class InviteSession : public DialogUsage
{
   public:
      /// Called to set the offer that will be used in the next messages that
      /// sends and offer. Does not send an offer
     virtual void provideOffer(const SdpContents& offer);

      /// Called to set the answer that will be used in the next messages that
      /// sends an offer. Does not send an answer
      virtual void provideAnswer(const SdpContents& answer);

      /// Makes the specific dialog end. Will send a BYE (not a CANCEL)
      virtual void end();

      /// Rejects an offer at the SIP level. So this can send a 488 to a
      /// reINVITE or UPDATE
      virtual void reject(int statusCode, WarningCategory *warning = 0);

      //accept a re-invite, etc.  Always 200?
      //this is only applicable to the UAS
      //virtual void accept(int statusCode=200);

      // will resend the current sdp in an UPDATE or reINVITE
      virtual void targetRefresh(const NameAddr& localUri);

      // Following methods are for sending requests within a dialog
      virtual void refer(const NameAddr& referTo);
      virtual void refer(const NameAddr& referTo, InviteSessionHandle sessionToReplace);
      virtual void info(const Contents& contents);

      const NameAddr& myAddr() const;
      const NameAddr& peerAddr() const;
      const SdpContents& getLocalSdp() const;
      const SdpContents& getRemoteSdp() const;

      bool isConnected() const;
      bool isTerminated() const;
      bool isEarly() const;
      
      virtual std::ostream& dump(std::ostream& strm) const;

      
      typedef enum
      {
         None, // means no Offer or Answer (may have SDP)
         Offer,
         Answer
      } OfferAnswerType;

      InviteSessionHandle getSessionHandle();

   protected:

      typedef enum
      {
         Undefined,  // Not used
         Connected,
         SentUpdate, // Sent an UPDATE
         SentUpdateGlare, // got a 491
         SentReinvite, // Sent a reINVITE
         SentReinviteGlare, // Got a 491
         ReceivedUpdate, // Received an UPDATE
         ReceivedReinvite, // Received a reINVITE
         ReceivedReinviteNoOffer, // Received a reINVITE with no offer
         Answered,
         WaitingToOffer,
         WaitingToTerminate,
         Terminated, // Ended. waiting to delete

         UAC_Start,
         UAC_Early,
         UAC_EarlyWithOffer,
         UAC_EarlyWithAnswer,
         UAC_Answered,
         UAC_SentUpdateEarly,
         UAC_SentUpdateConnected,
         UAC_ReceivedUpdateEarly,
         UAC_SentAnswer,
         UAC_QueuedUpdate,
         UAC_Cancelled,

         UAS_Start,
         UAS_Offer, 
         UAS_OfferProvidedAnswer,
         UAS_EarlyOffer,
         UAS_EarlyProvidedAnswer, 

         UAS_NoOffer, 
         UAS_ProvidedOffer, 
         UAS_EarlyNoOffer, 
         UAS_EarlyProvidedOffer, 
         UAS_Accepted, 

         UAS_AcceptedWaitingAnswer, 
         UAS_OfferReliable,
         UAS_NoOfferReliable,
         UAS_FirstSentOfferReliable,
         UAS_FirstEarlyReliable,
         UAS_EarlyReliable,
         UAS_SentUpdate,
         UAS_SentUpdateAccepted,
         UAS_ReceivedUpdate,
         UAS_ReceivedUpdateWaitingAnswer,
         UAS_WaitingToTerminate,
         UAS_WaitingToHangup
      } State;

      typedef enum
      {
         OnRedirect, // 3xx
         OnGeneralFailure, // 481 or 408
         OnInvite, // UAS
         OnInviteOffer,// UAS
         OnInviteReliableOffer, // UAS
         OnInviteReliable, // UAS
         OnCancel, // UAS
         OnBye, 
         On200Bye, 
         On1xx, // UAC
         On1xxEarly, // UAC
         On1xxOffer, // UAC
         On1xxAnswer, // UAC
         On2xx, 
         On2xxOffer,
         On2xxAnswer,
         On487Invite,
         On489Invite,
         On491Invite,
         OnInviteFailure,
         OnAck,
         OnAckAnswer,
         On200Cancel, // UAC
         OnCancelFailure, // UAC
         OnUpdate,
         OnUpdateRejected,
         On491Update,
         On489Update,
         On200Update,
         OnPrack, // UAS
         On200Prack, // UAC
         Unknown
      } Event;

      typedef enum
      {
         NitComplete,
         NitProceeding
      } NitState;

      InviteSession(DialogUsageManager& dum, Dialog& dialog);
      virtual ~InviteSession();

      virtual void dispatch(const SipMessage& msg);
      virtual void dispatch(const DumTimeout& timer);

      // Utility methods (one for each State)
      void dispatchConnected(const SipMessage& msg);
      void dispatchSentUpdate(const SipMessage& msg);
      void dispatchSentReinvite(const SipMessage& msg);
      void dispatchGlare(const SipMessage& msg);
      void dispatchReceivedUpdateOrReinvite(const SipMessage& msg);
      void dispatchAnswered(const SipMessage& msg);
      void dispatchWaitingToOffer(const SipMessage& msg);
      void dispatchWaitingToTerminate(const SipMessage& msg);
      void dispatchTerminated(const SipMessage& msg);

      void dispatchNotify(const SipMessage& msg);      
      void dispatchRefer(const SipMessage& msg);      

      void startRetransmit200Timer();
      void start491Timer();

      void handleSessionTimerResponse(const SipMessage& msg);
      void handleSessionTimerRequest(SipMessage &response, const SipMessage& request);

      static Data toData(State state);
      void transition(State target);

      std::auto_ptr<SdpContents> getSdp(const SipMessage& msg);
      bool isReliable(const SipMessage& msg);
      static std::auto_ptr<SdpContents> makeSdp(const SdpContents& sdp);
      static void setSdp(SipMessage& msg, const SdpContents& sdp);

      void storePeerCapabilities(const SipMessage& msg);
      bool updateMethodSupported() const;

      Tokens mPeerSupportedMethods;
      Tokens mPeerSupportedOptionTags;
      Mimes mPeerSupportedMimeTypes;
      Tokens mPeerSupportedEncodings;
      Tokens mPeerSupportedLanguages;

      Event toEvent(const SipMessage& msg, const SdpContents* sdp);
      
      State mState;
      NitState mNitState;

      std::auto_ptr<SdpContents> mCurrentLocalSdp;
      std::auto_ptr<SdpContents> mCurrentRemoteSdp;
      std::auto_ptr<SdpContents> mProposedLocalSdp;
      std::auto_ptr<SdpContents> mProposedRemoteSdp;

      SipMessage mLastSessionModification; // UPDATE or reINVITE
      SipMessage mInvite200; // 200 OK for reINVITE for retransmissions
      
      unsigned long mCurrentRetransmit200;

      // Session Timer settings
      int  mSessionInterval;
      bool mSessionRefresherUAS;
      int  mSessionTimerSeq;
      bool mSentRefer;
      
   private:
      friend class Dialog;
      friend class DialogUsageManager;

      // disabled
      InviteSession(const InviteSession&);
      InviteSession& operator=(const InviteSession&);

      // Utility methods for handling particular methods
      void dispatchOthers(const SipMessage& msg);
      void dispatchUnhandledInvite(const SipMessage& msg);
      void dispatchPrack(const SipMessage& msg);
      void dispatchCancel(const SipMessage& msg);
      void dispatchBye(const SipMessage& msg);
      void dispatchInfo(const SipMessage& msg);
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
