
#ifndef __B2BCall_h
#define __B2BCall_h

/* B2BCall models a bridged call between a caller and a callee
*/

#include "rutil/Data.hxx"
#include "resip/dum/DialogUsageManager.hxx"

namespace b2bua
{

class B2BCall;
class MyDialogSetHandler;

}

#include "AuthorizationManager.hxx"
#include "CallHandle.hxx"
#include "CDRHandler.hxx"
#include "MediaManager.hxx"
#include "MyAppDialog.hxx"


namespace b2bua
{

class B2BCall {

protected:

  typedef enum B2BCallState {
    NewCall = 0,				// just started
    CallerCancel,				// CANCEL received from A leg
    AuthorizationPending,
    AuthorizationSuccess,
    AuthorizationFail,
//    RoutesPending,				// routes requested
//    RoutesSuccess,
//    RoutesFail,
//   MediaProxyPending,				// Media Proxy requested
    MediaProxySuccess,
    MediaProxyFail,
    ReadyToDial,				// Route ready
    DialInProgress,				// INVITE sent
    DialFailed,					// Network error, e.g. ICMP
    DialRejected,				// SIP error received
						// error code in data member
						// failureStatusCode
    SelectAlternateRoute,			// Need to select another
						// route
    DialAborted,				// No other carriers 
						// available, or a failure
						// that doesn't require us
						// to try another carrier
						// e.g. Busy
//    DialReceived100,				// 100 response received
    DialReceived180,				// 180 response received
//    DialReceived183,				// 183 response received
    DialReceivedEarlyAnswer,			// early answer (SDP) received
//    DialEarlyMediaProxyRequested,		// Media proxy requested for
						// early media
    DialEarlyMediaProxySuccess,			// Media Proxy ready
    DialEarlyMediaProxyFail,			// Media Proxy failed
    CallAccepted,				// 200 received
//    CallAcceptedMediaProxyRequested,		// Media proxy requested for
						// media
    CallAcceptedMediaProxySuccess,		// Media Proxy ready
    CallAcceptedMediaProxyFail,			// Media proxy failed
    CallActive,					// Call in progress
    CallerHangup,				// Caller has hungup
    CalleeHangup,				// Callee has hungup
    LocalHangup,				// B2BUA initiated hangup
    CallStop,					// Call is stopped
//    CallStopMediaProxyNotified,			// Media proxy informed
    CallStopMediaProxySuccess,			// Media proxy acknowledged
    CallStopMediaProxyFail,			// Media proxy failed to ack
    CallStopFinal				// Call can be purged from
						// the system
  };
  
  static resip::Data callStateNames[];

  typedef enum BasicClearingReason {
    NoAnswer = 0,	// caller gave up/timeout
    Busy,		// callee indicated busy
    Congestion,		// callee indicated congestion
    Error,		// callee indicated error
    Answered		// call was answered
  };

  static const char *basicClearingReasonName[];

  typedef enum FullClearingReason {
    Unset,

    // No attempted
    InvalidDestination,
    AuthError,

    // not answered
    NoAnswerCancel,
    NoAnswerTimeout,
    NoAnswerError,	// media negotiation failed

    // Rejected
    RejectBusy,
    RejectOther,	// rejectOtherCode = SIP error code

    // Answered
    AnsweredALegHangup,	// A leg hangup first
    AnsweredBLegHangup, // B leg hangup first
    AnsweredLimit,      // reached limit
    AnsweredShutdown,   // Answered, B2BUA shutdown
    AnsweredError,      // An error occured, e.g. during a re-INVITE
    AnsweredNoMedia,	// Media stopped
    AnsweredUnknown	// Answered, hangup for unknown reason
  };

  CDRHandler& cdrHandler;
  resip::DialogUsageManager& dum;
  //CallController *callController;
  AuthorizationManager& authorizationManager;
  
  //static std::list<B2BCall *> calls;

  // Attributes of the call, from the original INVITE
  resip::NameAddr sourceAddr;
  resip::Uri destinationAddr;
  resip::Data authRealm;
  resip::Data authUser;
  resip::Data authPassword;
  resip::Data srcIp;
  resip::Data contextId;
  resip::Data accountId;
  resip::Data baseIp;
  resip::Data controlId;

  //int callState;

  B2BCallState callState;

  BasicClearingReason basicClearingReason;
  FullClearingReason fullClearingReason;
  int rejectOtherCode;

  // Call time information
  time_t startTime;
  time_t connectTime;
  time_t finishTime;

