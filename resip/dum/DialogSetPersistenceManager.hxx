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
//class DialogId;
class DialogUsageManager;


class BaseSubscriptionData
{
   public:
      const Data& getLastRequest() const;
      const Data& getLastResponse() const;
      const Data& getDocumentKey() const;
      const Data& getEventType() const;
      const Data& getSubscriptionId() const;
      const Data& getSubscriptionState() const;
      BaseSubscriptionData ( const Data& documentKey, const Data & eventType, const Data& subscriptionId, const Data& subscriptionState );
   private:
      Data mDocumentKey;
      Data mEventType;
      Data mSubscriptionId;
      Data mSubscriptionState;
};

class ServerSubscriptionData : public BaseSubscriptionData
{
   public:
      const Data& getSubscriber () const;
      const Data& getLastSubscribeBuff() const;
      UInt64 getAbsoluteExpiry() const;
      ServerSubscriptionData (const Data& documentKey, const Data & eventType, const Data& subscriptionId, const Data& subscriptionState,
            const Data& subscriber, UInt64 mAbsoluteExpiry);
   private:
      Data mSubscriber;
      UInt64 mAbsoluteExpiry;
};

class DialogData{
   public:
      DialogData(const Data& remoteTag, const Data& type, const std::vector<Data>&routes, const Data& localContact,
            int localCSeq, int remoteCSeq, const Data& remoteTarget, const Data& localNameAddr, const Data& remoteNameAddr, const Data& callId);
      Data getRemoteTag() const;
      Data getState() const;
      const Data& getType() const;
      const std::vector<Data> & getRoutes() const;
      Data getLocalContact() const;
      int getLocalCSeq() const;
      int getRemoteCSeq() const;
      Data getRemoteTarget() const;
      Data getLocalNameAddr() const;
      Data getRemoteNameAddr() const;
      Data getCallId() const;
      static Data dialogTypeData(const Dialog& dlg);
      const std::list<ServerSubscriptionData *> & getServerSubscriptions() const;
      void addServerSubscription (ServerSubscriptionData * serverSub);
   private:
      Data mRemoteTag;
      Data mState;
      Data mType;
      std::vector<Data>mRoutes;
      Data mLocalContact;
      int mLocalCSeq;
      int mRemoteCSeq;
      Data mRemoteTarget;
      Data mLocalNameAddr;
      Data mRemoteNameAddr;
      Data mCallId;
      std::list<ServerSubscriptionData *> mServerSubscriptions;
};

class DialogSetData{
   friend class DialogSetPersistenceManager;
   public:
      DialogSetData();
      DialogSetData(const DialogSet & ds, const SipMessage & req);
      const Data& getState() const;
      const Data& getId() const;
      const Data& getCallId() const;
      const Data& getTag() const;
      //const Data& getRequest() const;
      const Data& getCancelKey() const;
      const Data& getRequestUri() const;
      const Data& getCSeq() const;
      const std::list<DialogData>&  getDialogs() const;
      void setId(const Data& id);
      void setCallId(const Data& callId);
      void setTag(const Data& tag);
      //void setState(const Data& state);
      //void setRequest(const Data& requestBuff);
      void setCancelKey(const Data& cancelKey);
      void setRequestUri(const Data& requestUri);
      void setCSeq(const Data& cSeq);
      void addDialogData (const DialogData& dialog);
      static Data dialogSetIdToData(const DialogSetId &id);
      static Data dialogSetStateData(const DialogSet &ds);

   private:
      Data mDialogSetId;
      Data mCallId;
      Data mTag;
      Data mDSState;
      Data mDSRequest;
      Data mCancelKey;
      Data mRequestUri;
      Data mCSeq;
      std::list<DialogData> mDialogs;
      friend EncodeStream& operator<<(EncodeStream& strm, const DialogSetData& data);

};



EncodeStream&
operator<<(EncodeStream& strm, const DialogSetData& data);

   class DialogSetPersistenceManager
   {
      public:
      enum ReadResult
      {
         Found,
         NotFound,
         Error
      };

      typedef HashMap<DialogSetId, DialogSet*> DialogSetMap;
      DialogSetPersistenceManager(DialogUsageManager &dum);
      virtual ~DialogSetPersistenceManager();
      //updates the DialogSet from the map to the state found in persistent layer
      bool syncDialogSet(DialogSetId id, DialogSetMap & dsmap);
      //updates the whole DialogSetMap to the state found in persistent layer
      bool syncAllDialogSets (DialogSetMap& map);
      bool internalUpdateDialogSet (DialogSet & ds, const DialogSetData& data);
      DialogSet * internalCreateDialogSet(const DialogSetData &data);
      // saves changes from ds to persistent layer
      bool saveDialogSetChangesToPersistence(DialogSetChangeInfoManager::DialogSetChangesMap &changes);
      bool createDialogSetDataFromDialogSet (const DialogSet & dialogSet, DialogSetData & data);

      private:
      DialogUsageManager & mDum;
      virtual bool checkIfUpdateNeeded()=0;
      virtual ReadResult readDialogSet(const DialogSetId & id, DialogSetData & dsdata)=0;

      virtual bool readAllDialogSets(std::list<DialogSetData *>& dialogs)=0;
      virtual bool addDialogSet(const DialogSetData & data)=0;
      virtual bool updateDialogSet(const DialogSetData & data)=0;
      virtual bool removeDialogSet(const DialogSetId &id)=0;
   };
}
#endif /* DIALOGSETPERSISTENCEMANAGER_HXX_ */
