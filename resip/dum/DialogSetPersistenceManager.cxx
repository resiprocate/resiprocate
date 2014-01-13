/*
 * DialogSetPersistenceManager.cxx
 *
 *  Created on: Dec 11, 2013
 *      Author: dragos
 */


#include "resip/dum/DialogSetPersistenceManager.hxx"
#include "resip/dum/DialogSet.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "rutil/Logger.hxx"
#include "resip/dum/AppDialogSetFactory.hxx"
#include "resip/dum/AppDialogSet.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;


DialogSetData::DialogSetData()
{
}

/*
DialogSetData::DialogSetData(const DialogSet &ds, const SipMessage & req)

{
   DialogSet::State state = ds.mState;

   //UAC DialogSets won't use all of these states ..
   switch(state){
      case (DialogSet::Initial):
         mDSState ="Initial";
         break;
      case (DialogSet::WaitingToEnd):
         mDSState="WaitingToEnd";
         break;
      case (DialogSet::ReceivedProvisional):
         mDSState="ReceivedProvisional";
         break;
      case (DialogSet::Established):
         mDSState="Established";
         break;
      case (DialogSet::Terminating):
         mDSState="Terminating";
         break;
      case (DialogSet::Cancelling):
         mDSState="Cancelling";
         break;
      case (DialogSet::Destroying):
         mDSState="Destroying";
         break;

   }
   mDialogSetId = DialogSetData::dialogSetIdToData(ds.getId());
   mDSRequest = Data(req.getBuffer());
}

*/

const Data &
DialogSetData::getId() const
{
   return mDialogSetId;
}

const Data &
DialogSetData::getCallId() const
{
   return mCallId;
}

const Data &
DialogSetData::getTag() const
{
   return mTag;
}

const Data &
DialogSetData::getCancelKey() const
{
   return mCancelKey;
}

const Data &
DialogSetData::getRequestUri() const
{
   return mRequestUri;
}
const Data &
DialogSetData::getCSeq() const
{
   return mCSeq;
}

const std::list<DialogData>&
DialogSetData::getDialogs() const
{
   return mDialogs;
}

void
DialogSetData::setId(const Data& id)
{
   mDialogSetId = id;
}

void
DialogSetData::setCallId(const Data& callId)
{
   mCallId = callId;
}

void
DialogSetData::setTag(const Data& tag)
{
   mTag = tag;
}

void
DialogSetData::setCancelKey(const Data& cancelKey)
{
   mCancelKey = cancelKey;
}

void
DialogSetData::setRequestUri(const Data& requestUri)
{
   mRequestUri = requestUri;
}

void
DialogSetData::setCSeq(const Data& cSeq)
{
   mCSeq = cSeq;
}

void
DialogSetData::addDialogData(const DialogData& dialog)
{
   mDialogs.push_front(dialog);
}

Data
DialogSetData::dialogSetIdToData(const DialogSetId &id)
{
   Data dsid (id.getCallId() + Data("-") + Data(id.getLocalTag()));
   return dsid;
}

Data
DialogSetData::dialogSetStateData(const DialogSet &ds)
{
   DialogSet::State state = ds.mState;
   Data dsstate;
   switch(state){
      case (DialogSet::Initial):
         dsstate ="Initial";
         break;
      case (DialogSet::WaitingToEnd):
         dsstate="WaitingToEnd";
         break;
      case (DialogSet::ReceivedProvisional):
         dsstate="ReceivedProvisional";
         break;
      case (DialogSet::Established):
         dsstate="Established";
         break;
      case (DialogSet::Terminating):
         dsstate="Terminating";
         break;
      case (DialogSet::Cancelling):
         dsstate="Cancelling";
         break;
      case (DialogSet::Destroying):
         dsstate="Destroying";
         break;

   }
   return dsstate;
}


EncodeStream&
resip::operator<<(EncodeStream& strm, const DialogSetData& data)
{
   strm
      << data.getId()
      << data.getCancelKey()
      << data.getRequestUri()
      << data.getCSeq()
      ;
   return strm;
}

DialogData::DialogData(const Data& remoteTag, const Data& type, const std::vector<Data>&routes, const Data& localContact,
      int localCSeq, int remoteCSeq, const Data& remoteTarget, const Data& localNameAddr, const Data& remoteNameAddr, const Data& callId)
   : mRemoteTag(remoteTag),
     mLocalContact(localContact),
     mRoutes(routes),
     mLocalCSeq(localCSeq),
     mRemoteCSeq(remoteCSeq),
     mRemoteTarget(remoteTarget),
     mLocalNameAddr(localNameAddr),
     mRemoteNameAddr(remoteNameAddr),
     mCallId(callId),
     mServerSubscriptions()

