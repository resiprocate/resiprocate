#include "DialogSetMgr.h"

#include "resiprocate/os/MD5Stream.hxx"

DialogSetMgr::~DialogSetMgr() {
  DialogSetMap_iter iter = mDialogSetMap.begin();
  while (iter!=mDialogSetMap.end())
  {
    delete iter->second.second;
    mDialogSetMap.erase(iter);
  }
}

DialogState*
DialogSetMgr::matchDialogState(SipMessage* msg)
{
  assert(0); // Clean this function out
  Data key = getDialogSetKey(msg);
  DialogSetMap_iter iter = mDialogSetMap.find(key);
  if (iter!=mDialogSetMap.end())
  {
    return iter->second.second;
  }
  else
  {
    return NULL;
  }
}

DialogState*
DialogSetMgr::attachDialogState(SipMessage* msg)
{
  Data key = getDialogSetKey(msg);
  DialogState* dialogState;
  DialogSetMap_iter iter = mDialogSetMap.find(key);
  if (iter==mDialogSetMap.end()) 
  {
    dialogState = new DialogState(key,msg);
    pair<int, DialogState*> mapSecond = make_pair(1,dialogState);
    mDialogSetMap.insert(make_pair(key,mapSecond));
    cout << "Creating DialogSet " << key << endl;
  }
  else
  {
    dialogState = iter->second.second;
    iter->second.first++;
    cout << "Reference for DialogSet " << key << " is now " << iter->second.first << endl;
  }
  return dialogState;
}

void
DialogSetMgr::detachDialogState(SubDialog* subDialog)
{
  DialogSetMap_iter iter = mDialogSetMap.find(subDialog->dialogState()->key());
  if (iter!=mDialogSetMap.end())
  {
    iter->second.first -=1;
    cout << "Reference for DialogSet " << subDialog->dialogState()->key() << " is now " << iter->second.first << endl;
    if (iter->second.first == 0)
    {
      cout << "Destroying DialogSet " << subDialog->dialogState()->key() << endl;
      delete iter->second.second;
      mDialogSetMap.erase(iter);
    }
  }
}

// The following Key function builds handles to
// identify a Dialog Set (messages that share
// a common callID, remote and local tag) 

Data
DialogSetMgr::getDialogSetKey(SipMessage * msg)
{
  bool putToOnLeft; 

  if (msg->isExternal())
    putToOnLeft = true;
  else
    putToOnLeft = false;

  if (msg->isResponse())
    putToOnLeft = !putToOnLeft;
  
  Data leftTag("");
  Data rightTag("");
  if (putToOnLeft)
  {
    if (msg->header(h_To).exists(p_tag)) 
      leftTag = msg->header(h_To).param(p_tag);
    if (msg->header(h_From).exists(p_tag)) 
      rightTag = msg->header(h_From).param(p_tag);
  }
  else
  {
    if (msg->header(h_From).exists(p_tag)) 
      leftTag = msg->header(h_From).param(p_tag);
    if (msg->header(h_To).exists(p_tag)) 
      rightTag = msg->header(h_To).param(p_tag);
  }

  MD5Stream strm;
  strm << msg->header(h_CallId).value();
  strm << ":" << leftTag << rightTag;
  return strm.getHex();
}

