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
      virtual void onDialogModified(ClientInvSession&, const SipMessage& msg)=0;
      virtual void onInfo(ClientInvSession&, const SipMessage& msg)=0;
      virtual void onRefer(ClientInvSession&, const SipMessage& msg)=0;
      virtual void onEarly(ClientInvSession&, const SipMessage& msg)=0;
      virtual void onAnswer(ClientInvSession&, const SipMessage& msg)=0;
      virtual void onOffer(ClientInvSession&, const SipMessage& msg)=0;
      virtual void onConnected(ClientInvSession&, const SipMessage& msg)=0;
      virtual void onOfferRejected(ClientInvSession&, const SipMessage& msg)=0;
      virtual void onTerminated(ClientInvSession&, const SipMessage& msg)=0;


};

}

#endif
