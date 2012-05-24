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
   typedef std::vector<DialogEventInfo> DialogEventInfos;
   DialogEventInfos getDialogEventInfo() const;

   virtual ~DialogEventStateManager();

private:
      DialogEventStateManager();
      
      void onTryingUas(Dialog& dialog, const SipMessage& invite);
      void onTryingUac(DialogSet& dialogSet, const SipMessage& invite);
      void onProceedingUac(const DialogSet& dialogSet, const SipMessage& response);

   //?dcm? how is direction determined when the onEarly is the first use of
   //this dialog?
      void onEarly(const Dialog& dialog, InviteSessionHandle is);
      
      void onConfirmed(const Dialog& dialog, InviteSessionHandle is);
      void onTerminated(const Dialog& dialog, const SipMessage& msg, InviteSessionHandler::TerminatedReason reason);
      
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

   DialogEventInfo* findOrCreateDialogInfo(const Dialog& dialog);

   void onDialogSetTerminatedImpl(const DialogSetId& dialogSetId, const SipMessage& msg, InviteSessionHandler::TerminatedReason reason);
   TerminatedDialogEvent* onDialogTerminatedImpl(DialogEventInfo* eventInfo, InviteSessionHandler::TerminatedReason reason, 
                                                 int responseCode = 0, Uri* remoteTarget = NULL);

   static int getResponseCode(const SipMessage& msg);
   static Uri* getFrontContact(const SipMessage& msg);

private:

   friend class DialogUsageManager;
   friend class ServerInviteSession;
   friend class ClientInviteSession;
   friend class InviteSession;
   friend class DialogSet;

   // disabled
   DialogEventStateManager(const DialogEventStateManager& orig);

   // .jjg. we'll only have the DialogSetId if we aren't yet in the 'early' state;
   // once we get to early, we'll remove the DialogSetId in favour of the DialogId.
   // The comparator/key of the map must have an ordering so that a key can be
   // contructed which points to the beginning of a dialogSet.  This could be done by
   // no remote tag being always, which might be the existing behaviour, but
   // shouldn't be relied on.
   std::map<DialogId, DialogEventInfo*, DialogIdComparator> mDialogIdToEventInfo;

   DialogEventHandler* mDialogEventHandler;
};

}
#endif
