#include "InviteSessionHandler.hxx"

using namespace resip;

void 
InviteSessionHandler::onReadyToSend(InviteSession::Handle handle, const SipMessage& msg)
{
   handle->send(msg);
}
