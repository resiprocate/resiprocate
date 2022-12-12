#include "Call.hxx"

#include <resip/stack/SdpContents.hxx>
#include <resip/stack/PlainContents.hxx>
#include <resip/stack/SipMessage.hxx>
#include <resip/stack/ShutdownMessage.hxx>
#include <resip/stack/SipStack.hxx>
#include <resip/dum/ClientAuthManager.hxx>
#include <resip/dum/ClientInviteSession.hxx>
#include <resip/dum/ClientSubscription.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/MasterProfile.hxx>
#include <resip/dum/ServerInviteSession.hxx>
#include <resip/dum/AppDialog.hxx>
#include <resip/dum/AppDialogSet.hxx>
#include <resip/dum/AppDialogSetFactory.hxx>
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/Random.hxx>
#include <rutil/WinLeakCheck.hxx>

#include <sstream>
#include <time.h>

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;
using namespace std;

static unsigned int CallTimerTime = 30;  // Time between call timer
static unsigned int CallTimeCounterToByeOn = 6;  // BYE the call after the call timer has expired 6 times

namespace resip
{
class CallTimer : public resip::DumCommand
{
   public:
      CallTimer(UserAgent& userAgent, Call* call) : mUserAgent(userAgent), mCall(call) {}
      CallTimer(const CallTimer& rhs) : mUserAgent(rhs.mUserAgent), mCall(rhs.mCall) {}
      ~CallTimer() {}

      void executeCommand() override { mUserAgent.onCallTimeout(mCall); }

