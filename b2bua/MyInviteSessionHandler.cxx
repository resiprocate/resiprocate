


#include "resip/stack/ExtensionHeader.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Tuple.hxx"
#include "rutil/Data.hxx"

#include "B2BCall.hxx"
#include "B2BCallManager.hxx"
#include "Logging.hxx"
#include "MyInviteSessionHandler.hxx"

using namespace b2bua;
using namespace resip;
using namespace std;

MyInviteSessionHandler::MyInviteSessionHandler(DialogUsageManager& dum, B2BCallManager& callManager) : dum(dum), callManager(callManager) {
}

void MyInviteSessionHandler::onSuccess(ClientRegistrationHandle h, const SipMessage& response) {
}

void MyInviteSessionHandler::onFailure(ClientRegistrationHandle, const SipMessage& msg) {
}

void MyInviteSessionHandler::onMessage(resip::Handle<resip::InviteSession>, const resip::SipMessage& msg) {
}

void MyInviteSessionHandler::onMessageSuccess(resip::Handle<resip::InviteSession>, const resip::SipMessage&) {
}

void MyInviteSessionHandler::onMessageFailure(resip::Handle<resip::InviteSession>, const resip::SipMessage&) {
}

void MyInviteSessionHandler::onFailure(ClientInviteSessionHandle cis, const SipMessage& msg) {
  B2BUA_LOG_DEBUG("onFailure: %d, %s", msg.header(h_StatusLine).statusCode(), msg.header(h_StatusLine).reason().c_str());
  B2BCall *call = getB2BCall(cis.get());
  if(call == NULL) {
    B2BUA_LOG_WARNING("onFailure: unrecognised dialog");
    return;
  }
  call->onRejected(msg.header(h_StatusLine).statusCode(), msg.header(h_StatusLine).reason());
}

void MyInviteSessionHandler::onForkDestroyed(ClientInviteSessionHandle) {
}

void MyInviteSessionHandler::onInfoSuccess(InviteSessionHandle, const SipMessage& msg) {
}

void MyInviteSessionHandler::onInfoFailure(InviteSessionHandle, const SipMessage& msg) {
}

void MyInviteSessionHandler::onProvisional(ClientInviteSessionHandle cis, const SipMessage& msg) {
  B2BCall *call = getB2BCall(cis.get());
  if(call == NULL) {
    B2BUA_LOG_WARNING("onProvisional: unrecognised dialog");
    return;
  }
  int code = msg.header(h_StatusLine).statusCode();
  switch(code) {
  case 100:
    //call->onTrying();
    break;
  case 180:
    call->onRinging();
    break;
  case 183:
    //call->onSessionProgress();
    break;
  default:
    B2BUA_LOG_DEBUG("onProvisional: unknown provisional code (%d)", code);
  } 
}

void MyInviteSessionHandler::onConnected(ClientInviteSessionHandle, const SipMessage& msg) {
  // FIXME - start charging here instead of waiting for onAnswer
}

void MyInviteSessionHandler::onStaleCallTimeout(ClientInviteSessionHandle) {
}

void MyInviteSessionHandler::onConnected(InviteSessionHandle, const SipMessage& msg) {
  // FIXME
}

void MyInviteSessionHandler::onRedirected(ClientInviteSessionHandle, const SipMessage& msg) {
}

void MyInviteSessionHandler::onAnswer(InviteSessionHandle is, const SipMessage& msg, const SdpContents& sdp) {
  MyAppDialog *myAppDialog = (MyAppDialog *)is->getAppDialog().get();
  B2BCall *call = getB2BCall(is.get());
  if(call == NULL) {
    B2BUA_LOG_WARNING("onAnswer: unrecognised dialog");
    return;
  }
  Tuple sourceTuple = msg.getSource();
  in_addr_t msgSourceAddress = sourceTuple.toGenericIPAddress().v4Address.sin_addr.s_addr;
  call->onAnswer(myAppDialog, sdp, msgSourceAddress);
}

