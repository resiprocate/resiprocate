#include "BaseUsage.hxx"

SipMessage* 
BaseUsage::makeInviteSession()
{
   return mDialog.makeInviteSession();
}

SipMessage* 
BaseUsage::makeSubscription()
{
   return mDialog.makeSubscription();
}

SipMessage* 
BaseUsage::makeRefer()
{
   return mDialog.makeRefer();
}

SipMessage* 
BaseUsage::makePublication()
{
   return mDialog.makePublication();
}

SipMessage* 
BaseUsage::makeRegistration()
{
   return mDialog.makeRegistration();
}

SipMessage* 
BaseUsage::makeOutOfDialogRequest()
{
   return mDialog.makeOutOfDialogRequest();
}