{
}
Data
DialogData::getRemoteTag() const
{
   return mRemoteTag;
}
Data
DialogData::getState() const
{
   return mState;
}

const Data&
DialogData::getType() const
{
   return mType;
}
const std::vector<Data> &
DialogData::getRoutes() const
{
   return mRoutes;
}

Data
DialogData::getLocalContact() const
{
   return mLocalContact;
}
int
DialogData::getLocalCSeq() const
{
   return mLocalCSeq;
}
int
DialogData::getRemoteCSeq() const
{
   return mRemoteCSeq;
}
Data
DialogData::getRemoteTarget() const
{
   return mRemoteTarget;
}
Data
DialogData::getLocalNameAddr() const
{
   return mLocalNameAddr;
}
Data
DialogData::getRemoteNameAddr() const
{
   return mRemoteNameAddr;
}
Data
DialogData::getCallId() const
{
   return mCallId;
}

Data
DialogData::dialogTypeData(const Dialog& dlg)
{
   Data ret;
   Dialog::DialogType type =dlg.mType;
   if (type == Dialog::Invitation)
   {
      ret  = Data("Invitation");
   }
   else if (type == Dialog::Subscription)
   {
      ret = Data("Subscription");
   }
   else
   {
      ret = Data("Fake");
   }

   return ret;
}

const std::list<ServerSubscriptionData *> &
DialogData::getServerSubscriptions() const
{
   return mServerSubscriptions;
}

void
DialogData::addServerSubscription (ServerSubscriptionData * serverSubs)
{
   mServerSubscriptions.push_back(serverSubs);
}


ServerSubscriptionData::ServerSubscriptionData (const Data& lastRequest, const Data & lastResponse, const Data& documentKey, const Data & eventType, const Data& subscriptionId,
      const Data & mSubscriptionState, const Data& subscriber,const Data& lastSubscribeBuff, UInt64 absoluteExpiry)
   : BaseSubscriptionData(lastRequest, lastResponse, documentKey, eventType, subscriptionId, mSubscriptionState),
     mSubscriber(subscriber),
     mLastSubscribeBuff(lastSubscribeBuff),
     mAbsoluteExpiry(absoluteExpiry)
{
}

const Data&
ServerSubscriptionData::getSubscriber() const
{
   return mSubscriber;
}

const Data&
ServerSubscriptionData::getLastSubscribeBuff() const
{
   return mLastSubscribeBuff;
}

UInt64
ServerSubscriptionData::getAbsoluteExpiry() const
{
   return mAbsoluteExpiry;
}

BaseSubscriptionData::BaseSubscriptionData (const Data& lastRequest, const Data& lastResponse, const Data& documentKey, const Data & eventType,
      const Data& subscriptionId, const Data& subscriptionState)
   : mLastRequest(lastRequest),
     mLastResponse(lastResponse),
     mDocumentKey(documentKey),
     mEventType(eventType),
     mSubscriptionId(subscriptionId),
     mSubscriptionState(subscriptionState)
{
}

const Data&
BaseSubscriptionData::getLastRequest() const
{
   return mLastRequest;
}

const Data&
BaseSubscriptionData::getLastResponse() const
{
   return mLastResponse;
}


const Data&
BaseSubscriptionData::getDocumentKey() const
{
   return mDocumentKey;
}

const Data&
BaseSubscriptionData::getEventType() const
{
   return mEventType;
}

const Data&
BaseSubscriptionData::getSubscriptionId() const
{
   return mSubscriptionId;
}

const Data&
BaseSubscriptionData::getSubscriptionState() const
{
   return mSubscriptionState;
}

DialogSetPersistenceManager::DialogSetPersistenceManager(DialogUsageManager & dum)
   : mDum(dum)
{
}
DialogSetPersistenceManager::~DialogSetPersistenceManager()
{
}

bool
DialogSetPersistenceManager::syncDialogSet(DialogSetId id, DialogSetMap & map)
{

   if (!checkIfUpdateNeeded())
   {
      DebugLog ( << "no update needed for DialogSet with id " << id);
      return true;
   }

   DialogSetData dialogSetData;
   ReadResult res = readDialogSet(id, dialogSetData);

   if (res == NotFound)// if DialogSet not found in DB
   {
      DebugLog( << "DialogSet with id " << id << " not found in persistent layer");
      if (map.find(id) != map.end())// if DialogSet found in memory, remove it
      {
         DebugLog ( << "Removing in-memory DialogSet with id " << id);
         mDum.removeDialogSet(id);
      }
      return true;
   }
   else if (res == Error)
   {
      ErrLog(<< "Error reading DialogSet from persistent layer");
      return false;
   }
   DebugLog( <<"DialogSet with id " << id << " found in persistent layer");
   DialogSetMap::iterator it = map.find(id);
   if (it == map.end())
   {
      if (!internalCreateDialogSet(dialogSetData))
      {
         ErrLog ( << "creation of new DialogSet from DialogSetData failed");
         return false;
      }

   }
   else
   {
      DebugLog ( << "updating DialogSet in memory with id " << DialogSetData::dialogSetIdToData(id) );
      DialogSet * dset = it->second;
      if (!internalUpdateDialogSet (*dset, dialogSetData))
      {
         ErrLog ( << "update of in memory DialogSet failed");
         return false;
      }
   }
   return true;

}


