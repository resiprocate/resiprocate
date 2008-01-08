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

      // .jjg. looks like we can have just one onEarly -- same logic void
      //onEarlyUac(const Dialog& dialog, InviteSessionHandle is); void
      //onEarlyUas(const Dialog& dialog, InviteSessionHandle is);
   //?dcm? how is direction determined when the onEarly is the first use of
   //this dialog?
      void onEarly(const Dialog& dialog, InviteSessionHandle is);
      
      void onConfirmed(const Dialog& dialog, InviteSessionHandle is);
      void onTerminated(const Dialog& dialog, const SipMessage& msg, InviteSessionHandler::TerminatedReason reason);
      
      //TODO - implement this
      void onTerminated(const DialogSet& dialogSet, const SipMessage& msg, InviteSessionHandler::TerminatedReason reason);
      
      
   // order by DialogSet, such that the following ordering occurs
   // DialogSetId          remoteTag
   //     a                  null
   //     a                   1
   //     a                   2
   //     b                   1
   //     b                   2
   class DialogIdComparator
   {
   public:
      bool operator()(const DialogId& x, const DialogId& y) const
      {
         if (x.getDialogSetId() == y.getDialogSetId())
         {
            return (x.getRemoteTag() < y.getRemoteTag());
         }
         return (x.getDialogSetId() < y.getDialogSetId());
      }
   };

   typedef std::vector<DialogEventInfo> DialogInfos;
   DialogInfos getDialogEventInfos() const;

private:
   DialogEventInfo* findOrCreateDialogInfo(const Dialog& dialog);

   void onTerminatedImpl(const DialogSetId& dialogSetId, const SipMessage& msg, InviteSessionHandler::TerminatedReason reason);

private:

   friend class DialogUsageManager;

   // !jjg! we'll only have the DialogSetId if we aren't yet in the 'early' state;
   // once we get to early, we'll remove the DialogSetId in favour of the DialogId
   std::map<DialogId, DialogEventInfo*, DialogIdComparator> mDialogIdToEventInfo;

   DialogEventHandler* mDialogEventHandler;
};

}
#endif
