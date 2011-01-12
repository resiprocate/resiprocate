#if !defined(RemoteParticipant_hxx)
#define RemoteParticipant_hxx

#include "ConversationManager.hxx"
#include "Participant.hxx"
#include "media/RtpStream.hxx"
#include "RemoteParticipantDialogSet.hxx"

#include <resip/dum/AppDialogSet.hxx>
#include <resip/dum/AppDialog.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/DialogSetHandler.hxx>
#include <resip/dum/SubscriptionHandler.hxx>

namespace resip
{
class DialogUsageManager;
class SipMessage;
}

namespace sdpcontainer
{
class Sdp; 
class SdpMediaLine;
}

namespace recon
{
class ConversationManager;

/**
  This class represent a remote participant.  A remote participant is a 
  participant with a network connection to a remote entity.  This
  implementation is for a SIP / RTP connections.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class RemoteParticipant : public Participant, public resip::AppDialog
{
public:
   RemoteParticipant(ParticipantHandle partHandle,   // UAC
                     ConversationManager& conversationManager, 
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet);  

   RemoteParticipant(ConversationManager& conversationManager,            // UAS or forked leg
                     resip::DialogUsageManager& dum,
                     RemoteParticipantDialogSet& remoteParticipantDialogSet,
                     const RemoteParticipant* orig = NULL);

   virtual ~RemoteParticipant();

   virtual resip::InviteSessionHandle& getInviteSessionHandle() { return mInviteSessionHandle; }
   virtual unsigned int getLocalRTPPort( const sdpcontainer::SdpMediaLine::SdpMediaType& mediaType, ConversationProfile* profile = NULL );
   typedef std::map<sdpcontainer::SdpMediaLine::SdpMediaType, bool> MediaHoldStateMap;
   void buildSdpOffer(ConversationProfile* profile, MediaHoldStateMap holdStates, resip::SdpContents& offer, std::set<sdpcontainer::SdpMediaLine::SdpMediaType> existingMediaTypes=std::set<sdpcontainer::SdpMediaLine::SdpMediaType>());
   virtual bool isHolding();

   virtual void initiateRemoteCall(resip::SharedPtr<ConversationProfile> profile, const resip::NameAddr& destination, Conversation* conversation, const ConversationManager::MediaAttributes& mediaAttributes, const ConversationManager::CallAttributes& callAttributes);
   virtual int getConnectionPortOnBridge();
   virtual boost::shared_ptr<RtpStream> getRtpStream( sdpcontainer::SdpMediaLine::SdpMediaType mediaType ) { return mDialogSet.getRtpStream( mediaType ); }

   virtual int getMediaConnectionId();
   virtual void destroyParticipant(const resip::Data& appDefinedReason = resip::Data::Empty);
   virtual void addToConversation(Conversation* conversation, unsigned int inputGain = 100, unsigned int outputGain = 100);
   virtual void removeFromConversation(Conversation* conversation, bool bTriggerHold );
   virtual void accept(ConversationManager::MediaAttributes mediaAttributes);
   virtual void alert(bool earlyFlag);
   virtual void reject(unsigned int rejectCode);
   virtual void redirect(resip::NameAddr& destination);
   virtual void redirectToParticipant(resip::InviteSessionHandle& destParticipantInviteSessionHandle);
   virtual void checkHoldCondition();
   virtual void updateMedia(ConversationManager::MediaAttributes mediaAttributes, bool sendOffer);

   virtual void setPendingOODReferInfo(resip::ServerOutOfDialogReqHandle ood, const resip::SipMessage& referMsg); // OOD-Refer (no Sub)
   virtual void setPendingOODReferInfo(resip::ServerSubscriptionHandle ss, const resip::SipMessage& referMsg); // OOD-Refer (with Sub)
   virtual void acceptPendingOODRefer(ConversationManager::MediaAttributes mediaAttribs);
   virtual void rejectPendingOODRefer(unsigned int statusCode);
   virtual void processReferNotify(const resip::SipMessage& notify);
   void handleNonDialogCreatingProvisionalWithEarlyMedia(const resip::SipMessage& msg, const resip::SdpContents& sdp);

   // Called by RemoteParticipantDialogSet when Related Conversations should be destroyed
   virtual void destroyConversations();

   // DTMF Handler
   virtual void onDtmfEvent(int dtmf, int duration, bool up);

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

private:       
   void hold();
   void unhold();
   void provideOffer(bool postOfferAccept);
   bool provideAnswer(const resip::SdpContents& offer, bool postAnswerAccept, bool postAnswerAlert);
   bool answerMediaLine(resip::SdpContents::Session::Medium& mediaSessionCaps, const sdpcontainer::SdpMediaLine& sdpMediaLine, resip::SdpContents& answer, bool potential, ConversationProfile* profile );
   bool buildSdpAnswer(const resip::SdpContents& offer, resip::SdpContents& answer);
   bool formMidDialogSdpOfferOrAnswer(const resip::SdpContents& localSdp, const resip::SdpContents& remoteSdp, resip::SdpContents& newSdp, bool offer);
   void setProposedSdp(const resip::SdpContents& sdp);
   void setLocalSdp(const resip::SdpContents& sdp);
   void setRemoteSdp(const resip::SdpContents& sdp, bool answer=false);
   void setRemoteSdp(const resip::SdpContents& sdp, sdpcontainer::Sdp* remoteSdp);
   void adjustRTPStreams(bool sendingOffer=false, const resip::SipMessage* msg=NULL);
   void checkRemoteIceAttribs(const resip::SdpContents& remoteSdp);
   void onRtpStreamClosed(sdpcontainer::SdpMediaLine::SdpMediaType mediaType, RtpStream::ClosedReason reason, const boost::system::error_code& ec);
   void handleRtpStreamClosed(sdpcontainer::SdpMediaLine::SdpMediaType mediaType, RtpStream::ClosedReason reason, const boost::system::error_code& ec);
   virtual void replaceWithParticipant(RemoteParticipant* replacingParticipant);

   typedef std::map<sdpcontainer::SdpMediaLine::SdpMediaType, boost::signals::connection> MapMediaTypeToConnection;
   MapMediaTypeToConnection m_onRtpStreamClosedConns;

   resip::DialogUsageManager &mDum;
   resip::InviteSessionHandle mInviteSessionHandle; 
   RemoteParticipantDialogSet& mDialogSet;   
   resip::DialogId mDialogId;

   typedef enum
   {
      Connecting=1, 
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
   //bool mLocalHold;
   MediaHoldStateMap mMediaHoldStates;
   void stateTransition(State state);

   resip::AppDialogHandle mReferringAppDialog; 

   resip::SipMessage mPendingOODReferMsg;
   resip::ServerOutOfDialogReqHandle mPendingOODReferNoSubHandle;
   resip::ServerSubscriptionHandle mPendingOODReferSubHandle;

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
   };
   PendingRequest mPendingRequest;
   std::auto_ptr<resip::SdpContents> mPendingOffer;

   sdpcontainer::Sdp* mLocalSdp;
   UInt64 mLocalSdpSessionId;
   UInt64 mLocalSdpVersion;
   sdpcontainer::Sdp* mRemoteSdp;

   ConversationMap mRelatedConversations;

   std::list<resip::SdpContents::Session::Codec> mLicensedCodecs;

   // The last non-0.0.0.0 IP address from the session-level c= line of the remoteSdp.
   // Used so that we can support receiving old-style hold.
   resip::Data mLastRemoteIPAddr;

   // remote ICE u-frag, stored here so that we can determine when the other end signals
   // an ICE restart
   resip::Data mRemoteIceUFrag;
};

}

#endif


/* ====================================================================

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
