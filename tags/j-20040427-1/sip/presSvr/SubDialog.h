#ifndef SUBDIALOG
#define SUBDIALOG

#include "resiprocate/SipMessage.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/os/Data.hxx"

#include "SubscriptionState.h"
#include "DialogState.h"

using namespace resip;

enum SubDialogCondition { PENDING, ACTIVE, TERMINATED };

class SubDialog {

  public:

    SubDialog(Data key, SipStack* stack,DialogState* dlgState);
    ~SubDialog();
    SubDialogCondition processSubscribe(SipMessage* msg);
    SubDialogCondition processNotifyResponse(SipMessage *msg);
    const Data& key(){ return mKey; }
    DialogState* dialogState() {return mDlgState;}
    time_t expires(); 
    void presenceDocumentChanged(const Contents* document);
    void sendNotify();

  private:
    void sendNotify(const Contents* document);
    Data mKey;
    SipStack* mStack;
    SubscriptionState* mSubState;
    DialogState* mDlgState;

};

#endif
