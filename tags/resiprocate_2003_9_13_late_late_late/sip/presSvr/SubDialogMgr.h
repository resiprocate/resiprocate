#ifndef SUBDIALOGMGR
#define SUBDIALOGMGR

#include <map>
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/os/Data.hxx"
#include "SubDialog.h"
#include "DialogSetMgr.h"

using resip::SipMessage;
using resip::Data;

// dialog key -> dialog object
typedef map<Data,SubDialog*> DialogMap_t;

// expiration time -> dialog key
typedef multimap<time_t,Data> ExpirationMap_t;

class SubDialogMgr {
  public:
    SubDialogMgr(SipStack* stack):mStack(stack),mDialogSetMgr(*this){}
    void dispatchSubscribe(SipMessage* msg);
    void dispatchNewSubscribe(SipMessage* msg);
    void dispatchNotifyResponse(SipMessage* msg);
    void processExpirations();
    bool dialogExists(SipMessage* msg);
  private:
    SubDialog* matchDialog(SipMessage* msg);
    SubDialog* createDialog(SipMessage* msg);
    Data getDialogKey(SipMessage* msg);
    void destroyDialog(SubDialog* dlg);
    SipStack *mStack;
    DialogSetMgr mDialogSetMgr;
    DialogMap_t mDialogMap;
    ExpirationMap_t mExpirationMap;
};

#endif
