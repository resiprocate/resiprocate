#include "InviteSessionHandler.hxx"

using namespace resip;

void 
InviteSessionHandler::onReadyToSend(InviteSessionHandle handle, SipMessage& msg)
{
   handle->send(msg);
}

void 
InviteSessionHandler::onAckNotReceived(InviteSessionHandle handle, const SipMessage& msg)
{
   handle->send(handle->end());
}

void 
InviteSessionHandler::onIllegalNegotiation(InviteSessionHandle handle, const SipMessage& msg)
{
   handle->send(handle->end());
}