void MyInviteSessionHandler::onEarlyMedia(ClientInviteSessionHandle cis, const SipMessage& msg, const SdpContents& sdp) {
  B2BCall *call = getB2BCall(cis.get());
  if(call == NULL) {
    B2BUA_LOG_WARNING("onEarlyMedia: unrecognised dialog");
    return;
  }
  Tuple sourceTuple = msg.getSource();
  in_addr_t msgSourceAddress = sourceTuple.toGenericIPAddress().v4Address.sin_addr.s_addr;
  call->onEarlyMedia(sdp, msgSourceAddress);
}

void MyInviteSessionHandler::onOfferRequired(InviteSessionHandle, const SipMessage& msg) {
  // FIXME
}

void MyInviteSessionHandler::onOfferRejected(Handle<InviteSession>, const SipMessage *msg) {
  // FIXME
  B2BUA_LOG_DEBUG("onOfferRejected: %d, %s", msg->header(h_StatusLine).statusCode(), msg->header(h_StatusLine).reason().c_str());
}

void MyInviteSessionHandler::onInfo(InviteSessionHandle, const SipMessage& msg) {
}

void MyInviteSessionHandler::onRefer(InviteSessionHandle, ServerSubscriptionHandle, const SipMessage& msg) {
}

void MyInviteSessionHandler::onReferAccepted(InviteSessionHandle, ClientSubscriptionHandle, const SipMessage& msg) {
}

void MyInviteSessionHandler::onReferRejected(InviteSessionHandle, const SipMessage& msg) {
}

void MyInviteSessionHandler::onReferNoSub(resip::Handle<resip::InviteSession> is, const resip::SipMessage& msg) {
}

void MyInviteSessionHandler::onRemoved(ClientRegistrationHandle) {
}

int MyInviteSessionHandler::onRequestRetry(ClientRegistrationHandle, int retrySeconds, const SipMessage& response) {
//  cerr << "onRequestRetry not implemented" << endl;
  //FIXME
  B2BUA_LOG_DEBUG("onRequestRetry not implemented");
  return -1;
}

// New inbound connection
// We must create a B2BCall and insert the B2BCall into the linked list
void MyInviteSessionHandler::onNewSession(ServerInviteSessionHandle sis, InviteSession::OfferAnswerType oat, const SipMessage& msg) {
  //cerr << "onNewSession sis" << endl;

  // Are we shutting down?  If so, reject the call with SIP code 503
  if(callManager.isStopping()) {
    B2BUA_LOG_DEBUG("rejecting inbound call as we are stopping");
    sis->reject(503);
    return;
  }

  // Check the headers
  if(!msg.exists(h_From)) {
    B2BUA_LOG_WARNING("inbound connection missing from header, rejecting dialog");
    sis->reject(603);
    return;
  }
  // FIXME - do above for all headers
  if(msg.getReceivedTransport() == 0) {
    // msg not received from the wire
    // FIXME
    B2BUA_LOG_WARNING("request not received from the wire");
    sis->reject(603);
  }
  Tuple sourceTuple = msg.getSource();
  Data sourceIp = Data(inet_ntoa(sourceTuple.toGenericIPAddress().v4Address.sin_addr));
  Data contextId;
  Data accountId;
  Data baseIp;
  Data controlId;
  ExtensionHeader xContextId("X-MyB2BUA-Context-ID");
  if(msg.exists(xContextId)) {
    const StringCategories& contextIds = msg.header(xContextId);
    contextId = Data((contextIds.begin())->value());
  }
  ExtensionHeader xAccountId("X-MyB2BUA-Account-ID");
  if(msg.exists(xAccountId)) {
    const StringCategories& accountIds = msg.header(xAccountId);
    accountId = Data((accountIds.begin())->value());
  }
  ExtensionHeader xBaseIp("X-MyB2BUA-Base-IP");
  if(msg.exists(xBaseIp)) {
    const StringCategories& baseIps = msg.header(xBaseIp);
    baseIp = Data((baseIps.begin())->value());
  }
  ExtensionHeader xControlId("X-MyB2BUA-Control-ID");
  if(msg.exists(xControlId)) {
    const StringCategories& controlIds = msg.header(xControlId);
    controlId = Data((controlIds.begin())->value());
  }
  // Now inspect the authentication info
  Data authRealm("");
  Data authUser("");
  Data authPassword(""); 
  if(msg.exists(h_ProxyAuthorizations)) {
    for(Auths::const_iterator it = msg.header(h_ProxyAuthorizations).begin(); it != msg.header(h_ProxyAuthorizations).end(); it++) {
      if(dum.isMyDomain(it->param(p_realm))) {
        authRealm = it->param(p_realm);
        authUser = it->param(p_username);
      }
    }
  }
  try {
    callManager.onNewCall((MyAppDialog *)sis->getAppDialog().get(), msg.header(h_From), msg.header(h_RequestLine).uri(), authRealm, authUser, Data(""), sourceIp, contextId, accountId, baseIp, controlId);
  } catch (...) {
    B2BUA_LOG_ERR("failed to instantiate B2BCall");
    sis->reject(500);   // Indicate temporary error condition
  }
}