      resip::Message* clone() const override { return new CallTimer(*this); }
      EncodeStream& encode(EncodeStream& strm) const override { strm << "CallTimer:"; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const override { return encode(strm); }

   private:
      UserAgent& mUserAgent;
      Call* mCall;
};
}

Call::Call(UserAgent& userAgent) 
: AppDialogSet(userAgent.getDialogUsageManager()),
  mUserAgent(userAgent),
  mTimerExpiredCounter(0),
  mPlacedCall(false),
  mUACConnectedDialogId(Data::Empty, Data::Empty, Data::Empty)
{
   mUserAgent.registerCall(this);
}

Call::~Call()
{
   mUserAgent.unregisterCall(this);
}

void 
Call::initiateCall(const Uri& target, std::shared_ptr<UserProfile> profile, DialogId* replacesJoinDialogInfo, bool sendJoin)
{
   SdpContents offer;
   makeOffer(offer);
   auto invite = mUserAgent.getDialogUsageManager().makeInviteSession(NameAddr(target), profile, &offer, this);
   if (replacesJoinDialogInfo)
   {
      if (sendJoin)
      {
         // Add Join header
         invite->header(h_Join).value() = replacesJoinDialogInfo->getCallId();
         invite->header(h_Join).param(p_toTag) = replacesJoinDialogInfo->getLocalTag();
         invite->header(h_Join).param(p_fromTag) = replacesJoinDialogInfo->getRemoteTag();
      }
      else
      {
         // Add replaces header
         invite->header(h_Replaces).value() = replacesJoinDialogInfo->getCallId();
         invite->header(h_Replaces).param(p_toTag) = replacesJoinDialogInfo->getLocalTag();
         invite->header(h_Replaces).param(p_fromTag) = replacesJoinDialogInfo->getRemoteTag();
      }
   }
   mUserAgent.getDialogUsageManager().send(std::move(invite));
   mPlacedCall = true;
}

void 
Call::terminateCall()
{
   AppDialogSet::end(); 
}

void 
Call::timerExpired()
{
   mTimerExpiredCounter++;
   if(mTimerExpiredCounter < CallTimeCounterToByeOn)
   {
      // First few times, send a message to the other party
      //if(mInviteSessionHandle.isValid())
      //{
      //   PlainContents plain("test message");
      //   mInviteSessionHandle->message(plain);
      //}
   }
   else 
   {
      // Then hangup
      terminateCall();
   }

   // start timer for next one
   unique_ptr<ApplicationMessage> timer(new CallTimer(mUserAgent, this));
   mUserAgent.mStack->post(std::move(timer), CallTimerTime, &mUserAgent.getDialogUsageManager());
}

bool Call::toggleHold()
{
   // TODO should probably check if call is established first
   // It is assumed that the hold mode has already been toggled.
   // All that we are doing her is to force a reINVITE so that
   // the message decorator renders the SDP in the proper hold mode.
   DebugLog(<< "Forcing reINVITE for call: " << *this);
   mInviteSessionHandle->provideOfferCommand(mInviteSessionHandle->getLocalSdp());

   return(false);
}

std::shared_ptr<UserProfile> 
Call::selectUASUserProfile(const SipMessage& msg)
{
   return mUserAgent.getIncomingUserProfile(msg);
}

bool 
Call::isUACConnected()
{
   return !mUACConnectedDialogId.getCallId().empty();
}

bool 
Call::isStaleFork(const DialogId& dialogId)
{
   return (!mUACConnectedDialogId.getCallId().empty() && dialogId != mUACConnectedDialogId);
}

void 
Call::makeOffer(SdpContents& offer)
{
   static Data txt("v=0\r\n"
                   "o=- 0 0 IN IP4 0.0.0.0\r\n"
                   "s=basicClient\r\n"
                   "c=IN IP4 0.0.0.0\r\n"  
                   "t=0 0\r\n"
                   "m=audio 8000 RTP/AVP 0 101\r\n"
                   "a=rtpmap:0 pcmu/8000\r\n"
                   "a=rtpmap:101 telephone-event/8000\r\n"
                   "a=fmtp:101 0-15\r\n");

   static HeaderFieldValue hfv(txt.data(), txt.size());
   static Mime type("application", "sdp");
   static SdpContents offerSdp(hfv, type);

   offer = offerSdp;

   // Set sessionid and version for this offer
   uint64_t currentTime = Timer::getTimeMicroSec();
   offer.session().origin().getSessionId() = currentTime;
   offer.session().origin().getVersion() = currentTime;  
}

////////////////////////////////////////////////////////////////////////////////
// InviteSessionHandler      ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void
Call::onNewSession(ClientInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   InfoLog(<< "onNewSession(ClientInviteSessionHandle): msg=" << msg.brief());
   mInviteSessionHandle = h->getSessionHandle();  // Note:  each forked leg will update mInviteSession - need to set mInviteSessionHandle for final answering leg on 200
   if(mInviteSessionHandleReplaced.isValid())
   {
       // See comment in flowTerminated for an explanation of this logic
       ((Call*)mInviteSessionHandleReplaced->getAppDialogSet().get())->terminateCall();
   }
}

void
Call::onNewSession(ServerInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   InfoLog(<< "onNewSession(ServerInviteSessionHandle):  msg=" << msg.brief());
   mInviteSessionHandle = h->getSessionHandle();

   // First check if this INVITE is to replace an existing session
   if(msg.exists(h_Replaces))
   {
      pair<InviteSessionHandle, int> presult;
      presult = mDum.findInviteSession(msg.header(h_Replaces));
      if(!(presult.first == InviteSessionHandle::NotValid())) 
      {         
         Call* callToReplace = dynamic_cast<Call *>(presult.first->getAppDialogSet().get());
         InfoLog(<< "onNewSession(ServerInviteSessionHandle): replacing existing call");

         // Copy over flag that indicates if we placed the call or not
         mPlacedCall = callToReplace->mPlacedCall;

         if(mPlacedCall)
         {
            // Restart Call Timer
            unique_ptr<ApplicationMessage> timer(new CallTimer(mUserAgent, this));
            mUserAgent.mStack->post(std::move(timer), CallTimerTime, &mUserAgent.getDialogUsageManager());
         }

         // Session to replace was found - end old session
         callToReplace->end();
      }
      else
      {
          // Session to replace not found - reject it
          h->reject(481 /* Call/Transaction Does Not Exist */);
      }
   }
}

void
Call::onFailure(ClientInviteSessionHandle h, const SipMessage& msg)
{
   WarningLog(<< "onFailure: msg=" << msg.brief());

   if (msg.isResponse()) 
   {
      switch(msg.header(h_StatusLine).statusCode()) 
      {
         case 408:
         case 503:
            if(!msg.isFromWire())
            {
               // Try another flow? 
            }
         default:
            break;
      }
   }
}

void
Call::onEarlyMedia(ClientInviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   InfoLog(<< "onEarlyMedia: msg=" << msg.brief() << ", sdp=" << sdp);
}

void
Call::onProvisional(ClientInviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onProvisional: msg=" << msg.brief());

   if(isStaleFork(h->getDialogId()))
   {
      // If we receive a response from a stale fork (ie. after someone sends a 200), then we want to ignore it
      InfoLog(<< "onProvisional: from stale fork, msg=" << msg.brief());
      return;
   }
   InfoLog(<< "onProvisional: msg=" << msg.brief());
}

void
Call::onConnected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onConnected: msg=" << msg.brief());
   if(!isUACConnected())
   {
      // It is possible in forking scenarios to get multiple 200 responses, if this is 
      // our first 200 response, then this is the leg we accept, store the connected DialogId
      mUACConnectedDialogId = h->getDialogId();
      // Note:  each forked leg will update mInviteSessionHandle (in onNewSession call) - need to set mInviteSessionHandle for final answering leg on 200
      mInviteSessionHandle = h->getSessionHandle();  

      // start call timer
      unique_ptr<ApplicationMessage> timer(new CallTimer(mUserAgent, this));
      mUserAgent.mStack->post(std::move(timer), CallTimerTime, &mUserAgent.getDialogUsageManager());
   }
   else
   {
      // We already have a connected leg - end this one with a BYE
      h->end();
   }
}