bool
DialogSetPersistenceManager::internalUpdateDialogSet (DialogSet & ds, const DialogSetData& data)
{
   const std::list<DialogData> & dialogs = data.getDialogs();
   if (dialogs.empty())
   {
      WarningLog( << "the DialogSet retrieved from persistent layer has no Dialogs.");
   }
   for (std::list<DialogData>::const_iterator it = dialogs.begin(); it!=dialogs.end(); it++)
   {
      DebugLog( << "internally updating Dialog with id " << (*it).getRemoteTag() << " from DialogData");
      //Dialog * dialog = new Dialog(*this, *it, *dset);
      Dialog * dialog = ds.findDialog (DialogId (ds.getId(), (*it).getRemoteTag()));
      if (!dialog)//not covering this case yet
      {
         ErrLog ( << "the DialogSet retrieved from persistent layer has a Dialog which is not found in memory");
         return false;
      }
      //the data that can be updated in a dialog is local cseq, remote cseq and remote target (for target refresh).
      dialog->mLocalCSeq = (*it).getLocalCSeq();
      dialog->mRemoteCSeq = (*it).getRemoteCSeq();
      NameAddr remoteTarget ((*it).getRemoteTarget());
      dialog->mRemoteTarget = remoteTarget;

      const std::list<ServerSubscriptionData *> serverSubscriptions = (*it).getServerSubscriptions();

      for (std::list<ServerSubscriptionData *>::const_iterator serverSubsIt = serverSubscriptions.begin();
            serverSubsIt !=serverSubscriptions.end(); ++serverSubsIt)
      {

      }

   }
   return true;
}
DialogSet *
DialogSetPersistenceManager::internalCreateDialogSet(const DialogSetData &data)
{
   DebugLog ( << "creating UAS DialogSet from persistent layer data");

   DialogSet * dset = mDum.makeUASDialogSetFromDialogSetData(data);

   const std::list<DialogData> & dialogs = data.getDialogs();
   if (dialogs.empty())
   {
      WarningLog( << "the DialogSet retrieved from persistent layer has no Dialogs.");
   }
   for (std::list<DialogData>::const_iterator it = dialogs.begin(); it!=dialogs.end(); ++it)
   {
      DebugLog( << "creating new Dialog from DialogData with remote tag " << (*it).getRemoteTag());
      Dialog * dialog = new Dialog(mDum, *it, *dset);

      SipMessage dummy;
      dset->createAppDialog(dialog, dummy);

      //looking for server subscriptions and creating them in memory
      const std::list<ServerSubscriptionData *> serverSubscriptions = (*it).getServerSubscriptions();

      for (std::list<ServerSubscriptionData *>::const_iterator serverSubsIt = serverSubscriptions.begin();
            serverSubsIt !=serverSubscriptions.end(); ++serverSubsIt)
      {
         DebugLog (<< "creating new Subscription from ServerSubscriptionData");
         ServerSubscription * server = new ServerSubscription(mDum, *dialog, **serverSubsIt);
         dialog->mServerSubscriptions.push_back(server);
      }
   }
   return dset;
}

bool
DialogSetPersistenceManager::saveDialogSetChangesToPersistence(DialogSetChangeInfoManager::DialogSetChangesMap &changes)
{

   DebugLog ( << "saving DialogSet changes to persistent layer");

   if (changes.empty())
   {
      DebugLog( << "no changes to be saved" );
   }

   DialogSetChangeInfoManager::DialogSetChangesMap::iterator it;

   for (it = changes.begin(); it != changes.end(); it ++)
   {
      DialogSetChangeInfo dsChange = *(it->second);
      if (dsChange.isRemoved())
      {
         DebugLog ( << "removing DialogSet with ID" << dsChange.getDialogSetId() << " to persistent layer");
         if (!removeDialogSet(dsChange.getDialogSetId()))
         {
            return false;
         }
      }
      else if (dsChange.isNew())
      {
         DebugLog ( << "adding DialogSet with ID" << dsChange.getDialogSetId()<< " to persistent layer");
         DialogSet * dset = mDum.findDialogSet(dsChange.getDialogSetId());
         if (!dset)
         {
            WarningLog ( << "DialogSet with ID " << dsChange.getDialogSetId() << " not found in DUM.  Probably DialogSet is destroying");
            return false;
         }
         DialogSetData data;
         createDialogSetDataFromDialogSet (*dset, data);
         if (!addDialogSet(data)){
            return false;
         }
      }
      else if (dsChange.isChanged())
      {
         DebugLog ( << "updating DialogSet with ID" << dsChange.getDialogSetId()<< " to persistent layer");
         DialogSet * dset = mDum.findDialogSet(dsChange.getDialogSetId());
         if (!dset)
         {
            WarningLog ( << "DialogSet with ID " << dsChange.getDialogSetId() << " not found in DUM. Probably DialogSet is destroying");
            return false;
         }
         DialogSetData data;
         createDialogSetDataFromDialogSet (*dset, data);
         if (!updateDialogSet(data)){
            return false;
         }
      }
      else
      {
         DebugLog ( << "unrecognized set of flags");
         return false;
      }
   }

   return true;

}