void MyInviteSessionHandler::onNewSession(ClientInviteSessionHandle cis, InviteSession::OfferAnswerType oat, const SipMessage& msg) {
}

void MyInviteSessionHandler::onTerminated(InviteSessionHandle is, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg) {
  B2BUA_LOG_DEBUG("onTerminated, reason = %d", reason);
  B2BCall *call = getB2BCall(is.get());
  if(call == NULL) {
    B2BUA_LOG_WARNING("onTerminated: unrecognised dialog");
    return;
  }
  MyAppDialog *myAppDialog = (MyAppDialog *)is->getAppDialog().get();
  switch(reason) {

  // received a BYE or CANCEL from peer
  case RemoteCancel: 
    B2BUA_LOG_DEBUG("onTerminated: RemoteCancel");
    call->onCancel();
    break;

  case RemoteBye: 
    B2BUA_LOG_DEBUG("onTerminated: RemoteBye");
    // MyAppDialog *myAppDialog = (MyAppDialog *)is->getAppDialog().get();
    call->onHangup(myAppDialog);
    break;

  // ended by the application
  case LocalBye: 
    B2BUA_LOG_DEBUG("onTerminated: LocalBye");
    //call->onFailure(myAppDialog);
    break;

  case Referred: 
    B2BUA_LOG_DEBUG("onTerminated: Referred");
    //call->onFailure(myAppDialog);
    break;

  // ended due to a failure
  case Error:
    B2BUA_LOG_DEBUG("onTerminated: Error");
    call->onFailure(myAppDialog);
    break;

  case Timeout:
    B2BUA_LOG_DEBUG("onTerminated: Timeout");
    call->onFailure(myAppDialog);
    break;

  case LocalCancel: // ended by the application via Cancel
    B2BUA_LOG_DEBUG("onTerminated: LocalCancel"); 
    // no need to do anything, because we have initiated this cancel
    break;

  default:
    B2BUA_LOG_WARNING("onTerminated: unhandled case %d", reason);
    break;
  }
}

void MyInviteSessionHandler::onOffer(InviteSessionHandle is, const SipMessage& msg, const SdpContents& sdp) {
  B2BCall *call = getB2BCall(is.get());
  if(call == NULL) {
    B2BUA_LOG_WARNING("onOffer: unrecognised dialog");
    return;
  }
  B2BUA_LOG_DEBUG("onOffer received");
  MyAppDialog *myAppDialog = (MyAppDialog *)is->getAppDialog().get();
  Tuple sourceTuple = msg.getSource();
  in_addr_t msgSourceAddress = sourceTuple.toGenericIPAddress().v4Address.sin_addr.s_addr;
  call->onOffer(myAppDialog, sdp, msgSourceAddress);  
}

/**
 * Handy utility functions
 */
B2BCall *MyInviteSessionHandler::getB2BCall(InviteSession *is) {
  MyAppDialog *myAppDialog = (MyAppDialog *)is->getAppDialog().get();
  return (B2BCall *)myAppDialog->getB2BCall();
}

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

