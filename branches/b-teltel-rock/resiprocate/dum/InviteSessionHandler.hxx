#if !defined(RESIP_INVITESESSIONHANDLER_HXX)
#define RESIP_INVITESESSIONHANDLER_HXX

#include "resiprocate/dum/InviteSession.hxx"
#include "resiprocate/dum/Handles.hxx"

namespace resip
{

class SdpContents;
class SipMessage;


class InviteSessionHandler
{
   public:
      /// called when an initial INVITE or the intial response to an outoing invite  
      virtual void onNewSession(ClientInviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)=0;
      virtual void onNewSession(ServerInviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)=0;

      // Received a failure response from UAS
      virtual void onFailure(ClientInviteSessionHandle, const SipMessage& msg)=0;
      
      /// called when dialog enters the Early state - typically after getting 100
      virtual void onEarlyMedia(ClientInviteSessionHandle, const SipMessage&, const SdpContents*)=0;

      /// called when dialog enters the Early state - typically after getting 18x
      virtual void onProvisional(ClientInviteSessionHandle, const SipMessage&)=0;

      /// called when a dialog initiated as a UAC enters the connected state
      virtual void onConnected(ClientInviteSessionHandle, const SipMessage& msg)=0;

      // called when a dialog initiated as a UAS enters the connected state
      virtual void onConnected(InviteSessionHandle, const SipMessage& msg)=0;

      // UAC gets no final response within the stale call timeout (default is 1 hour)
      // expect an onTerminated after this
      virtual void onStaleCallTimeout(ClientInviteSessionHandle)=0;

      virtual void onForkDestroyed(ClientInviteSessionHandle)=0;

      /// called when an dialog enters the terminated state - this can happen
      /// after getting a BYE, Cancel, or 4xx,5xx,6xx response
      virtual void onTerminated(InviteSessionHandle, const SipMessage& msg)=0;

      /// called when a 3xx with valid targets is encountered in an early dialog     
      /// This is different then getting a 3xx in onTerminated, as another
      /// request will be attempted, so the DialogSet will not be destroyed.
      /// Basically an onTermintated that conveys more information.
      /// checking for 3xx respones in onTerminated will not work as there may
      /// be no valid targets.
      virtual void onRedirected(ClientInviteSessionHandle, const SipMessage& msg)=0;

      // called to allow app to adorn a message. default is to send immediately
      virtual void onReadyToSend(InviteSessionHandle, SipMessage& msg);

      /** called when an SDP answer is received - has nothing to do with user
          answering the call */ 
      virtual void onAnswer(InviteSessionHandle, const SipMessage& msg, const SdpContents*)=0;

      /// called when an SDP offer is received - must send an answer soon after this
      virtual void onOffer(InviteSessionHandle, const SipMessage& msg, const SdpContents*)=0;      

      //called when an Invite w/out SDP is sent, or any other context which
      //requires an SDP offer from the user
      virtual void onOfferRequired(InviteSessionHandle, const SipMessage& msg)=0;      
      
      /// called if an offer in a UPDATE or re-INVITE was rejected - not real
      /// useful 
      virtual void onOfferRejected(InviteSessionHandle, const SipMessage& msg)=0;
      
      /// called when some state in the Dialog changes - typically remoteURI, or
      /// a re-invite
      virtual void onDialogModified(InviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)=0;

      /// called when INFO message is received 
      virtual void onInfo(InviteSessionHandle, const SipMessage& msg)=0;

      /// called when response to INFO message is received 
      virtual void onInfoSuccess(InviteSessionHandle, const SipMessage& msg)=0;
      virtual void onInfoFailure(InviteSessionHandle, const SipMessage& msg)=0;

      /// called when an REFER message is received.  The refer is accepted or
      /// rejected using the server subscription. If the offer is accepted,
      /// DialogUsageManager::makeInviteSessionFromRefer can be used to create an
      /// InviteSession that will send notify messages using the ServerSubscription
      virtual void onRefer(InviteSessionHandle, ServerSubscriptionHandle, const SipMessage& msg)=0;

      /// called when an REFER message receives a failure response 
      virtual void onReferRejected(InviteSessionHandle, const SipMessage& msg)=0;

      //default behaviour is to send a BYE to end the dialog, msg is the 
      //2xx that was being retransmitted
      virtual void onAckNotReceived(InviteSessionHandle, const SipMessage& msg);

      virtual void onIllegalNegotiation(InviteSessionHandle, const SipMessage& msg);     

};

}

#endif
