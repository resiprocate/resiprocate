#include "PublicationCreator.hxx"

using namespace resip;

PublicationCreator::PublicationCreator(DialogUsageManager& dum, const Uri& aor)
   : BaseCreator(dum)
{
   makeInitialRequest(NameAddr(aor), PUBLISH);
}

void
PublicationCreator::dispatch(SipMessage& msg)
{
}