bool
DialogSetPersistenceManager::createDialogSetDataFromDialogSet (const DialogSet & dialogSet, DialogSetData & data)
{
   data.setId(dialogSet.getId().toString().data());

   data.setCallId ( dialogSet.getId().getCallId().data());
   data.setTag( dialogSet.getId().getLocalTag().data());
   //data.setRequest(dialogSet.)
   //CancelKey is used for INVITE/CANCEL and is the branch id
   data.setCancelKey( dialogSet.mCancelKey.data());
   data.setRequestUri(dialogSet.mMergeKey.requestUri().data());
   data.setCSeq(dialogSet.mMergeKey.cseq());


   DialogSet::DialogMap dialogs = dialogSet.mDialogs;

   for (DialogSet::DialogMap::iterator it = dialogs.begin(); it != dialogs.end(); it++){

      Data dialogRemoteTag = it->second->getId().getRemoteTag().data();
      Data dialogType = DialogData::dialogTypeData((*(it->second))).data();

      /*getting routes */
      NameAddrs routeSet = (it->second)->mRouteSet;
      std::vector<Data>dialogRoutes;
      for (NameAddrs::iterator routesIter = routeSet.begin(); routesIter != routeSet.end(); routesIter++){
         dialogRoutes.push_back((*routesIter).toString().data());
      }


      Data dialogLocalContact = it->second->getLocalContact().toString().data();
      int dialogLocalCSeq =  it->second->getLocalCSeq();
      int dialogRemoteCSeq = it->second->getRemoteCSeq();
      Data dialogRemoteTarget = it->second->getRemoteTarget().toString().data();
      Data dialogLocalNameAddr = it->second->getLocalNameAddr().toString().data();
      Data dialogRemoteNameAddr = it->second->getRemoteNameAddr().toString().data();
      Data dialogCallId = it->second->getCallId().toString().data();



      DialogData dialogData (
            dialogRemoteTag,
            dialogType,
            dialogRoutes,
            dialogLocalContact,
            dialogLocalCSeq,
            dialogRemoteCSeq,
            dialogRemoteTarget,
            dialogLocalNameAddr,
            dialogRemoteNameAddr,
            dialogCallId
            );

      //searching for server subscriptions
      std::list<ServerSubscription*> serverSubscriptions = it->second->mServerSubscriptions;

      for (std::list<ServerSubscription*>::iterator i=serverSubscriptions.begin();
           i != serverSubscriptions.end(); ++i)
      {
         Data lastRequest = (*i)->mLastRequest.get()->toString();
         Data lastResponse = (*i)->mLastResponse.get()->toString();
         Data documentKey = (*i)->mDocumentKey;
         Data eventType = (*i)->mEventType;
         Data subscriptionId = (*i)->mSubscriptionId;
         const Data& subscriptionState = getSubscriptionStateString((*i)->mSubscriptionState);
         Data subscriber =  (*i)->mSubscriber;
         Data lastSubscribeBuff = (*i)->mLastSubscribe.toString();

         UInt32 absoluteExpiry = (*i)->mAbsoluteExpiry;

         ServerSubscriptionData * serverSubscriptionData = new ServerSubscriptionData(
                     lastRequest,
                     lastResponse,
                     documentKey,
                     eventType,
                     subscriptionId,
                     subscriptionState,
                     subscriber,
                     lastSubscribeBuff,
                     absoluteExpiry
                     );
         dialogData.addServerSubscription(serverSubscriptionData);
      }

      data.addDialogData(dialogData);


   }


   DebugLog (<< "created DialogSetData: " << data);

   //Data localTag, Data state, Data localContact, int localCSeq, int remoteCSeq, Data remoteTarget, Data localNameAddr, Data remoteNameAddr, Data callId

   return true;
}
