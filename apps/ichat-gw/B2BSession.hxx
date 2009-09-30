#if !defined(B2BSession_hxx)
#define B2BSession_hxx

#include <rutil/Log.hxx>
#include <rutil/SharedPtr.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/DialogSetHandler.hxx>
#include <resip/dum/AppDialogSet.hxx>

namespace gateway
{

class Server;
class IChatIPPortData;

typedef unsigned int B2BSessionHandle;

class B2BSession : public resip::AppDialogSet
{
public:
   B2BSession(Server& server, bool hasDialogSet=true);
   virtual ~B2BSession();

   virtual void end();

   void setPeer(B2BSession* peer);
   B2BSession* getPeer();
   void stealPeer(B2BSession* victimSession);

   B2BSessionHandle getB2BSessionHandle() const { return mHandle; }

   void initiateIChatCallRequest(const std::string& to, const std::string& from);
   bool createNewPeer(const resip::Uri& destinationUri, const resip::NameAddr& from, const resip::SdpContents *sdp);
   void startIChatCall(const resip::Uri& destinationUri, const resip::NameAddr& from, const resip::SdpContents *sdp);
   void startSIPCall(const resip::Uri& destinationUri, const resip::NameAddr& from, const resip::SdpContents *sdp);
   bool checkIChatCallMatch(const resip::SipMessage& msg);

   // iChat call notifications from Jabber Component
   void notifyIChatCallCancelled();
   void notifyIChatCallProceeding(const std::string& to);
   void notifyIChatCallFailed(unsigned int statusCode);
   void continueIChatCall(const std::string& remoteIPPortListBlob);
   void timeoutIChatCall();

   // IPC messages for Jabber Component
   void initiateIChatCall();
   void cancelIChatCall();
   void proceedingIChatCall();
   void acceptIChatCall();
   void rejectIChatCall();

protected:
   friend class Server;

   virtual resip::SharedPtr<resip::UserProfile> selectUASUserProfile(const resip::SipMessage& msg); 

   // Invite Session Handler /////////////////////////////////////////////////////
   virtual void onNewSession(resip::ClientInviteSessionHandle h, resip::InviteSession::OfferAnswerType oat, const resip::SipMessage& msg);
   virtual void onNewSession(resip::ServerInviteSessionHandle h, resip::InviteSession::OfferAnswerType oat, const resip::SipMessage& msg);
   virtual void onFailure(resip::ClientInviteSessionHandle h, const resip::SipMessage& msg);
   virtual void onEarlyMedia(resip::ClientInviteSessionHandle, const resip::SipMessage&, const resip::SdpContents&);
   virtual void onProvisional(resip::ClientInviteSessionHandle, const resip::SipMessage& msg);
   virtual void onConnected(resip::ClientInviteSessionHandle h, const resip::SipMessage& msg);
   virtual void onConnected(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onStaleCallTimeout(resip::ClientInviteSessionHandle);
   virtual void onTerminated(resip::InviteSessionHandle h, resip::InviteSessionHandler::TerminatedReason reason, const resip::SipMessage* msg);
   virtual void onRedirected(resip::ClientInviteSessionHandle, const resip::SipMessage& msg);
   virtual void onAnswer(resip::InviteSessionHandle, const resip::SipMessage& msg, const resip::SdpContents&);
   virtual void onOffer(resip::InviteSessionHandle handle, const resip::SipMessage& msg, const resip::SdpContents& offer);
   virtual void onOfferRequired(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onOfferRejected(resip::InviteSessionHandle, const resip::SipMessage* msg);
   virtual void onOfferRequestRejected(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onRemoteSdpChanged(resip::InviteSessionHandle, const resip::SipMessage& msg, const resip::SdpContents& sdp);
   virtual void onInfo(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onInfoSuccess(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onInfoFailure(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onRefer(resip::InviteSessionHandle, resip::ServerSubscriptionHandle, const resip::SipMessage& msg);
   virtual void onReferAccepted(resip::InviteSessionHandle, resip::ClientSubscriptionHandle, const resip::SipMessage& msg);
   virtual void onReferRejected(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual bool doReferNoSub(const resip::SipMessage& msg);
   virtual void onReferNoSub(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onMessage(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onMessageSuccess(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onMessageFailure(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onForkDestroyed(resip::ClientInviteSessionHandle);

   // DialogSetHandler  //////////////////////////////////////////////
   virtual void onTrying(resip::AppDialogSetHandle, const resip::SipMessage& msg);
   virtual void onNonDialogCreatingProvisional(resip::AppDialogSetHandle, const resip::SipMessage& msg);

   // ClientSubscriptionHandler ///////////////////////////////////////////////////
   virtual void onUpdatePending(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify, bool outOfOrder);
   virtual void onUpdateActive(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify, bool outOfOrder);
   virtual void onUpdateExtension(resip::ClientSubscriptionHandle, const resip::SipMessage& notify, bool outOfOrder);
   virtual void onTerminated(resip::ClientSubscriptionHandle h, const resip::SipMessage* notify);
   virtual void onNewSubscription(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify);
   virtual int  onRequestRetry(resip::ClientSubscriptionHandle h, int retryMinimum, const resip::SipMessage& notify);

private:
   bool buildLocalOffer(resip::SdpContents& sdp);
   bool buildLocalAnswer(resip::SdpContents& sdp);
   bool provideLocalAnswer(void);
   void endPeer();
   bool isUACConnected();
   bool isStaleFork(const resip::DialogId& dialogId);
   void fixupSdp(const resip::SdpContents& origSdp, resip::SdpContents& fixedSdp);
   unsigned int mapRejectionCodeForIChat(unsigned int statusCode);

   Server& mServer;
   bool mHasDialogSet;
   resip::DialogUsageManager& mDum;
   B2BSession* mPeer;
   resip::InviteSessionHandle mInviteSessionHandle;
   B2BSessionHandle mHandle;
   resip::AppDialogSetHandle mReferringAppDialogSet;

   // State information
   resip::DialogId mUACConnectedDialogId;
   bool mWaitingOfferFromPeer;
   bool mWaitingAnswerFromPeer;
   bool mWaitingNitAnswerFromPeer;
   bool mAnchorMedia;
   unsigned short mMediaRelayPort;

   // IChat specific endpoint info
   bool mIChatEndpoint;
   bool mIChatWaitingToAccept;
   bool mIChatWaitingToProceed;
   bool mIChatWaitingToContinue;
   resip::Uri mIChatDestination;
   resip::NameAddr mIChatFrom;
   resip::SdpContents* mIChatSdp;
   std::string mIChatCallToJID;
   std::string mIChatCallFromJID;
};
 
}

#endif

/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
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

