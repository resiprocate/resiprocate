#include "InviteSessionHandler.hxx"

using namespace resip;

void 
InviteSessionHandler::onReadyToSend(InviteSessionHandle handle, const SipMessage& msg)
{
   handle->send(msg);
}
