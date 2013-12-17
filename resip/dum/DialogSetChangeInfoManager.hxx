/*
 * DialogSetChangeInfoManager.hxx
 *
 *  Created on: Dec 12, 2013
 *      Author: dragos
 */

#ifndef DIALOGSETCHANGEINFOMANAGER_HXX_
#define DIALOGSETCHANGEINFOMANAGER_HXX_


#include "resip/dum/DialogSet.hxx"
//#include "resip/dum/DialogSetId.hxx"
//#include "resip/dum/DialogSetPersistenceManager.hxx"

using namespace std;

namespace resip
{
class DialogSet;
class DialogSetId;
class DialogSetPersistenceManager;
//can be extended
enum DSChangeFlags{
 DS_UNCHANGED = 0,
 DS_NEW = 1,
 DS_CHANGED = 2,
 DS_REMOVED = 4,
 DS_DIALOG_ADDED = 8,
 DS_DIALOG_CHANGED = 16,
 DS_DIALOG_REMOVED = 32,
 DS_DIALOG_SERVERSUBSCRIPTION_ADDED = 64,
 DS_DIALOG_SERVERSUBSCRIPTION_CHANGED = 128,
 DS_DIALOG_SERVERSUBSCRIPTION_REMOVED = 256
};


class DialogSetChangeInfo{
   friend class DialogSetChangeInfoManager;
   friend class DialogSetPersistenceManager;
   public:
   DialogSetChangeInfo();
   DialogSetChangeInfo(const DialogSetId & id);
   DialogSet * getDialogSet();
   DialogSetId getDialogSetId();
   SipMessage* getRequest();
   int getFlags();
   private:

      DialogSetId mDSId; //uniquely identifies a DialogSet
      DialogSet * mDS;
   //DialogId mDialogId;//uniquely identifies a Dialog
      Dialog * mDialog;
      SipMessage * mRequest;
      //DialogSetData mDialogSetData;
      Data ServerSubscriptionKey;//uniquely identifies a ServerSubscription
      int flags;
//      std::string GetDialogSetState();

};

class DialogSetChangeInfoManager{
   public:
      typedef std::map <DialogSetId, DialogSetChangeInfo> DialogSetChangesMap;
      bool DialogSetAdded(DialogSet * ds,SipMessage & req);
      bool DialogSetChanged(DialogSet* ds);
      bool DialogSetRemoved(const DialogSetId &id);
      bool DialogAdded (DialogSet * ds, Dialog *dialog);
      bool updateChanges();
      DialogSetChangesMap& getChanges ();
      bool reset();
      private:
         DialogSetChangesMap changes;

};
}

#endif /* DIALOGSETCHANGEINFOMANAGER_HXX_ */
