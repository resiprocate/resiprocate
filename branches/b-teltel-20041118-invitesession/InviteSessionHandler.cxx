#include "InviteSessionHandler.hxx"
#include "resiprocate/os/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

void 
InviteSessionHandler::onReadyToSend(InviteSessionHandle handle, SipMessage& msg)
{
   handle->send(msg);
}

void 
InviteSessionHandler::onAckNotReceived(InviteSessionHandle handle, const SipMessage& msg)
{
   InfoLog(<< "InviteSessionHandler::onAckNotReceived: " << msg.brief());
   DebugLog(<< "onAckNotReceived(full): " << msg);
   handle->end();
}

void 
InviteSessionHandler::onIllegalNegotiation(InviteSessionHandle handle, const SipMessage& msg)
{
   InfoLog(<< "InviteSessionHandler::onIllegalNegotiation: " << msg.brief());
   DebugLog(<< "onIllegalNegotiation(full): " << msg);
   handle->end();
}