void
Call::onConnected(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onConnected: msg=" << msg.brief());
}

void
Call::onStaleCallTimeout(ClientInviteSessionHandle h)
{
   WarningLog(<< "onStaleCallTimeout");
}

void
Call::onTerminated(InviteSessionHandle h, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg)
{
   Data reasonData;

   switch(reason)
   {
   case InviteSessionHandler::RemoteBye:
      reasonData = "received a BYE from peer";
      break;
   case InviteSessionHandler::RemoteCancel:
      reasonData = "received a CANCEL from peer";
      break;   
   case InviteSessionHandler::Rejected:
      reasonData = "received a rejection from peer";
      break;
   case InviteSessionHandler::LocalBye:
      reasonData = "ended locally via BYE";
      break;
   case InviteSessionHandler::LocalCancel:
      reasonData = "ended locally via CANCEL";
      break;
   case InviteSessionHandler::Replaced:
      reasonData = "ended due to being replaced";
      break;
   case InviteSessionHandler::Referred:
      reasonData = "ended due to being referred";
      break;
   case InviteSessionHandler::Error:
      reasonData = "ended due to an error";
      break;
   case InviteSessionHandler::Timeout:
      reasonData = "ended due to a timeout";
      break;
   default:
      assert(false);
      break;
   }

   if(isStaleFork(h->getDialogId()))
   {
      // If we receive a response from a stale fork (ie. after someone sends a 200), then we want to ignore it
      if(msg)
      {
         InfoLog(<< "onTerminated: from stale fork, reason=" << reasonData << ", msg=" << msg->brief());
      }
      else
      {
         InfoLog(<< "onTerminated: from stale fork, reason=" << reasonData);
      }
      return;
   }

   if(msg)
   {
      InfoLog(<< "onTerminated: reason=" << reasonData << ", msg=" << msg->brief());
   }
   else
   {
      InfoLog(<< "onTerminated: reason=" << reasonData);
   }
}

void
Call::onRedirected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   // DUM will recurse on redirect requests, so nothing to do here
   InfoLog(<< "onRedirected: msg=" << msg.brief());
}

void
Call::onAnswer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   if(isStaleFork(h->getDialogId()))
   {
      // If we receive a response from a stale fork (ie. after someone sends a 200), then we want to ignore it
      InfoLog(<< "onAnswer: from stale fork, msg=" << msg.brief() << ", sdp=" << sdp);
      return;
   }
   InfoLog(<< "onAnswer: msg=" << msg.brief() << ", sdp=" << sdp);

   // Process Answer here
}

void
Call::onOffer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{         
   if(isStaleFork(h->getDialogId()))
   {
      // If we receive a response from a stale fork (ie. after someone sends a 200), then we want to ignore it
      InfoLog(<< "onOffer: from stale fork, msg=" << msg.brief() << ", sdp=" << sdp);
      return;
   }
   InfoLog(<< "onOffer: msg=" << msg.brief() << ", sdp=" << sdp);

   // If auto answer is disabled, send a 180 and let it ring forever
   if(mUserAgent.ringNoAnswer())
   {
      ServerInviteSession* uas = dynamic_cast<ServerInviteSession*>(h.get());
      uas->provisional(180);
   }
   // Default is to answer any incoming INVITE
   else
   {
      // Provide Answer here - for test client just echo back same SDP as received for now
      h->provideAnswer(sdp);
      ServerInviteSession* uas = dynamic_cast<ServerInviteSession*>(h.get());
      if(uas && !uas->isAccepted())
      {
         uas->accept();
      }
   }
}

