#ifndef TUPRESSVR
#define TUPRESSVR

#include "resip/stack/SipStack.hxx"
#include "resip/stack/SipMessage.hxx"
#include "SubDialogMgr.h"

using namespace resip;

class TuPresSvr {
  public:
    TuPresSvr(SipStack* stack):mStack(stack),mDialogMgr(stack){}
    bool process();
    void processSubscribe(SipMessage* msg);
    void processNewSubscribe(SipMessage* msg);
    void processPublish(SipMessage* msg);
  private:
    SipStack* mStack;
    SubDialogMgr mDialogMgr;
};

#endif
