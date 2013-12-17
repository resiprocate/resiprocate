/*
 * DialogSetChangeInfoManager.cxx
 *
 *  Created on: Dec 12, 2013
 *      Author: dragos
 */
#include "resip/dum/DialogSetChangeInfoManager.hxx"

using namespace resip;

DialogSetChangeInfo::DialogSetChangeInfo(const DialogSetId& id): mDSId(id), mDS(0), mDialog(0), flags(0){}
DialogSetChangeInfo::DialogSetChangeInfo(): mDSId(Data(""), Data("")), mDS(0), mDialog(0), flags(0){}
DialogSet * DialogSetChangeInfo::getDialogSet(){
   return mDS;
}


DialogSetId DialogSetChangeInfo::getDialogSetId(){
   return mDSId;
}
int DialogSetChangeInfo::getFlags(){
   return flags;
}

SipMessage * DialogSetChangeInfo::getRequest(){
   return mRequest;
}




DialogSetChangeInfoManager::DialogSetChangesMap& DialogSetChangeInfoManager::getChanges (){
   return changes;
}

bool DialogSetChangeInfoManager::reset(){
   changes.clear();
}

bool DialogSetChangeInfoManager::DialogSetAdded(DialogSet *ds, SipMessage &request){
   std::map<DialogSetId, DialogSetChangeInfo>::iterator it= changes.find(ds->getId());
   if (it != changes.end()){

      return false;
   }
   DialogSetChangeInfo dsc(ds->getId());
   dsc.flags = DS_NEW;
   dsc.mDS = ds;
   dsc.mRequest = &request;
   //dsc.mDialogSetData (ds, request);
   changes[ds->getId()]  = dsc;
   return true;
}


bool DialogSetChangeInfoManager::DialogSetChanged(DialogSet *ds){
   std::map<DialogSetId, DialogSetChangeInfo>::iterator it= changes.find(ds->getId());
   if (it == changes.end()){
      DialogSetChangeInfo dsc(ds->getId());
      dsc.flags = DS_REMOVED;
      dsc.mDS =  ds;
      changes[ds->getId()]  = dsc;
   }
   else {
      it->second.flags &= DS_CHANGED;
      it->second.mDS = ds;

   }
   return true;
}


bool DialogSetChangeInfoManager::DialogSetRemoved(const DialogSetId & id){
   std::map<DialogSetId, DialogSetChangeInfo>::iterator it= changes.find(id);
   if (it == changes.end()){
      DialogSetChangeInfo dsc(id);
      dsc.flags = DS_REMOVED;
      changes[id]  = dsc;
   }
   else {
      it->second.flags = DS_REMOVED;

   }
   return true;
}

bool DialogSetChangeInfoManager::DialogAdded(DialogSet *ds, Dialog *dlg){
   return true;
}



