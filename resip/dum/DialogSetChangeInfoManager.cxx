/*
 * DialogSetChangeInfoManager.cxx
 *
 *  Created on: Dec 12, 2013
 *      Author: dragos
 */
#include "resip/dum/DialogSetChangeInfoManager.hxx"
#include "rutil/Logger.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM


DialogSetChangeInfo::DialogSetChangeInfo(const DialogSetId & id, DSChangeFlags flag)
   : mDialogSetId(id),
     mFlags(flag)
{
}

const
DialogSetId DialogSetChangeInfo::getDialogSetId() const
{
   return mDialogSetId;
}

void
DialogSetChangeInfo::added()
{
    mFlags |= DS_NEW;
}

void
DialogSetChangeInfo::changed()
{
    mFlags |= DS_CHANGED;
}

void
DialogSetChangeInfo::removed()
{
    mFlags |= DS_REMOVED;
}

bool
DialogSetChangeInfo::isNew() const
{
   return (mFlags & DS_NEW)? true:false;
}
bool
DialogSetChangeInfo::isChanged() const
{
   return (mFlags & DS_CHANGED)? true:false;
}
bool
DialogSetChangeInfo::isRemoved() const
{
   return (mFlags & DS_REMOVED)? true:false;
}
/*
const std::list<Dialog *> & DialogSetChangeInfo::getAddedDialogs() const{
   return mAddedDialogs;
}
*/

DialogSetChangeInfoManager::DialogSetChangesMap&
DialogSetChangeInfoManager::getChanges ()
{
   return mChanges;
}

bool
DialogSetChangeInfoManager::reset()
{
   for (DialogSetChangesMap::iterator it = mChanges.begin(); it != mChanges.end(); it++)
   {
      delete it->second;
   }
   mChanges.clear();
}

bool
DialogSetChangeInfoManager::DialogSetAdded(const DialogSetId &id)
{
   DebugLog ( << "flagging DialogSet " << id << "for creation in persistent layer");
   /*
   if (!ds){
      DebugLog(<< "argument is null");
      return false;
   }
   */
   std::map<DialogSetId, DialogSetChangeInfo *>::iterator it= mChanges.find(id);
   if (it != mChanges.end()){
      DebugLog ( << "DialogSet already added");
      return false;
   }
   DialogSetChangeInfo *dsc = new DialogSetChangeInfo(id, DS_NEW);
   //dsc->mFlags = DS_NEW;
   //dsc->mDS = ds;
   //dsc->mDialogSetId = ds->getId();
   //dsc->mRequest = &request;
   mChanges[id]  = dsc;
   return true;
}


bool
DialogSetChangeInfoManager::DialogSetChanged(const DialogSetId &id)
{
   DebugLog ( << "flagging DialogSet " << id << "for update in persistent layer");
   std::map<DialogSetId, DialogSetChangeInfo *>::iterator it= mChanges.find(id);
   if (it == mChanges.end())
   {
      DialogSetChangeInfo *dsc = new DialogSetChangeInfo(id, DS_CHANGED);
      //dsc->changed();
      mChanges[id]  = dsc;
      //DialogSetChangeInfo * dsc = new DialogSetChangeInfo(ds->getId());
      //dsc->mFlags = DS_CHANGED;
      //dsc->mDS =  ds;
      //dsc->mDialogSetId = ds->getId();
      //mChanges[ds->getId()]  = dsc;
   }
   else
   {
      it->second->changed();
   }
   return true;
}


bool
DialogSetChangeInfoManager::DialogSetRemoved(const DialogSetId & id)
{
   DebugLog ( << "flagging DialogSet " << id << "for removal from persistent layer");
   std::map<DialogSetId, DialogSetChangeInfo *>::iterator it = mChanges.find(id);
   if (it == mChanges.end())
   {
      DialogSetChangeInfo * dsc = new DialogSetChangeInfo(id, DS_REMOVED);
      dsc->removed();
      //dsc->mFlags = DS_REMOVED;
      mChanges[id]  = dsc;
   }
   else {
      it->second->removed();

   }
   return true;
}



