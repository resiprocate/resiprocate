#include "OutOfDialogReqCreator.hxx"

using namespace resip;

OutOfDialogReqCreator::OutOfDialogReqCreator(DialogUsageManager& dum, MethodTypes method, const NameAddr& target, const NameAddr& from)
   : BaseCreator(dum)
{
   makeInitialRequest(target, from, method);
}

void
OutOfDialogReqCreator::dispatch(SipMessage& msg)
{
}

