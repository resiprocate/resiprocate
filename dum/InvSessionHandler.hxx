#if !defined(RESIP_INVSESSIONHANDLER_HXX)
#define RESIP_INVSESSIONHANDLER_HXX

/** @file InvSessionHandler.hxx
 *  
 */

namespace resip
{

class InvSessionHandler
{
   public:
      /// called when an initial INVITE arrives 
      virtual void onNewInvSession(ClientInvSession&, const SipMessage& msg)=0;

      /// called when some state in the Dialog changes - typically remoteURI
      virtual void onDialogModified(ClientInvSession&, const SipMessage& msg)=0;

      /// called when INFO message is received 
      virtual void onInfo(ClientInvSession&, const SipMessage& msg)=0;

      /// called when an REFER messages is received 
      virtual void onRefer(ClientInvSession&, const SipMessage& msg)=0;

      /// called when dialog enters the Early state - typically after getting 100
      virtual void onEarly(ClientInvSession&, const SipMessage& msg)=0;

      /** called when an SDP answer is received - has nothing to do with user
          answering the call */ 
      virtual void onAnswer(ClientInvSession&, const SipMessage& msg)=0;

      /// called when an SDP offer is received - must send an answer soon after this
      virtual void onOffer(ClientInvSession&, const SipMessage& msg)=0;

      /// called when dialog enters connected state (after getting a 200)
      virtual void onConnected(ClientInvSession&, const SipMessage& msg)=0;

      /// called if an offer in a UPDATE or re-INVITE was rejected - not real
      /// useful 
      virtual void onOfferRejected(ClientInvSession&, const SipMessage& msg)=0;

      /// called when an dialog enters the terminated state - this can happen
      /// after getting a BYE, Cancel, or 4xx,5xx,6xx response
      virtual void onTerminated(ClientInvSession&, const SipMessage& msg)=0;


};

}

#endif
