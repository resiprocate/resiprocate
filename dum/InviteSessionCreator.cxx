#include "InviteSessionCreator.hxx"
#include "resiprocate/SdpContents.hxx"

using namespace resip;

InviteSessionCreator::InviteSessionCreator(DialogUsageManager& dum, const Uri& aor, const SdpContents* initial)
   : BaseCreator(dum),
     mState(Initialized),
     mInitialOffer(static_cast<SdpContents*>(initial->clone()))
{
   makeInitialRequest(NameAddr(aor), INVITE);
   getLastRequest().setContents(mInitialOffer);
}

void
InviteSessionCreator::end()
{
   assert(0);
}

void
InviteSessionCreator::dispatch(const SipMessage& msg)
{
   // !jf! does this have to do with CANCELing all of the branches associated
   // with a single invite request
}
