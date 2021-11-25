#if !defined(RemoteParticipant_hxx)
#define RemoteParticipant_hxx

#include <map>

#include "ConversationManager.hxx"
#include "Participant.hxx"
#include "RemoteParticipantDialogSet.hxx"

#include <rutil/AsyncBool.hxx>
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

class RemoteParticipant : public virtual Participant, public resip::AppDialog
{
public:
   RemoteParticipant(ParticipantHandle partHandle,   // UAC
                     ConversationManager& conversationManager, 
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet);  

   RemoteParticipant(ConversationManager& conversationManager,            // UAS or forked leg
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet);

   virtual ~RemoteParticipant();

   virtual resip::InviteSessionHandle& getInviteSessionHandle() { return mInviteSessionHandle; }

   typedef std::function<void(bool sdpOk, std::unique_ptr<resip::SdpContents> sdp)>
   ContinuationSdpReady;
   virtual void buildSdpOffer(bool holdSdp, ContinuationSdpReady c) = 0;

   virtual bool isHolding() { return mLocalHold; }
   virtual bool isRemoteHold() { return mRemoteHold; }

   virtual void initiateRemoteCall(const resip::NameAddr& destination);
   virtual void initiateRemoteCall(const resip::NameAddr& destination, const std::shared_ptr<resip::UserProfile>& callingProfile, const std::multimap<resip::Data,resip::Data>& extraHeaders);
   virtual void destroyParticipant();
   virtual void addToConversation(Conversation *conversation, unsigned int inputGain = 100, unsigned int outputGain = 100);
   virtual void removeFromConversation(Conversation *conversation);
   virtual void accept();
   virtual void alert(bool earlyFlag);
   virtual void reject(unsigned int rejectCode);
   virtual void redirect(resip::NameAddr& destination, unsigned int redirectCode = 302, ConversationManager::RedirectSuccessCondition successCondition = ConversationManager::RedirectSuccessOnConnected);
   virtual void redirectToParticipant(resip::InviteSessionHandle& destParticipantInviteSessionHandle, ConversationManager::RedirectSuccessCondition successCondition = ConversationManager::RedirectSuccessOnConnected);
   virtual void info(const resip::Contents& contents);
   virtual void checkHoldCondition();
   virtual void setLocalHold(bool hold);

   virtual void setPendingOODReferInfo(resip::ServerOutOfDialogReqHandle ood, const resip::SipMessage& referMsg); // OOD-Refer (no Sub)
   virtual void setPendingOODReferInfo(resip::ServerSubscriptionHandle ss, const resip::SipMessage& referMsg); // OOD-Refer (with Sub)
   virtual void acceptPendingOODRefer();
   virtual void rejectPendingOODRefer(unsigned int statusCode);
   virtual void redirectPendingOODRefer(resip::NameAddr& destination);
   virtual void processReferNotify(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify);

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

protected:
   void setRemoteHold(bool remoteHold);
   void setProposedSdp(const resip::SdpContents& sdp);
   void setLocalSdp(const resip::SdpContents& sdp);
   std::shared_ptr<resip::SdpContents> getLocalSdp() { return mLocalSdp; };
   void setRemoteSdp(const resip::SdpContents& sdp, bool answer=false);
   std::shared_ptr<resip::SdpContents> getRemoteSdp() { return mRemoteSdp; };

   virtual bool mediaStackPortAvailable() = 0;

   RemoteParticipantDialogSet& getDialogSet() { return mDialogSet; };

private:       
   void hold();
   void unhold();
   void provideOffer(bool postOfferAccept);
   resip::AsyncBool provideAnswer(const resip::SdpContents& offer, bool postAnswerAccept, bool postAnswerAlert);
   virtual resip::AsyncBool buildSdpAnswer(const resip::SdpContents& offer, ContinuationSdpReady c) = 0;
   virtual void replaceWithParticipant(Participant* replacingParticipant);

   resip::DialogUsageManager &mDum;
   resip::InviteSessionHandle mInviteSessionHandle;
   resip::ClientSubscriptionHandle mReferSubscriptionHandle;
   RemoteParticipantDialogSet& mDialogSet;
   resip::DialogId mDialogId;

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
   State mState;
   bool mOfferRequired;
   bool mLocalHold;
   bool mRemoteHold;
   void stateTransition(State state);

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
};

}

#endif


/* ====================================================================

 Copyright (c) 2021, Daniel Pocock https://danielpocock.com
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
