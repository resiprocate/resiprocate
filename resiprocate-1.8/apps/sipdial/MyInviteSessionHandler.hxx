

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

/* ====================================================================
 *
 * Copyright 2012 Daniel Pocock.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