void
Call::onOfferRequired(InviteSessionHandle h, const SipMessage& msg)
{
   if(isStaleFork(h->getDialogId()))
   {
      // If we receive a response from a stale fork (ie. after someone sends a 200), then we want to ignore it
      InfoLog(<< "onOfferRequired: from stale fork, msg=" << msg.brief());
      return;
   }
   InfoLog(<< "onOfferRequired: msg=" << msg.brief());

   // Provide Offer Here
   SdpContents offer;
   makeOffer(offer);

   h->provideOffer(offer);
}

void
Call::onOfferRejected(InviteSessionHandle h, const SipMessage* msg)
{
   if(isStaleFork(h->getDialogId()))
   {
      // If we receive a response from a stale fork (ie. after someone sends a 200), then we want to ignore it
      if(msg)
      {
         WarningLog(<< "onOfferRejected: from stale fork, msg=" << msg->brief());
      }
      else
      {
         WarningLog(<< "onOfferRejected: from stale fork");
      }
      return;
   }
   if(msg)
   {
      WarningLog(<< "onOfferRejected: msg=" << msg->brief());
   }
   else
   {
      WarningLog(<< "onOfferRejected");
   }
}

void
Call::onOfferRequestRejected(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onOfferRequestRejected: msg=" << msg.brief());
   // This is called when we are waiting to resend a INVITE with no sdp after a glare condition, and we 
   // instead receive an inbound INVITE or UPDATE
}

void
Call::onRemoteSdpChanged(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   /// called when a modified SDP is received in a 2xx response to a
   /// session-timer reINVITE. Under normal circumstances where the response
   /// SDP is unchanged from current remote SDP no handler is called
   /// There is not much we can do about this.  If session timers are used then they are managed separately per leg
   /// and we have no real mechanism to notify the other peer of new SDP without starting a new offer/answer negotiation
   InfoLog(<< "onRemoteSdpChanged: msg=" << msg << ", sdp=" << sdp);

   // Process SDP Answer here
}

void
Call::onInfo(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onInfo: msg=" << msg.brief());

   // Handle message here
   h->acceptNIT();
}

void
Call::onInfoSuccess(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onInfoSuccess: msg=" << msg.brief());
}

void
Call::onInfoFailure(InviteSessionHandle h, const SipMessage& msg)
{
   WarningLog(<< "onInfoFailure: msg=" << msg.brief());
}

void
Call::onRefer(InviteSessionHandle h, ServerSubscriptionHandle ss, const SipMessage& msg)
{
   InfoLog(<< "onRefer: msg=" << msg.brief());

   // Handle Refer request here
}

void
Call::onReferAccepted(InviteSessionHandle h, ClientSubscriptionHandle csh, const SipMessage& msg)
{
   InfoLog(<< "onReferAccepted: msg=" << msg.brief());
}

void
Call::onReferRejected(InviteSessionHandle h, const SipMessage& msg)
{
   WarningLog(<< "onReferRejected: msg=" << msg.brief());
}

void
Call::onReferNoSub(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onReferNoSub: msg=" << msg.brief());

   // Handle Refer request with (no-subscription indication) here
}

void
Call::onMessage(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onMessage: msg=" << msg.brief());

   // Handle message here
   h->acceptNIT();

   if(!mPlacedCall)
   {
      // If we didn't place the call - answer the message with another message
      PlainContents plain("test message answer");
      h->message(plain);
   }
}

void
Call::onMessageSuccess(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onMessageSuccess: msg=" << msg.brief());
}

void
Call::onMessageFailure(InviteSessionHandle h, const SipMessage& msg)
{
   WarningLog(<< "onMessageFailure: msg=" << msg.brief());
}

void
Call::onForkDestroyed(ClientInviteSessionHandle h)
{
   InfoLog(<< "onForkDestroyed:");
}

void 
Call::onReadyToSend(InviteSessionHandle h, SipMessage& msg)
{
}

