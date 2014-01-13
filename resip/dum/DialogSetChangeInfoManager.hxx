/*
 * DialogSetChangeInfoManager.hxx
 *
 *  Created on: Dec 12, 2013
 *      Author: dragos
 */

#ifndef DIALOGSETCHANGEINFOMANAGER_HXX_
#define DIALOGSETCHANGEINFOMANAGER_HXX_


#include "resip/dum/DialogSet.hxx"

namespace resip
{
class DialogSet;
class DialogSetId;
class DialogSetPersistenceManager;

enum DSChangeFlags
{
 DS_UNCHANGED = 0,
 DS_NEW = 1,
 DS_CHANGED = 2,
 DS_REMOVED = 4
};

//DialogSetChangeInfo is associated with a DialogSet and contains a set of flags in order to know if the DialogSet was created, changed or removed
class DialogSetChangeInfo
{
   friend class DialogSetChangeInfoManager;
   friend class DialogSetPersistenceManager;
   private:
      //DialogSetChangeInfo(const DialogSet & dialogSet);
      DialogSetChangeInfo(const DialogSetId & dialogSetId, DSChangeFlags flag);
      //const DialogSet & getDialogSet() const;
      const DialogSetId getDialogSetId() const;
      void added();
      void changed();
      void removed();
      bool isNew() const;
      bool isChanged() const;
      bool isRemoved() const;
      //const std::list<Dialog *> & getAddedDialogs() const;
      //int getFlags() const;
      DialogSetId mDialogSetId; //uniquely identifies a DialogSet
      //const DialogSet & mDS;
      //std::list<Dialog *> mAddedDialogs;
      //Data ServerSubscriptionKey;//uniquely identifies a ServerSubscription
      int mFlags;

};


//keeps track of change state for Dialogs, until reset() is called. It is useful to track that state in order to write the DialogSet data to persistent layer (by DialogSetPersistenceManager)
class DialogSetChangeInfoManager
{
   public:
      typedef std::map <DialogSetId, DialogSetChangeInfo *> DialogSetChangesMap;
      bool DialogSetAdded(const DialogSetId & id);
      bool DialogSetChanged(const DialogSetId & id);
      bool DialogSetRemoved(const DialogSetId & id);
      //bool DialogAdded (DialogSet * ds, Dialog *dialog);
      DialogSetChangesMap& getChanges();
      bool reset();
      private:
         DialogSetChangesMap mChanges;

};
}

#endif /* DIALOGSETCHANGEINFOMANAGER_HXX_ */
