#include "InviteSessionCreator.hxx"

InviteSessionCreator::InviteSessionCreator(DialogUsageManager& dum, const Uri& aor, const SdpContents* initial)
   : BaseCreator(dum),
     mState(Initialized),
     mInitialOffer(static_cast<SdpContents*>(initial->clone()))
{
   makeInitialRequest(aor, INVITE);
}

void
InviteSessionCreator::end()
{
   assert(0);
}

   
