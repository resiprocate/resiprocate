#include "basicClientCall.hxx"

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

BasicClientCall::BasicClientCall(BasicClientUserAgent& userAgent) 
: AppDialogSet(userAgent.getDialogUsageManager()),
  mUserAgent(userAgent),
  mUACConnectedDialogId(Data::Empty, Data::Empty, Data::Empty)
{
}

BasicClientCall::~BasicClientCall()
{
}

SharedPtr<UserProfile> 
BasicClientCall::selectUASUserProfile(const SipMessage& msg)
{
	return mUserAgent.getIncomingUserProfile(msg);
}

bool 
BasicClientCall::isUACConnected()
{
   return !mUACConnectedDialogId.getCallId().empty();
}

bool 
BasicClientCall::isStaleFork(const DialogId& dialogId)
{
   return (!mUACConnectedDialogId.getCallId().empty() && dialogId != mUACConnectedDialogId);
}

////////////////////////////////////////////////////////////////////////////////
// InviteSessionHandler      ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void
BasicClientCall::onNewSession(ClientInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   InfoLog(<< "onNewSession(ClientInviteSessionHandle): msg=" << msg.brief());
   mInviteSessionHandle = h->getSessionHandle();  // Note:  each forked leg will update mInviteSession - need to set mInviteSessionHandle for final answering leg on 200
}

void
BasicClientCall::onNewSession(ServerInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   InfoLog(<< "onNewSession(ServerInviteSessionHandle):  msg=" << msg.brief());
   mInviteSessionHandle = h->getSessionHandle();
}

void
BasicClientCall::onFailure(ClientInviteSessionHandle h, const SipMessage& msg)
{
   WarningLog(<< "onFailure: msg=" << msg.brief());

   if (msg.isResponse()) 
   {
      switch(msg.header(h_StatusLine).statusCode()) 
      {
         case 408:
         case 503:
            if(msg.getReceivedTransport() == 0)
            {
               // Try another flow? TODO
            }
         default:
            break;
      }
   }
}

void
BasicClientCall::onEarlyMedia(ClientInviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   InfoLog(<< "onEarlyMedia: msg=" << msg.brief() << ", sdp=" << sdp);
}

void
BasicClientCall::onProvisional(ClientInviteSessionHandle h, const SipMessage& msg)
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
BasicClientCall::onConnected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onConnected: msg=" << msg.brief());
   if(!isUACConnected())
   {
      // It is possible in forking scenarios to get multiple 200 responses, if this is 
      // our first 200 response, then this is the leg we accept, store the connected DialogId
      mUACConnectedDialogId = h->getDialogId();
      // Note:  each forked leg will update mInviteSessionHandle (in onNewSession call) - need to set mInviteSessionHandle for final answering leg on 200
      mInviteSessionHandle = h->getSessionHandle();  
   }
   else
   {
      // We already have a connected leg - end this one with a BYE
      h->end();
   }
}

void
BasicClientCall::onConnected(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onConnected: msg=" << msg.brief());
}

void
BasicClientCall::onStaleCallTimeout(ClientInviteSessionHandle h)
{
   WarningLog(<< "onStaleCallTimeout");
}

void
BasicClientCall::onTerminated(InviteSessionHandle h, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg)
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
BasicClientCall::onRedirected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   // DUM will recurse on redirect requests, so nothing to do here
   InfoLog(<< "onRedirected: msg=" << msg.brief());
}

void
BasicClientCall::onAnswer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
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
BasicClientCall::onOffer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{         
   if(isStaleFork(h->getDialogId()))
   {
      // If we receive a response from a stale fork (ie. after someone sends a 200), then we want to ignore it
      InfoLog(<< "onOffer: from stale fork, msg=" << msg.brief() << ", sdp=" << sdp);
      return;
   }
   InfoLog(<< "onOffer: msg=" << msg.brief() << ", sdp=" << sdp);

   // Provide Answer here - for test client just echo back same SDP as received for now
   h->provideAnswer(sdp);
   ServerInviteSession* uas = dynamic_cast<ServerInviteSession*>(h.get());
   if(uas && !uas->isAccepted())
   {
      uas->accept();
   }
}

void
BasicClientCall::onOfferRequired(InviteSessionHandle h, const SipMessage& msg)
{
   if(isStaleFork(h->getDialogId()))
   {
      // If we receive a response from a stale fork (ie. after someone sends a 200), then we want to ignore it
      InfoLog(<< "onOfferRequired: from stale fork, msg=" << msg.brief());
      return;
   }
   InfoLog(<< "onOfferRequired: msg=" << msg.brief());

   // Provide Offer Here
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
   SdpContents sdp(&hfv, type);

   // Set sessionid and version for this offer
   UInt64 currentTime = Timer::getTimeMicroSec();
   sdp.session().origin().getSessionId() = currentTime;
   sdp.session().origin().getVersion() = currentTime;  

   h->provideOffer(sdp);
}

void
BasicClientCall::onOfferRejected(InviteSessionHandle h, const SipMessage* msg)
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
BasicClientCall::onOfferRequestRejected(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onOfferRequestRejected: msg=" << msg.brief());
   // This is called when we are waiting to resend a INVITE with no sdp after a glare condition, and we 
   // instead receive an inbound INVITE or UPDATE
}

