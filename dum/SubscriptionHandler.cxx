#include "SubscriptionHandler.hxx"

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

