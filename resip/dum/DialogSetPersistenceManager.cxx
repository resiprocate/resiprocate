/*
 * DialogSetPersistenceManager.cxx
 *
 *  Created on: Dec 11, 2013
 *      Author: dragos
 */


#include "resip/dum/DialogSetPersistenceManager.hxx"
#include "resip/dum/DialogSet.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;


DialogSetData::DialogSetData(){

}
DialogSetData::DialogSetData(DialogSet &ds, SipMessage & req){
   DialogSet::State state = ds.mState;
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
   dsid = DialogSetData::DialogSetIdToData(ds.getId());
   dsrequest = Data(req.getBuffer());
}
Data DialogSetData::getState() const{
   return dsstate;
}
Data DialogSetData::getId() const{
   return dsid;
}

Data DialogSetData::getRequest() const{
   return dsrequest;
}

void DialogSetData::setId(Data id){
   dsid = id;
}

void DialogSetData::setState(Data state){
   dsstate = state;
}

void DialogSetData::setRequest(Data requestBuff){
   dsrequest = requestBuff;
}

Data DialogSetData::DialogSetIdToData(const DialogSetId &id){
   Data did (id.getCallId() + Data("-") + Data(id.getLocalTag()));
//   did << Data("-");
//   did << Data(id.getLocalTag());
   return did;
}

DialogSetPersistenceManager::DialogSetPersistenceManager(DialogUsageManager & dum): mDum(dum){}
DialogSetPersistenceManager::~DialogSetPersistenceManager(){}
bool DialogSetPersistenceManager::updateDialogSet(DialogSetId id, DialogSetMap & map){
	//InfoLog(<< "here");

   if (checkIfUpdateNeeded())
   {
      DialogSetData dsdata;
      bool res = readDialogSetFromDB(id, dsdata);
      if (!res){// if DialogSet not found in DB
         DebugLog( << "DialogSet with id" << DialogSetData::DialogSetIdToData(id) << " not found in persistent layer");
         if (map.find(id) != map.end()){// if DialogSet found in memory, remove it
            DebugLog ( << "Removing in-memory DialogSet with id " << id);
            mDum.removeDialogSet(id);
         }
         return true;
      }
      DebugLog( <<"DialogSet with id " << DialogSetData::DialogSetIdToData(id) << " found in persistent layer");
      DialogSetMap::iterator it = map.find(id);
      DialogSet * dset = 0;
      if (it == map.end()){
         DebugLog ( << "creating new DialogSet in memory with id " << DialogSetData::DialogSetIdToData(id));
         Data requestMsgBuff = dsdata.getRequest();
         SipMessage *request = SipMessage::make(requestMsgBuff, true);
         dset = mDum.makeDialogSetFromRequest(*request);
      }
      else {
         DebugLog ( << "updating DialogSet in memory with id " << DialogSetData::DialogSetIdToData(id) );
         dset = it->second;
      }
      Data state = dsdata.getState();
      //we may not use all these states when
      if (state == "Initial")
         dset->mState = DialogSet::Initial;
      else if (state == "WaitingToEnd")
         dset->mState = DialogSet::WaitingToEnd;
      else if (state == "ReceivedProvisional")
         dset->mState = DialogSet::ReceivedProvisional;
      else if (state == "Established")
         dset->mState = DialogSet::Established;
      else if (state == "Terminating")
         dset->mState = DialogSet::Terminating;
      else if (state == "Cancelling")
         dset->mState = DialogSet::Cancelling;
      else if (state == "Destroying")
         dset->mState = DialogSet::Destroying;

   return true;
   }
}
