#include "SubscriptionHandler.hxx"
#include "ServerSubscription.hxx"

using namespace resip;

void 
ServerSubscriptionHandler::onError(ServerSubscriptionHandle, const SipMessage& msg)
{
}

void 
ServerSubscriptionHandler::onExpiredByClient(ServerSubscriptionHandle, const SipMessage& sub, SipMessage& notify)
{
}

void 
ServerSubscriptionHandler::onExpired(ServerSubscriptionHandle, SipMessage& notify)
{
}

//!dcm! -- a bit clunky, but really want these objects to not have state
bool 
ServerSubscriptionHandler::hasDefaultExpires() const
{
   return false;
}

int 
ServerSubscriptionHandler::getDefaultExpires() const
{
   return -1;
}

void 
ServerSubscriptionHandler::onRefresh(ServerSubscriptionHandle handle, const SipMessage& sub)
{
   handle->send(handle->accept(200));
   handle->send(handle->neutralNotify());
}