void 
Call::onFlowTerminated(InviteSessionHandle h)
{
   if(h->isConnected())
   {
      NameAddr inviteWithReplacesTarget;
      if(h->remoteTarget().uri().exists(p_gr))
      {
         // If remote contact is a GRUU then use it
         inviteWithReplacesTarget.uri() = h->remoteTarget().uri();
      }
      else
      {
         //.Use remote AOR
         inviteWithReplacesTarget.uri() = h->peerAddr().uri();
      }
      InfoLog(<< "BasicClientCall::onFlowTerminated: trying INVITE w/replaces to " << inviteWithReplacesTarget);
      // The flow terminated - try an Invite (with Replaces) to recover the call
      Call *replacesCall = new Call(mUserAgent);      

      // Copy over flag that indicates whether original call was placed or received
      replacesCall->mPlacedCall = mPlacedCall;  

      // Note:  We want to end this call since it is to be replaced.  Normally the endpoint
      // receiving the INVITE with replaces would send us a BYE for the session being replaced.
      // However, since the old flow is dead, we will never see this BYE.  We need this call to
      // go away somehow, however we cannot just end it directly here via terminateCall.
      // Since the flow to other party is likely fine - if we terminate this call now the BYE 
      // is very likely to make it to the far end, before the above INVITE - if this happens then 
      // the replaces logic of the INVITE will have no effect.  We want to delay the release of 
      // this call, by passing our handle to the new INVITE call and have it terminate this call, 
      // once we know the far end has processed our new INVITE.
      replacesCall->mInviteSessionHandleReplaced = mInviteSessionHandle;

      SdpContents offer;
      replacesCall->makeOffer(offer);
      auto invite = mUserAgent.getDialogUsageManager().makeInviteSession(inviteWithReplacesTarget, h, getUserProfile(), &offer, replacesCall);
      mUserAgent.getDialogUsageManager().send(std::move(invite));
   }
}

////////////////////////////////////////////////////////////////////////////////
// DialogSetHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
Call::onTrying(AppDialogSetHandle h, const SipMessage& msg)
{
   InfoLog(<< "onTrying: msg=" << msg.brief());
   if(isUACConnected()) return;  // Ignore 100's if already connected

   // Handle message here
}

void 
Call::onNonDialogCreatingProvisional(AppDialogSetHandle h, const SipMessage& msg)
{
   InfoLog(<< "onNonDialogCreatingProvisional: msg=" << msg.brief());
   if(isUACConnected()) return;  // Ignore provisionals if already connected

   // Handle message here
}

////////////////////////////////////////////////////////////////////////////////
// ClientSubscriptionHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
Call::onUpdatePending(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   InfoLog(<< "onUpdatePending(ClientSubscriptionHandle): " << msg.brief());
   if (msg.exists(h_Event) && msg.header(h_Event).value() == "refer")
   {
      //process Refer Notify Here
   }
   h->acceptUpdate();
}

void
Call::onUpdateActive(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   InfoLog(<< "onUpdateActive(ClientSubscriptionHandle): " << msg.brief());
   if (msg.exists(h_Event) && msg.header(h_Event).value() == "refer")
   {
      //process Refer Notify Here
   }
   h->acceptUpdate();
}

void
Call::onUpdateExtension(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   InfoLog(<< "onUpdateExtension(ClientSubscriptionHandle): " << msg.brief());
   if (msg.exists(h_Event) && msg.header(h_Event).value() == "refer")
   {
      //process Refer Notify Here
   }
   h->acceptUpdate();
}

void 
Call::onNotifyNotReceived(resip::ClientSubscriptionHandle h)
{
   InfoLog(<< "onNotifyNotReceived(ClientSubscriptionHandle)");
   h->end();
}

void
Call::onTerminated(ClientSubscriptionHandle h, const SipMessage* msg)
{
   if(msg)
   {
      InfoLog(<< "onTerminated(ClientSubscriptionHandle): " << msg->brief());
      //Note:  Final notify is sometimes only passed in the onTerminated callback
      if (msg->isRequest() && msg->exists(h_Event) && msg->header(h_Event).value() == "refer")
      {
         //process Refer Notify Here
      }
   }
   else
   {
      InfoLog(<< "onTerminated(ClientSubscriptionHandle)");
   }
}

void
Call::onNewSubscription(ClientSubscriptionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onNewSubscription(ClientSubscriptionHandle): " << msg.brief());
}

int 
Call::onRequestRetry(ClientSubscriptionHandle h, int retrySeconds, const SipMessage& msg)
{
   InfoLog(<< "onRequestRetry(ClientSubscriptionHandle): " << msg.brief());
   return -1;
}

void 
Call::onRedirectReceived(AppDialogSetHandle h, const SipMessage& msg)
{
   InfoLog(<< "onRedirectReceived: msg=" << msg.brief());
}


/* ====================================================================

 Copyright (c) 2021, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
