#include "InviteSessionHandler.hxx"

using namespace resip;

void 
InviteSessionHandler::onReadyToSend(InviteSessionHandle handle, SipMessage& msg)
{
   handle->send(msg);
}
