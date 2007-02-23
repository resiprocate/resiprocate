


#include "resip/stack/ExtensionHeader.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Tuple.hxx"
#include "rutil/Data.hxx"

#include "DialInstance.hxx"
#include "MyInviteSessionHandler.hxx"

using namespace resip;
using namespace std;

MyInviteSessionHandler::MyInviteSessionHandler(DialInstance& dialInstance) : 
   mDialInstance(dialInstance)
{
}

MyInviteSessionHandler::~MyInviteSessionHandler()
{
}

void MyInviteSessionHandler::onSuccess(ClientRegistrationHandle h, const SipMessage& response) 
{
}

void MyInviteSessionHandler::onFailure(ClientRegistrationHandle, const SipMessage& msg) 
{
}

void MyInviteSessionHandler::onMessage(resip::Handle<resip::InviteSession>, const resip::SipMessage& msg) 
{
}

void MyInviteSessionHandler::onMessageSuccess(resip::Handle<resip::InviteSession>, const resip::SipMessage&) 
{
}

void MyInviteSessionHandler::onMessageFailure(resip::Handle<resip::InviteSession>, const resip::SipMessage&) 
{
}

void MyInviteSessionHandler::onFailure(ClientInviteSessionHandle cis, const SipMessage& msg) 
{
   mDialInstance.onFailure();
}

void MyInviteSessionHandler::onForkDestroyed(ClientInviteSessionHandle) 
{
}

void MyInviteSessionHandler::onInfoSuccess(InviteSessionHandle, const SipMessage& msg) 
{
}

void MyInviteSessionHandler::onInfoFailure(InviteSessionHandle, const SipMessage& msg) 
{
}

void MyInviteSessionHandler::onProvisional(ClientInviteSessionHandle cis, const SipMessage& msg) 
{
}

void MyInviteSessionHandler::onConnected(ClientInviteSessionHandle cis, const SipMessage& msg) 
{
   mDialInstance.onConnected(cis);

   SdpContents *sdp = (SdpContents*)msg.getContents();
   cis->provideAnswer(*sdp);
}

void MyInviteSessionHandler::onStaleCallTimeout(ClientInviteSessionHandle) 
{
}

void MyInviteSessionHandler::onConnected(InviteSessionHandle, const SipMessage& msg) 
{
}

void MyInviteSessionHandler::onRedirected(ClientInviteSessionHandle, const SipMessage& msg) 
{
}

void MyInviteSessionHandler::onAnswer(InviteSessionHandle is, const SipMessage& msg, const SdpContents& sdp) 
{
}

void MyInviteSessionHandler::onEarlyMedia(ClientInviteSessionHandle cis, const SipMessage& msg, const SdpContents& sdp) 
{
}

void MyInviteSessionHandler::onOfferRequired(InviteSessionHandle, const SipMessage& msg) 
{
}

void MyInviteSessionHandler::onOfferRejected(Handle<InviteSession>, const SipMessage *msg) 
{
}

void MyInviteSessionHandler::onDialogModified(InviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg) 
{
}

void MyInviteSessionHandler::onInfo(InviteSessionHandle, const SipMessage& msg) 
{
}

void MyInviteSessionHandler::onRefer(InviteSessionHandle, ServerSubscriptionHandle, const SipMessage& msg) 
{
}

void MyInviteSessionHandler::onReferAccepted(InviteSessionHandle, ClientSubscriptionHandle, const SipMessage& msg) 
{
   mDialInstance.onReferSuccess();
}

void MyInviteSessionHandler::onReferRejected(InviteSessionHandle, const SipMessage& msg) 
{
   mDialInstance.onReferFailed();
}

void MyInviteSessionHandler::onReferNoSub(resip::Handle<resip::InviteSession> is, const resip::SipMessage& msg) 
{
}

void MyInviteSessionHandler::onRemoved(ClientRegistrationHandle) 
{
}

int MyInviteSessionHandler::onRequestRetry(ClientRegistrationHandle, int retrySeconds, const SipMessage& response) 
{
  return -1;
}

void MyInviteSessionHandler::onNewSession(ServerInviteSessionHandle sis, InviteSession::OfferAnswerType oat, const SipMessage& msg) 
{
}

void MyInviteSessionHandler::onNewSession(ClientInviteSessionHandle cis, InviteSession::OfferAnswerType oat, const SipMessage& msg) 
{
}

void MyInviteSessionHandler::onTerminated(InviteSessionHandle is, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg) 
{
   mDialInstance.onTerminated();
}

void MyInviteSessionHandler::onOffer(InviteSessionHandle is, const SipMessage& msg, const SdpContents& sdp) 
{
}


