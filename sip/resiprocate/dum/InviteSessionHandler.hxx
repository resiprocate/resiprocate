#if !defined(RESIP_INVITESESSIONHANDLER_HXX)
#define RESIP_INVITESESSIONHANDLER_HXX

/** @file InvSessionHandler.hxx
 *  
 */

namespace resip
{

class InviteSessionHandler
{
   public:
      /// called when an initial INVITE arrives 
      virtual void onNewInvSession(ServerInviteSession::Handle&, const SipMessage& msg)=0;
      virtual void onNewInvSession(ClientInviteSession::Handle&, const SipMessage& msg)=0;
      
      /// called when some state in the Dialog changes - typically remoteURI
      virtual void onDialogModified(InviteSession::Handle&, const SipMessage& msg)=0;

      /// called when INFO message is received 
      virtual void onInfo(InviteSession::Handle&, const SipMessage& msg)=0;

      /// called when an REFER messages is received 
      virtual void onRefer(InviteSession::Handle&, const SipMessage& msg)=0;

      /// called when dialog enters the Early state - typically after getting 100
      virtual void onEarly(ClientInviteSession::Handle&, const SipMessage& msg)=0;

      /** called when an SDP answer is received - has nothing to do with user
          answering the call */ 
      virtual void onAnswer(ClientInviteSession::Handle&, const SipMessage& msg)=0;

      /// called when an SDP offer is received - must send an answer soon after this
      virtual void onOffer(InviteSession::Handle&, const SipMessage& msg)=0;

      /// called when dialog enters connected state (after getting a 200)
      virtual void onConnected(ClientInviteSession::Handle&, const SipMessage& msg)=0;

      /// called if an offer in a UPDATE or re-INVITE was rejected - not real
      /// useful 
      virtual void onOfferRejected(InviteSession::Handle&, const SipMessage& msg)=0;

      /// called when an dialog enters the terminated state - this can happen
      /// after getting a BYE, Cancel, or 4xx,5xx,6xx response
      virtual void onTerminated(InviteSession::Handle&, const SipMessage& msg)=0;
};

}

#endif
