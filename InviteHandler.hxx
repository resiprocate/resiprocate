#if !defined(RESIP_INVITEHANDLER_HXX)
#define RESIP_INVITEHANDLER_HXX

/** @file InviteHandler.hxx
 *  
 */

namespace resip
{

class InviteHandler
{
   public:
      virtual void onDialogModified(ClientInvSession&, const SipMessage& msg)=0;
      virtual void onInfo(ClientInvSession&, const SipMessage& msg)=0;
      virtual void onRefer(ClientInvSession&, const SipMessage& msg)=0;
      virtual void onEarly(ClientInvSession&, const SipMessage& msg)=0;
      virtual void onAnswer(ClientInvSession&, const SipMessage& msg)=0;
      virtual void onOffer(ClientInvSession&, const SipMessage& msg)=0;
      virtual void onConnected(ClientInvSession&, const SipMessage& msg)=0;
      virtual void onTerminated(ClientInvSession&, const SipMessage& msg)=0;
};

}

#endif
