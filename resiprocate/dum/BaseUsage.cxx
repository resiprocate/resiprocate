#include "BaseUsage.hxx"

SipMessage* 
BaseUsage::makeInviteSession(const Uri& target)
{
   return mDialog.makeInviteSession(target);
}

SipMessage* 
BaseUsage::makeSubscription(const Uri& aor, const Data& eventType)
{
   return mDialog.makeSubscription(aor, eventType);
}

SipMessage* 
BaseUsage::makeRefer(const Uri& aor, const H_ReferTo::Type& referTo)
{
   return mDialog.makeRefer(aor, referTo);
}

SipMessage* 
BaseUsage::makePublication(const Uri& aor, const Data& eventType)
{
   return mDialog.makePublication(aor, eventType);
}

SipMessage* 
BaseUsage::makeRegistration(const Uri& aor)
{
   return mDialog.makeRegistration(aor);
}

SipMessage* 
BaseUsage::makeOutOfDialogRequest(const Uri& aor, const MethodTypes& meth)
{
   return mDialog.makeOutOfDialogRequest(aor, meth);
}


