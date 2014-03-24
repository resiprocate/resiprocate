#if !defined(RESIP_INVITESESSIONHANDLER_HXX)
#define RESIP_INVITESESSIONHANDLER_HXX

#include "resip/dum/InviteSession.hxx"
#include "resip/dum/Handles.hxx"

namespace resip
{

class SipMessage;
class SdpContents;
class Contents;

/** 
    Base class for class InviteSessionHandler.  The application must override this class
    and provide an imlementation of the handlers below.  The class must then be set as
    an invite session handler on dum (DialogUsageManager::setInviteSessionHandler()).

    If you wish to handle invite session offer/answers using generic contents instead of
    SDP, then construct this class with a bool argument of true, and be sure to add 
    implementations of all of the handlers below that take a Contents parameter type
    (ie. onEarlyMedia, onOffer, onAnswer, and onRemoteAnswerChanged).  Overriding these 
    methods will cause their SdpContent counterparts to not be called.  You can then provide 
    a basic empty body implementation of these SdpContent versions of the callbacks.

    Example handler for generic (non-sdp only) invite sessions:
    class MyInviteSessionHandler : public InviteSessionHandler
    {
       MyInviteSessionHandler() : InviteSessionHandler(true) {};
       ...
       virtual void onOffer(InviteSessionHandle, const SipMessage& msg, const SdpContents&)
       {}  // No body required
       virtual void onOffer(InviteSessionHandle, const SipMessage& msg, const Contents&)
       {
          // Add handler body here
       }
       ...
    };
*/
class InviteSessionHandler
{
   public:
      InviteSessionHandler(bool genericOfferAnswer=false) : mGenericOfferAnswer(genericOfferAnswer) {}
      virtual ~InviteSessionHandler() {}
      virtual bool isGenericOfferAnswer() { return mGenericOfferAnswer; }

      /// called when an initial INVITE or the intial response to an outoing invite  
      virtual void onNewSession(ClientInviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)=0;
      virtual void onNewSession(ServerInviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)=0;

      /// Received a failure response from UAS
      virtual void onFailure(ClientInviteSessionHandle, const SipMessage& msg)=0;
      
      /// called when an in-dialog provisional response is received that contains a body
      virtual void onEarlyMedia(ClientInviteSessionHandle, const SipMessage&, const SdpContents&)=0;
      virtual void onEarlyMedia(ClientInviteSessionHandle, const SipMessage&, const Contents&);

      /// called when dialog enters the Early state - typically after getting 18x
      virtual void onProvisional(ClientInviteSessionHandle, const SipMessage&)=0;

      /// called when a dialog initiated as a UAC enters the connected state
      virtual void onConnected(ClientInviteSessionHandle, const SipMessage& msg)=0;

      /// called when a dialog initiated as a UAS enters the connected state
      virtual void onConnected(InviteSessionHandle, const SipMessage& msg)=0;

      /// called when ACK (with out an answer) is received for initial invite (UAS)
      virtual void onConnectedConfirmed(InviteSessionHandle, const SipMessage &msg);

      /// called when PRACK is received for a reliable provisional answer (UAS)
      virtual void onPrack(ServerInviteSessionHandle, const SipMessage &msg);

      /** UAC gets no final response within the stale call timeout (default is 3
       * minutes). This is just a notification. After the notification is
       * called, the InviteSession will then call
       * InviteSessionHandler::terminate() */
      virtual void onStaleCallTimeout(ClientInviteSessionHandle h);

      /** called when an early dialog decides it wants to terminate the
       * dialog. Default behavior is to CANCEL all related early dialogs as
       * well.  */
      virtual void terminate(ClientInviteSessionHandle h);
      
      /// called when an dialog enters the terminated state - this can happen
      /// after getting a BYE, Cancel, or 4xx,5xx,6xx response - or the session
      /// times out
      enum TerminatedReason
      {
         Error,
         Timeout, 
         Replaced,
         LocalBye,
         RemoteBye,
         LocalCancel,
         RemoteCancel,
         Rejected, //Only as UAS, UAC has distinct onFailure callback
         Referred
      };
            
      virtual void onTerminated(InviteSessionHandle, InviteSessionHandler::TerminatedReason reason, const SipMessage* related=0)=0;

      /// called when a fork that was created through a 1xx never receives a 2xx
      /// because another fork answered and this fork was canceled by a proxy. 
      virtual void onForkDestroyed(ClientInviteSessionHandle)=0;

      /// called when a 3xx with valid targets is encountered in an early dialog     
      /// This is different then getting a 3xx in onTerminated, as another
      /// request will be attempted, so the DialogSet will not be destroyed.
      /// Basically an onTermintated that conveys more information.
      /// checking for 3xx respones in onTerminated will not work as there may
      /// be no valid targets.
      virtual void onRedirected(ClientInviteSessionHandle, const SipMessage& msg)=0;

