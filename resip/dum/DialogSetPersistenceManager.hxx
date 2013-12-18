/*
 * DialogSetPersistenceManager.hxx
 *
 *  Created on: Dec 9, 2013
 *      Author: dragos
 */
#ifndef DIALOGSETPERSISTENCEMANAGER_HXX_
#define DIALOGSETPERSISTENCEMANAGER_HXX_

//#include "resip/dum/DialogSetId.hxx"
#include "rutil/HashMap.hxx"
#include "resip/dum/DialogSet.hxx"
#include "resip/dum/DialogSetChangeInfoManager.hxx"


namespace resip{
class DialogSet;
class DialogSetId;
class DialogUsageManager;

class DialogSetData{
   friend class DialogSetPersistenceManager;
   public:
      DialogSetData();
      DialogSetData(DialogSet & ds, SipMessage & req);
      Data getState() const;
      Data getId() const;
      Data getRequest() const;
      void setId(Data id);
      void setState(Data state);
      void setRequest(Data requestBuff);
      static Data DialogSetIdToData(const DialogSetId &id);
   private:
      Data dsid;
      Data dsstate;
      Data dsrequest;
};

	class DialogSetPersistenceManager
	{
	   private:

	   public:
		  typedef HashMap<DialogSetId, DialogSet*> DialogSetMap;
		  DialogSetPersistenceManager(DialogUsageManager &dum);
		  virtual ~DialogSetPersistenceManager();
		  bool syncDialogSet(DialogSetId id, DialogSetMap & dsmap); //internally updates changes from DialogSet
		  bool saveDialogSetChangesToPersistence(DialogSetChangeInfoManager::DialogSetChangesMap &changes); // saves changes from ds to persistent layer

	   private:
		  DialogUsageManager & mDum;
		  virtual bool checkIfUpdateNeeded()=0;
		  virtual bool readDialogSetFromDB(DialogSetId id, DialogSetData & dsdata)=0;
		  virtual bool addDialogSet(DialogSetChangeInfo & dsChange)=0;
		  virtual bool updateDialogSet(DialogSetChangeInfo & dsChange)=0;
		  virtual bool removeDialogSet(const DialogSetId &id)=0;
	};
}
#endif /* DIALOGSETPERSISTENCEMANAGER_HXX_ */
