#ifndef TUPRESSVR
#define TUPRESSVR

#include "resiprocate/SipStack.hxx"
#include "resiprocate/SipMessage.hxx"
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