void
BasicClientCall::onRemoteSdpChanged(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   /// called when a modified SDP is received in a 2xx response to a
   /// session-timer reINVITE. Under normal circumstances where the response
   /// SDP is unchanged from current remote SDP no handler is called
   /// There is not much we can do about this.  If session timers are used then they are managed seperately per leg
   /// and we have no real mechanism to notify the other peer of new SDP without starting a new offer/answer negotiation
   InfoLog(<< "onRemoteSdpChanged: msg=" << msg << ", sdp=" << sdp);

   // Process SDP Answer here
}

void
BasicClientCall::onInfo(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onInfo: msg=" << msg.brief());

   // Handle message here
   h->acceptNIT();
}

void
BasicClientCall::onInfoSuccess(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onInfoSuccess: msg=" << msg.brief());
}

void
BasicClientCall::onInfoFailure(InviteSessionHandle h, const SipMessage& msg)
{
   WarningLog(<< "onInfoFailure: msg=" << msg.brief());
}

void
BasicClientCall::onRefer(InviteSessionHandle h, ServerSubscriptionHandle ss, const SipMessage& msg)
{
   InfoLog(<< "onRefer: msg=" << msg.brief());

   // Handle Refer request here
}

void
BasicClientCall::onReferAccepted(InviteSessionHandle h, ClientSubscriptionHandle csh, const SipMessage& msg)
{
   InfoLog(<< "onReferAccepted: msg=" << msg.brief());
}

void
BasicClientCall::onReferRejected(InviteSessionHandle h, const SipMessage& msg)
{
   WarningLog(<< "onReferRejected: msg=" << msg.brief());
}

void
BasicClientCall::onReferNoSub(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onReferNoSub: msg=" << msg.brief());

   // Handle Refer request with (no-subscription indication) here
}

void
BasicClientCall::onMessage(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onMessage: msg=" << msg.brief());

   // Handle message here
   h->acceptNIT();
}

void
BasicClientCall::onMessageSuccess(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onMessageSuccess: msg=" << msg.brief());
}

void
BasicClientCall::onMessageFailure(InviteSessionHandle h, const SipMessage& msg)
{
   WarningLog(<< "onMessageFailure: msg=" << msg.brief());
}

void
BasicClientCall::onForkDestroyed(ClientInviteSessionHandle h)
{
   InfoLog(<< "onForkDestroyed:");
}

void 
BasicClientCall::onReadyToSend(InviteSessionHandle h, SipMessage& msg)
{
}

////////////////////////////////////////////////////////////////////////////////
// DialogSetHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
BasicClientCall::onTrying(AppDialogSetHandle h, const SipMessage& msg)
{
   InfoLog(<< "onTrying: msg=" << msg.brief());
   if(isUACConnected()) return;  // Ignore 100's if already connected

   // Handle message here
}

void 
BasicClientCall::onNonDialogCreatingProvisional(AppDialogSetHandle h, const SipMessage& msg)
{
   InfoLog(<< "onNonDialogCreatingProvisional: msg=" << msg.brief());
   if(isUACConnected()) return;  // Ignore provionals if already connected

   // Handle message here
}

////////////////////////////////////////////////////////////////////////////////
// ClientSubscriptionHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
BasicClientCall::onUpdatePending(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   InfoLog(<< "onUpdatePending(ClientSubscriptionHandle): " << msg.brief());
   if (msg.exists(h_Event) && msg.header(h_Event).value() == "refer")
   {
      //process Refer Notify Here
   }
   h->acceptUpdate();
}

void
BasicClientCall::onUpdateActive(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   InfoLog(<< "onUpdateActive(ClientSubscriptionHandle): " << msg.brief());
   if (msg.exists(h_Event) && msg.header(h_Event).value() == "refer")
   {
      //process Refer Notify Here
   }
   h->acceptUpdate();
}

void
BasicClientCall::onUpdateExtension(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   InfoLog(<< "onUpdateExtension(ClientSubscriptionHandle): " << msg.brief());
   if (msg.exists(h_Event) && msg.header(h_Event).value() == "refer")
   {
      //process Refer Notify Here
   }
   h->acceptUpdate();
}

void 
BasicClientCall::onNotifyNotReceived(resip::ClientSubscriptionHandle h)
{
   InfoLog(<< "onNotifyNotReceived(ClientSubscriptionHandle)");
   h->end();
}

void
BasicClientCall::onTerminated(ClientSubscriptionHandle h, const SipMessage* msg)
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
BasicClientCall::onNewSubscription(ClientSubscriptionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onNewSubscription(ClientSubscriptionHandle): " << msg.brief());
}

int 
BasicClientCall::onRequestRetry(ClientSubscriptionHandle h, int retrySeconds, const SipMessage& msg)
{
   InfoLog(<< "onRequestRetry(ClientSubscriptionHandle): " << msg.brief());
   return -1;
}

void 
BasicClientCall::onRedirectReceived(AppDialogSetHandle h, const SipMessage& msg)
{
   InfoLog(<< "onRedirectReceived: msg=" << msg.brief());
}


/* ====================================================================

 Copyright (c) 2011, SIP Spectrum, Inc.
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
