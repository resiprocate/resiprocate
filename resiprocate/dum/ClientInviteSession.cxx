#include "ClientInviteSession.hxx"

ServerInviteSession::ServerInviteSession(DialogUsageManager& dum, const SipMessage& msg) 
   : InviteSession(dum)
{
}

void 
ClientInviteSession::setOffer(SdpContents* offer)
{
}

void 
ClientInviteSession::sendOfferInAnyMessage()
{
}

void 
ClientInviteSession::setAnswer(SdpContents* answer)
{
}

void 
ClientInviteSession::sendAnswerInAnyMessage()
{
}

void 
ClientInviteSession::end()
{
}

void 
ClientInviteSession::rejectOffer(int statusCode)
{
}
