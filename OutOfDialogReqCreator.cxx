#include "OutOfDialogReqCreator.hxx"

using namespace resip;

OutOfDialogReqCreator::OutOfDialogReqCreator(DialogUsageManager& dum, MethodTypes method, const NameAddr& target, Identity& identity)
   : BaseCreator(dum, identity)
{
   makeInitialRequest(target, method);
}

void
OutOfDialogReqCreator::dispatch(SipMessage& msg)
{
}

