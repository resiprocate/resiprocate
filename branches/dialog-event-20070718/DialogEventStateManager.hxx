#if !defined(RESIP_DialogEventStateManager_HXX)
#define RESIP_DialogEventStateManager_HXX

#include "resip/dum/DialogEventInfo.hxx"
#include "resip/dum/DialogEventHandler.hxx"
//include "resip/dum/InviteSessionHandler.hxx"

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

   void onTryingUas(Dialog& dialog);
   void onTryingUac(DialogSet& dialogSet);
   void onProceedingUac(const DialogSet& dialogSet);
   void onEarlyUac(const Dialog& dialog, InviteSessionHandle is);
   void onEarlyUas(const Dialog& dialog, InviteSessionHandle is);
   void onConfirmed(const Dialog& dialog, InviteSessionHandle is);
   //void onTerminated(const Dialog& dialog, InviteSessionHandler::TerminatedReason reason);

private:
   // !jjg! we'll only have the DialogSetId if we aren't yet in the 'early' state;
   // once we get to early, we'll remove the DialogSetId in favour of the DialogId

   // !jjg! use a comparator to ensure that: R, C, L == C, L
   // use find explicitly (otherwise won't be able to access the key...)
   std::map<DialogId, DialogEventInfo> mDialogEventInfos;

   DialogEventHandler* mDialogEventHandler;
};

}
#endif
