#include "OutOfDialogReqCreator.hxx"

using namespace resip;

OutOfDialogReqCreator::OutOfDialogReqCreator(DialogUsageManager& dum, MethodTypes method, const Uri& target)
   : BaseCreator(dum)
{
   makeInitialRequest(NameAddr(target), method);
}

