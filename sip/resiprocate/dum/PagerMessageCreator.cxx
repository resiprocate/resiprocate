#include "PagerMessageCreator.hxx"

using namespace resip;

PagerMessageCreator::PagerMessageCreator(DialogUsageManager& dum, const NameAddr& target, const NameAddr& from)
   : BaseCreator(dum)
{
   makeInitialRequest(target, from, MESSAGE);
   //rfc3428 section 9
   mLastRequest.remove(h_Supporteds);
   mLastRequest.remove(h_Accepts);
   mLastRequest.remove(h_Contacts);
}