      /// called to allow app to adorn a message. default is to send immediately
      virtual void onReadyToSend(InviteSessionHandle, SipMessage& msg);

      /// called when an answer is received - has nothing to do with user
      /// answering the call 
      virtual void onAnswer(InviteSessionHandle, const SipMessage& msg, const SdpContents&)=0;
      // You should only override the following method if genericOfferAnswer is true
      virtual void onAnswer(InviteSessionHandle, const SipMessage& msg, const Contents&);

      /// called when an offer is received - must send an answer soon after this
      virtual void onOffer(InviteSessionHandle, const SipMessage& msg, const SdpContents&)=0;      
      // You should only override the following method if genericOfferAnswer is true
      virtual void onOffer(InviteSessionHandle, const SipMessage& msg, const Contents&);      

      /// called when a modified body is received in a 2xx response to a
      /// session-timer reINVITE. Under normal circumstances where the response
      /// body is unchanged from current remote body no handler is called
      virtual void onRemoteSdpChanged(InviteSessionHandle, const SipMessage& msg, const SdpContents&);
      // You should only override the following method if genericOfferAnswer is true
      virtual void onRemoteAnswerChanged(InviteSessionHandle, const SipMessage& msg, const Contents&);  

      /// Called when an error response is received for a reinvite-nobody request (via requestOffer)
      virtual void onOfferRequestRejected(InviteSessionHandle, const SipMessage& msg);

      /// called when an Invite w/out offer is sent, or any other context which
      /// requires an offer from the user
      virtual void onOfferRequired(InviteSessionHandle, const SipMessage& msg)=0;      
      
      /// called if an offer in a UPDATE or re-INVITE was rejected - not real
      /// useful. A SipMessage is provided if one is available
      virtual void onOfferRejected(InviteSessionHandle, const SipMessage* msg)=0;
      
      /// called when INFO message is received 
      /// the application must call acceptNIT() or rejectNIT()
      /// once it is ready for another message.
      virtual void onInfo(InviteSessionHandle, const SipMessage& msg)=0;

      /// called when response to INFO message is received 
      virtual void onInfoSuccess(InviteSessionHandle, const SipMessage& msg)=0;
      virtual void onInfoFailure(InviteSessionHandle, const SipMessage& msg)=0;

      /// called when MESSAGE message is received 
      virtual void onMessage(InviteSessionHandle, const SipMessage& msg)=0;

      /// called when response to MESSAGE message is received 
      virtual void onMessageSuccess(InviteSessionHandle, const SipMessage& msg)=0;
      virtual void onMessageFailure(InviteSessionHandle, const SipMessage& msg)=0;

      /// called when an REFER message is received.  The refer is accepted or
      /// rejected using the server subscription. If the offer is accepted,
      /// DialogUsageManager::makeInviteSessionFromRefer can be used to create an
      /// InviteSession that will send notify messages using the ServerSubscription
      virtual void onRefer(InviteSessionHandle, ServerSubscriptionHandle, const SipMessage& msg)=0;

      virtual void onReferNoSub(InviteSessionHandle, const SipMessage& msg)=0;

      /// called when an REFER message receives a failure response 
      virtual void onReferRejected(InviteSessionHandle, const SipMessage& msg)=0;

      /// called when an REFER message receives an accepted response 
      virtual void onReferAccepted(InviteSessionHandle, ClientSubscriptionHandle, const SipMessage& msg)=0;

      /// called when ACK is received
      virtual void onAckReceived(InviteSessionHandle, const SipMessage& msg);

      /// default behaviour is to send a BYE to end the dialog
      virtual void onAckNotReceived(InviteSessionHandle);

      /// UAC gets no final response within the stale re-invite timeout (default is 40
      /// seconds).  Default behaviour is to send a BYE to end the dialog.
      virtual void onStaleReInviteTimeout(InviteSessionHandle h);

      /// will be called if reINVITE or UPDATE in dialog fails
      virtual void onIllegalNegotiation(InviteSessionHandle, const SipMessage& msg);     

      /// will be called if Session-Timers are used and Session Timer expires
      /// default behaviour is to send a BYE to send the dialog
      virtual void onSessionExpired(InviteSessionHandle);

      /// Called when a TCP or TLS flow to the server has terminated.  This can be caused by socket
      /// errors, or missing CRLF keep alives pong responses from the server.
      //  Called only if clientOutbound is enabled on the UserProfile and the first hop server 
      /// supports RFC5626 (outbound).
      /// Default implementation is to do nothing
      virtual void onFlowTerminated(InviteSessionHandle);

   private:
	  bool mGenericOfferAnswer;
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
