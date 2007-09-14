#if !defined(RESIP_DialogEventStateManager_HXX)
#define RESIP_DialogEventStateManager_HXX

#include "resip/dum/DialogEventInfo.hxx"
#include "resip/dum/DialogEventHandler.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/DialogSet.hxx"

namespace resip
{

/**
 * Implements the FSM for dialog state as spec'd in RFC 4235.
 * Called from DialogSet, ClientInviteSession, and ServerInviteSession.
 */
class DialogEventStateManager
{
public:
   DialogEventStateManager();

   void onTryingUas(Dialog& dialog, const SipMessage& invite);
   void onTryingUac(DialogSet& dialogSet, const SipMessage& invite);
   void onProceedingUac(const DialogSet& dialogSet, const SipMessage& response);
   // !jjg! do we need onProceedingUas? will we ever call it?
   void onEarlyUac(const Dialog& dialog, InviteSessionHandle is);
   void onEarlyUas(const Dialog& dialog, InviteSessionHandle is);
   void onConfirmed(const Dialog& dialog, InviteSessionHandle is);
   void onTerminated(const Dialog& dialog, const SipMessage& msg, InviteSessionHandler::TerminatedReason reason);

   const std::map<DialogId, DialogEventInfo>& getDialogEventInfos() const;

private:
   // !jjg! we'll only have the DialogSetId if we aren't yet in the 'early' state;
   // once we get to early, we'll remove the DialogSetId in favour of the DialogId

   // !jjg! note that a comparator is NOT needed...  the DialogId will be matched exactly,
   // all of the time.  before the Early state, we will always search for DialogId with
   // remote tag set to Data::Empty
   std::map<DialogId, DialogEventInfo> mDialogIdToEventInfo;

   DialogEventHandler* mDialogEventHandler;
};

}
#endif
