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
      /// called when an initial INVITE arrives 
      virtual void onNewSession(ClientInviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)=0;
      virtual void onNewSession(ServerInviteSessionHandle, const SipMessage& msg)=0;

      // Received a failure response from UAS
      virtual void onFailure(ClientInviteSessionHandle, const SipMessage& msg)=0;
      
      /// called when dialog enters the Early state - typically after getting 100
      virtual void onEarlyMedia(ClientInviteSessionHandle, const SipMessage&, const SdpContents*)=0;

      /// called when dialog enters the Early state - typically after getting 100
      virtual void onProvisional(ClientInviteSessionHandle, const SipMessage&)=0;

      /// called when dialog enters connected state (after getting a 200)
      virtual void onConnected(ClientInviteSessionHandle, const SipMessage& msg)=0;
      
      // UAC gets no final response within the stale call timeout(currently 3 minutes)
      virtual void onStaleCallTimeout(ClientInviteSessionHandle)=0;

      /// called when an dialog enters the terminated state - this can happen
      /// after getting a BYE, Cancel, or 4xx,5xx,6xx response
      virtual void onTerminated(InviteSessionHandle, const SipMessage& msg)=0;

      // called to allow app to adorn a message. default is to send immediately
      virtual void onReadyToSend(InviteSessionHandle, const SipMessage& msg);

      /** called when an SDP answer is received - has nothing to do with user
          answering the call */ 
      virtual void onAnswer(InviteSessionHandle, const SipMessage& msg, const SdpContents*)=0;

      /// called when an SDP offer is received - must send an answer soon after this
      virtual void onOffer(InviteSessionHandle, const SipMessage& msg, const SdpContents*)=0;
      
      
      /// called if an offer in a UPDATE or re-INVITE was rejected - not real
      /// useful 
      virtual void onOfferRejected(InviteSessionHandle, const SipMessage& msg)=0;

      //!dcm! -- timer B handling?--timer B for re-invite?
      
      /// callebranch/sip/resiprocate/dum/d when some state in the Dialog changes - typically remoteURI
      virtual void onDialogModified(InviteSessionHandle, const SipMessage& msg)=0;

      /// called when INFO message is received 
      virtual void onInfo(InviteSessionHandle, const SipMessage& msg)=0;

      /// called when an REFER messages is received 
      virtual void onRefer(InviteSessionHandle, const SipMessage& msg)=0;
};

}

#endif
