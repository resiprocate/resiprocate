

#ifndef __MyInviteSessionHandler_h
#define __MyInviteSessionHandler_h

#include "resip/stack/SipMessage.hxx"
#include "resip/dum/ClientInviteSession.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/ServerInviteSession.hxx"

class MyInviteSessionHandler : public resip::InviteSessionHandler {

protected:
  DialInstance& mDialInstance;

public:
  MyInviteSessionHandler(DialInstance& dialInstance);
  virtual ~MyInviteSessionHandler();

  virtual void onSuccess(resip::ClientRegistrationHandle h, const resip::SipMessage& response);
  virtual void onFailure(resip::ClientRegistrationHandle, const resip::SipMessage& msg);
  virtual void onMessage(resip::Handle<resip::InviteSession>, const resip::SipMessage& msg);
  virtual void onMessageSuccess(resip::Handle<resip::InviteSession>, const resip::SipMessage&);
  virtual void onMessageFailure(resip::Handle<resip::InviteSession>, const resip::SipMessage&);
  virtual void onFailure(resip::ClientInviteSessionHandle cis, const resip::SipMessage& msg);
  virtual void onForkDestroyed(resip::ClientInviteSessionHandle);
  virtual void onInfoSuccess(resip::InviteSessionHandle, const resip::SipMessage& msg);
  virtual void onInfoFailure(resip::InviteSessionHandle, const resip::SipMessage& msg);
  virtual void onProvisional(resip::ClientInviteSessionHandle cis, const resip::SipMessage& msg);
  virtual void onConnected(resip::ClientInviteSessionHandle, const resip::SipMessage& msg);
  virtual void onConnected(resip::InviteSessionHandle, const resip::SipMessage& msg);
  virtual void onStaleCallTimeout(resip::ClientInviteSessionHandle);
  virtual void onRedirected(resip::ClientInviteSessionHandle, const resip::SipMessage& msg);
  virtual void onAnswer(resip::InviteSessionHandle is, const resip::SipMessage& msg, const resip::SdpContents& sdp);
  virtual void onEarlyMedia(resip::ClientInviteSessionHandle, const resip::SipMessage& msg, const resip::SdpContents& sdp);
  virtual void onOfferRequired(resip::InviteSessionHandle, const resip::SipMessage& msg);
  virtual void onOfferRejected(resip::Handle<resip::InviteSession>, const resip::SipMessage *msg);
  virtual void onDialogModified(resip::InviteSessionHandle, resip::InviteSession::OfferAnswerType oat, const resip::SipMessage& msg);
  virtual void onInfo(resip::InviteSessionHandle, const resip::SipMessage& msg);
  virtual void onRefer(resip::InviteSessionHandle, resip::ServerSubscriptionHandle, const resip::SipMessage& msg);
  virtual void onReferAccepted(resip::InviteSessionHandle, resip::ClientSubscriptionHandle, const resip::SipMessage& msg);
  virtual void onReferRejected(resip::InviteSessionHandle, const resip::SipMessage& msg);
  virtual void onReferNoSub(resip::Handle<resip::InviteSession>, const resip::SipMessage&);
  virtual void onRemoved(resip::ClientRegistrationHandle);
  virtual int onRequestRetry(resip::ClientRegistrationHandle, int retrySeconds, const resip::SipMessage& response);
  virtual void onNewSession(resip::ServerInviteSessionHandle sis, resip::InviteSession::OfferAnswerType oat, const resip::SipMessage& msg);
  virtual void onNewSession(resip::ClientInviteSessionHandle cis, resip::InviteSession::OfferAnswerType oat, const resip::SipMessage& msg);
  virtual void onTerminated(resip::InviteSessionHandle is, resip::InviteSessionHandler::TerminatedReason reason, const resip::SipMessage* msg);
  virtual void onOffer(resip::InviteSessionHandle is, const resip::SipMessage& msg, const resip::SdpContents& sdp);

};


#endif
