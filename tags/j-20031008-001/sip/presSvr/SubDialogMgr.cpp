#include "resiprocate/Helper.hxx"
#include "resiprocate/os/MD5Stream.hxx"
#include "SubDialogMgr.h"

using namespace resip;

void
SubDialogMgr::dispatchSubscribe(SipMessage* msg)
{
  SubDialog *dialog = matchDialog(msg);
  if (!dialog)
  {
    auto_ptr<SipMessage> resp(Helper::makeResponse(*msg,481,""));
    mStack->send(*resp);
  }
  SubDialogCondition cond= dialog->processSubscribe(msg);
  if (cond==TERMINATED)
  {
    destroyDialog(dialog);
  }
  else
  {
    mExpirationMap.insert( make_pair(dialog->expires(),dialog->key()) );
  }
}

void
SubDialogMgr::dispatchNewSubscribe(SipMessage* msg)
{
  SubDialog *dialog = createDialog(msg);
  SubDialogCondition cond = dialog->processSubscribe(msg);
  if (cond==TERMINATED)
  {
    destroyDialog(dialog);
  }
  else
  {
    mExpirationMap.insert( make_pair(dialog->expires(),dialog->key()) ); 
  }
}


void
SubDialogMgr::dispatchNotifyResponse(SipMessage* msg)
{
  SubDialog *dialog = matchDialog(msg);
  if (dialog)
  {
    SubDialogCondition cond = dialog->processNotifyResponse(msg);
    if (cond==TERMINATED)
    {
      destroyDialog(dialog);
    }
  }
}

SubDialog *
SubDialogMgr::matchDialog(SipMessage* msg)
{
  Data dialogKey = getDialogKey(msg);
  DialogMap_t::iterator iter=mDialogMap.find(dialogKey) ;
  return (iter==mDialogMap.end()? NULL: (*iter).second);
}

bool
SubDialogMgr::dialogExists(SipMessage* msg)
{
  return (matchDialog(msg)!=NULL);
}

SubDialog *
SubDialogMgr::createDialog(SipMessage* msg)
{
  if (!msg->header(h_To).exists(p_tag))
      msg->header(h_To).param(p_tag) = Helper::computeTag(Helper::tagSize);
  Data dialogKey = getDialogKey(msg);
  assert(mDialogMap.find(dialogKey)==mDialogMap.end());

  DialogState* dlgState = mDialogSetMgr.attachDialogState(msg);
  SubDialog *dlg = new SubDialog(dialogKey,mStack,dlgState);
  pair<DialogMap_t::iterator,bool> result = 
    mDialogMap.insert(make_pair(dialogKey,dlg));
  assert(result.second);
  return(dlg);
}

void
SubDialogMgr::destroyDialog(SubDialog* dlg)
{
  DialogMap_t::iterator iter=
	 mDialogMap.find(dlg->key());
  assert ( iter!=mDialogMap.end() ); 
  mDialogSetMgr.detachDialogState(dlg);
  mDialogMap.erase(iter);
  delete (dlg);
}

Data
SubDialogMgr::getDialogKey(SipMessage * msg)
{
  //closer, but this is still broken
  // one has the dialogsetkey in it, the other doesn't.
  //
  if (msg->isRequest())
  {
    MD5Stream dlgKeyStr;
    dlgKeyStr << mDialogSetMgr.getDialogSetKey(msg);
    dlgKeyStr << ":";
    MD5Stream eventHashStr;
    assert(msg->exists(h_Event));
    eventHashStr << msg->header(h_Event);
    if (msg->header(h_Event).exists(p_id))
      eventHashStr << ":" << msg->header(h_Event).param(p_id);
    dlgKeyStr << eventHashStr.getHex();
    return dlgKeyStr.getHex();
  }
  else
  {
    assert(msg->exists(h_Vias));
    return msg->header(h_Vias).front().param(p_branch).clientData();
  }
}

// A (time,Data) pair gets put into mExpirationMap whenever
// a subscribe establishing a new expiration is processed.
// When a subscription is deleted, this map is not cleaned,
// so when processing it, we have to make sure the subscription
// that Data keys is still there.
// On a refresh subscription, old (time,Data) pairs belonging to 
// this subscription are not removed, so we have to make
// sure the subscription keyed by this Data has actually
// expired before taking action beyond removing this pair.

// A _Very large_ number of subscriptions expiring at once  might 
// block this thread too long. An alternative  would be to return 
// after processing the first (few?) subscriptions.

void
SubDialogMgr::processExpirations()
{
  ExpirationMap_t::iterator iter = mExpirationMap.begin();
  while (( iter != mExpirationMap.end() ) && ( (iter->first) <= time(NULL) ))
  {
    DialogMap_t::iterator dlgIter = mDialogMap.find(iter->second);
    if (dlgIter!=mDialogMap.end() 
	    && (dlgIter->second)->expires() <= time(NULL))
    {
      (dlgIter->second)->sendNotify();
      destroyDialog(dlgIter->second);
    }
    mExpirationMap.erase(iter);
    ++iter;
  }
}