  // CallHandle from CallController
  CallHandle *callHandle;
  std::list<CallRoute *>::iterator callRoute;
  resip::Data appRef1;
  resip::Data appRef2;
 
  // If we are waiting for something, this is the timeout
  time_t timeout;

  MyAppDialog *aLegAppDialog;
  MyAppDialog *bLegAppDialog;
  MyAppDialogSet *bLegAppDialogSet;
  //resip::SdpContents *aLegSdp;		// most recent offer from A leg
  //resip::SdpContents *bLegSdp;		// most recent offer from B leg
  bool earlyAnswerSent;
  MediaManager *mediaManager;
  // resip::SharedPtr<resip::UserProfile> outboundUserProfile;

  int failureStatusCode;
  resip::Data *failureReason;

  // Returns true if successful, false if new state is not
  // permitted
  bool setCallState(B2BCallState newCallState);
  bool isCallStatePermitted(B2BCallState newCallState);
  const resip::Data& getCallStateName(B2BCallState s);

  void setALegSdp(const resip::SdpContents& sdp, const in_addr_t& msgSourceAddress);
  void setBLegSdp(const resip::SdpContents& sdp, const in_addr_t& msgSourceAddress);

  void doNewCall();
  void doCallerCancel();
  void doAuthorizationPending();
  void doAuthorizationSuccess();
  void doAuthorizationFail();
  void doMediaProxySuccess();
  void doMediaProxyFail();
  void doReadyToDial();
  void doDialFailed();
  void doDialRejected();
  void doSelectAlternateRoute();
  void doDialAborted();
  //void doDialReceived100();
  void doDialReceived180();
  //void doCallDial183();
  void doDialReceivedEarlyAnswer();
  void doDialEarlyMediaProxySuccess();
  void doDialEarlyMediaProxyFail();
  void doCallAccepted();
  void doCallAcceptedMediaProxySuccess();
  void doCallAcceptedMediaProxyFail();
  void doCallActive();
  void doHangup();
  void doCallStop();
  void doCallStopMediaProxySuccess();
  void doCallStopMediaProxyFail();
  void doCallStopFinal();

  // sets the clearing reason codes if necessary
  void setClearingReason(FullClearingReason reason, int code);
  void setClearingReasonMediaFail();

  void writeCDR();

public:

  // More basic then B2BCallState, used for reporting
  typedef enum CallStatus {
    PreDial,			// the call hasn't started dialing
    Dialing,			// the call is dialing
    Connected,			// the call has connected
    Finishing,			// the call has been hung up
    Unknown			// unknown
  };

  void checkProgress(time_t now, bool stopping);
  bool isComplete();

  CallStatus getStatus();

  //static void setDum(resip::DialogUsageManager *dum);
  //static void setCallController(CallController *callController);

  // B2BCall(MyAppDialog *aLegDialog);
  B2BCall(CDRHandler& cdrHandler, resip::DialogUsageManager& dum, AuthorizationManager& authorizationManager, MyAppDialog *aLegDialog, const resip::NameAddr& sourceAddr, const resip::Uri& destinationAddr, const resip::Data& authRealm, const resip::Data& authUser, const resip::Data& authPassword, const resip::Data& srcIp, const resip::Data& contextId, const resip::Data& accountId, const resip::Data& baseIp, const resip::Data& controlId);
  virtual ~B2BCall();

  void setBLegAppDialog(MyAppDialog *myAppDialog);

  // called every time we go through the main program loop
  //static void checkCalls();

  // For InviteSessionHandler
  //void onTrying();
  void onRinging();
  //void onSessionProgress();
  void onEarlyMedia(const resip::SdpContents& sdp, const in_addr_t& msgSourceAddress);
  void onCancel();
  void onFailure(MyAppDialog *myAppDialog);
  void onRejected(int statusCode, const resip::Data& reason);
  void onOffer(MyAppDialog *myAppDialog, const resip::SdpContents& sdp, const in_addr_t& msgSourceAddress);
  void onAnswer(MyAppDialog *myAppDialog, const resip::SdpContents& sdp, const in_addr_t& msgSourceAddress);
  void onMediaTimeout();
  void onHangup(MyAppDialog *myAppDialog);

  // For B2BCallManager
  void onStopping();

  void releaseAppDialog(MyAppDialog *myAppDialog);
  void releaseAppDialogSet(MyAppDialogSet *myAppDialogSet);

  friend class MyDialogSetHandler;

};

}

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

