#ifndef DIALOGSETMGR
#define DIALOGSETMGR

#include "DialogState.h"
#include "SubDialog.h"

class  SubDialogMgr;

#include <map>

using namespace std;

//Map dialog set ID to a DialogState object
typedef map<Data,pair<int,DialogState *> > DialogSetMap_t;
typedef DialogSetMap_t::iterator DialogSetMap_iter;

class DialogSetMgr
{
  public:
    DialogSetMgr(SubDialogMgr& dialogMgr)
           :mDialogMgr(dialogMgr){}
    ~DialogSetMgr();
    DialogState* matchDialogState(SipMessage* msg); 
    DialogState* attachDialogState(SipMessage* msg);
    void detachDialogState(SubDialog* subDialog);
    Data getDialogSetKey(SipMessage* msg);
  private:
    DialogSetMap_t mDialogSetMap;
    SubDialogMgr& mDialogMgr;
};
#endif
