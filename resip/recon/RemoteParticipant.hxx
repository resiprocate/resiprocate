#if !defined(RemoteParticipant_hxx)
#define RemoteParticipant_hxx

#include <map>

#include "ConversationManager.hxx"
#include "Participant.hxx"
#include "IMParticipantBase.hxx"
#include "RemoteParticipantDialogSet.hxx"

#include <resip/stack/MediaControlContents.hxx>
#include <resip/stack/TrickleIceContents.hxx>

#include <resip/dum/AppDialogSet.hxx>
#include <resip/dum/AppDialog.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/DialogSetHandler.hxx>
#include <resip/dum/SubscriptionHandler.hxx>

#include <memory>

namespace resip
{
class DialogUsageManager;
class SipMessage;
}

namespace recon
{

/**
  This class represent a remote participant.  A remote participant is a 
  participant with a network connection to a remote entity.  This
  implementation is for a SIP / RTP connections.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class RemoteParticipant : public virtual Participant, public resip::AppDialog, public IMParticipantBase
{
public:
   // UAC
   RemoteParticipant(ParticipantHandle partHandle,
                     ConversationManager& conversationManager, 
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet);

   // UAS or forked leg
   RemoteParticipant(ConversationManager& conversationManager,
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet);

   virtual ~RemoteParticipant();

   virtual resip::InviteSessionHandle& getInviteSessionHandle() { return mInviteSessionHandle; }

   typedef std::function<void(bool sdpOk, std::unique_ptr<resip::SdpContents> sdp)>
   CallbackSdpReady;
   virtual void buildSdpOffer(bool holdSdp, CallbackSdpReady sdpReady, bool preferExistingSdp = false) = 0;

   virtual bool isHolding() { return mLocalHold; }
   virtual bool isRemoteHold() { return mRemoteHold; }

   virtual void initiateRemoteCall(const resip::NameAddr& destination);
   virtual void initiateRemoteCall(const resip::NameAddr& destination, const std::shared_ptr<ConversationProfile>& callingProfile, const std::multimap<resip::Data,resip::Data>& extraHeaders);
   virtual void destroyParticipant() override;
   virtual void addToConversation(Conversation *conversation, unsigned int inputGain = 100, unsigned int outputGain = 100) override;
   virtual void removeFromConversation(Conversation *conversation) override;
   virtual void accept();
   virtual void alert(bool earlyFlag);
   virtual void reject(unsigned int rejectCode);
   virtual void redirect(resip::NameAddr& destination, unsigned int redirectCode = 302, ConversationManager::RedirectSuccessCondition successCondition = ConversationManager::RedirectSuccessOnConnected);
   virtual void redirectToParticipant(resip::InviteSessionHandle& destParticipantInviteSessionHandle, ConversationManager::RedirectSuccessCondition successCondition = ConversationManager::RedirectSuccessOnConnected);
   virtual void info(const resip::Contents& contents);
   virtual void checkHoldCondition();
   virtual void setLocalHold(bool hold);
   virtual void sendInstantMessage(std::unique_ptr<resip::Contents> contents) override;

   virtual void setPendingOODReferInfo(resip::ServerOutOfDialogReqHandle ood, const resip::SipMessage& referMsg); // OOD-Refer (no Sub)
   virtual void setPendingOODReferInfo(resip::ServerSubscriptionHandle ss, const resip::SipMessage& referMsg); // OOD-Refer (with Sub)
   virtual void acceptPendingOODRefer();
   virtual void rejectPendingOODRefer(unsigned int statusCode);
   virtual void redirectPendingOODRefer(resip::NameAddr& destination);
   virtual void processReferNotify(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify);

   virtual bool onMediaControlEvent(resip::MediaControlContents::MediaControl& mediaControl);
   virtual bool onTrickleIce(resip::TrickleIceContents& trickleIce);

   // Called by RemoteParticipantDialogSet when Related Conversations should be destroyed
   virtual void destroyConversations();
   virtual void notifyTerminating();
   virtual void adjustRTPStreams(bool sendingOffer=false) = 0;

   // Invite Session Handler /////////////////////////////////////////////////////
   virtual void onNewSession(resip::ClientInviteSessionHandle h, resip::InviteSession::OfferAnswerType oat, const resip::SipMessage& msg);
   virtual void onNewSession(resip::ServerInviteSessionHandle h, resip::InviteSession::OfferAnswerType oat, const resip::SipMessage& msg);
   virtual void onFailure(resip::ClientInviteSessionHandle h, const resip::SipMessage& msg);
   virtual void onEarlyMedia(resip::ClientInviteSessionHandle, const resip::SipMessage&, const resip::SdpContents&);
   virtual void onProvisional(resip::ClientInviteSessionHandle, const resip::SipMessage& msg);
   virtual void onConnected(resip::ClientInviteSessionHandle h, const resip::SipMessage& msg);
   virtual void onConnected(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onConnectedConfirmed(resip::InviteSessionHandle, const resip::SipMessage &msg);
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
   virtual void doReferNoSub(const resip::SipMessage& msg);
   virtual void onReferNoSub(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onMessage(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onMessageSuccess(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onMessageFailure(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onForkDestroyed(resip::ClientInviteSessionHandle);

   // ClientSubscriptionHandler ///////////////////////////////////////////////////
   virtual void onUpdatePending(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify, bool outOfOrder);
   virtual void onUpdateActive(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify, bool outOfOrder);
   virtual void onUpdateExtension(resip::ClientSubscriptionHandle, const resip::SipMessage& notify, bool outOfOrder);
   virtual void onTerminated(resip::ClientSubscriptionHandle h, const resip::SipMessage* notify);
   virtual void onNewSubscription(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify);
   virtual int onRequestRetry(resip::ClientSubscriptionHandle h, int retryMinimum, const resip::SipMessage& notify);

   virtual void requestKeyframe() = 0;
   virtual void requestKeyframeFromPeer();
   // force a SIP re-INVITE to this RemoteParticipant
   virtual void reInvite();

protected:
   void addExtraHeader(const std::shared_ptr<resip::SipMessage>& invitemsg, const resip::Data& headerName, const resip::Data& headerValue);
   void setRemoteHold(bool remoteHold);
   void setProposedSdp(const resip::SdpContents& sdp);
   void setLocalSdp(const resip::SdpContents& sdp);
   std::shared_ptr<resip::SdpContents> getLocalSdp() { return mLocalSdp; };
   void setRemoteSdp(const resip::SdpContents& sdp, bool answer=false);
   std::shared_ptr<resip::SdpContents> getRemoteSdp() { return mRemoteSdp; };
   void setLocalSdpGathering(const resip::SdpContents& sdp);
   std::shared_ptr<resip::SdpContents> getLocalSdpGathering() { return mLocalSdpGathering; };

   virtual bool mediaStackPortAvailable() = 0;

   RemoteParticipantDialogSet& getDialogSet() { return mDialogSet; };

   virtual void notifyIncomingParticipant(const resip::SipMessage& msg, bool autoAnswer, ConversationProfile& conversationProfile);
   virtual void hold();
   virtual void unhold();

   virtual bool holdPreferExistingSdp() { return false; };

   bool isTrickleIce() { return mTrickleIce; };
   virtual void enableTrickleIce();

   virtual void conversationsConfirm();

   virtual std::chrono::duration<double> getKeyframeRequestInterval() const { return mKeyframeRequestInterval; }

   virtual void onLocalIceCandidate(const resip::Data& candidate, unsigned int lineIndex, const resip::Data& mid);

private:       
   void provideOffer(bool postOfferAccept, bool preferExistingSdp = false);
   void provideAnswer(const resip::SdpContents& offer, bool postAnswerAccept, bool postAnswerAlert);
   virtual void buildSdpAnswer(const resip::SdpContents& offer, CallbackSdpReady sdpReady) = 0;
   virtual void replaceWithParticipant(Participant* replacingParticipant) override;

   resip::DialogUsageManager &mDum;
   resip::InviteSessionHandle mInviteSessionHandle;
   resip::ClientSubscriptionHandle mReferSubscriptionHandle;
   RemoteParticipantDialogSet& mDialogSet;
   resip::DialogId mDialogId;

   friend class RemoteParticipantDialogSet;
   typedef enum
   {
      Connecting=1, 
      Accepted,
      Connected,
      Redirecting,
      Holding,
      Unholding,
      Replacing,
      PendingOODRefer,
      Terminating
   } State;
   State getState() { return mState; };
   State mState;
   bool mOfferRequired;
   bool mLocalHold;
   bool mRemoteHold;
   void stateTransition(State state);
   bool mTrickleIce;

   resip::AppDialogHandle mReferringAppDialog; 

   resip::SipMessage mPendingOODReferMsg;
   resip::ServerOutOfDialogReqHandle mPendingOODReferNoSubHandle;
   resip::ServerSubscriptionHandle mPendingOODReferSubHandle;
   ConversationManager::RedirectSuccessCondition mRedirectSuccessCondition;
   
   typedef enum
   {
      None = 0,
      Hold,
      Unhold,
      Redirect,
      RedirectTo
   } PendingRequestType;
   class PendingRequest
   {
   public:
      PendingRequest() : mType(None) {}
      PendingRequestType mType;
      resip::NameAddr mDestination;
      resip::InviteSessionHandle mDestInviteSessionHandle;
      unsigned int mRedirectCode;
      ConversationManager::RedirectSuccessCondition mRedirectSuccessCondition;
   };
   PendingRequest mPendingRequest;
   std::unique_ptr<resip::SdpContents> mPendingOffer;

   std::shared_ptr<resip::SdpContents> mLocalSdp;
   std::shared_ptr<resip::SdpContents> mRemoteSdp;

   std::shared_ptr<resip::SdpContents> mLocalSdpGathering;

   std::chrono::time_point<std::chrono::steady_clock> mLastRemoteKeyframeRequest = std::chrono::steady_clock::now();
   std::chrono::duration<double> mKeyframeRequestInterval = std::chrono::milliseconds(1000);
};

}

#endif


/* ====================================================================

 Copyright (c) 2021-2023, SIP Spectrum, Inc. www.sipspectrum.com
 Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2021-2022, Daniel Pocock https://danielpocock.com
 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
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
