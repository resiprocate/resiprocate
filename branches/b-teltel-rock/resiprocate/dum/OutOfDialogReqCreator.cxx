#include "OutOfDialogReqCreator.hxx"

using namespace resip;

OutOfDialogReqCreator::OutOfDialogReqCreator(DialogUsageManager& dum, MethodTypes method, const NameAddr& target, UserProfile& userProfile)
   : BaseCreator(dum, userProfile)
{
   makeInitialRequest(target, method);
}

void
OutOfDialogReqCreator::dispatch(SipMessage& msg)
{
}

