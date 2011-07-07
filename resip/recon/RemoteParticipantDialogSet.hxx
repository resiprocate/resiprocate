#if !defined(RemoteParticipantDialogSet_hxx)
#define RemoteParticipantDialogSet_hxx

#include <map>
#include <list>
#include <resip/dum/AppDialogSet.hxx>
#include <resip/dum/AppDialog.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/DialogSetHandler.hxx>
#include <resip/dum/SubscriptionHandler.hxx>

// FlowManager Includes
#include "MediaStream.hxx"

#include "media/MediaStack.hxx"
#include "ConversationManager.hxx"
#include "ConversationProfile.hxx"
#include "Participant.hxx"
#include "sdp/SdpMediaLine.hxx"
#include "client/IceCandidate.hxx"

namespace sdpcontainer
{
class Sdp;
class SdpMediaLine;
}

namespace resip
{
class DialogUsageManager;
class SipMessage;
}

namespace recon
{
class ConversationManager;
class RemoteParticipant;

/**
  This class is used by Invite DialogSets.  Non-Invite DialogSets
  are managed by DefaultDialogSet.  This class contains logic
  to handle forking and RTP connections.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class RemoteParticipantDialogSet : public resip::AppDialogSet, private flowmanager::MediaStreamHandler
{
public:
   RemoteParticipantDialogSet(ConversationManager& conversationManager,        
                              ConversationManager::ParticipantForkSelectMode forkSelectMode = ConversationManager::ForkSelectAutomatic);

   virtual ~RemoteParticipantDialogSet();

   virtual RemoteParticipant* createUACOriginalRemoteParticipant(ParticipantHandle handle);
   virtual resip::AppDialog* createAppDialog(const resip::SipMessage& msg);

   // Returns the port in use for a specific media type.
   virtual unsigned int getLocalRTPPort(const sdpcontainer::SdpMediaLine::SdpMediaType& mediaType, bool v6,  ConversationProfile* profile);
   virtual void freeLocalRTPPort(const sdpcontainer::SdpMediaLine::SdpMediaType& mediaType);

   virtual void setProposedSdp(ParticipantHandle handle, const resip::SdpContents& sdp);
   virtual sdpcontainer::Sdp* getProposedSdp() { return mProposedSdp; }
   virtual const resip::Data& connectionAddress() const { return mConnectionAddress; }
   virtual resip::Data& connectionAddress() { return mConnectionAddress; }
   virtual void setUACConnected(const resip::DialogId& dialogId, ParticipantHandle partHandle);
   virtual void setUACRedirected(const resip::DialogId& dialogId);
   virtual void destroyStaleDialogs(const resip::DialogId& dialogId);
   virtual bool isUACConnected();
   virtual bool isUACRedirected();
   virtual bool isStaleFork(const resip::DialogId& dialogId);

   virtual void removeDialog(const resip::DialogId& dialogId);
   virtual ConversationManager::ParticipantForkSelectMode getForkSelectMode();
   virtual ParticipantHandle getActiveRemoteParticipantHandle() { return mActiveRemoteParticipantHandle; }
   virtual void setActiveRemoteParticipantHandle(ParticipantHandle handle) { mActiveRemoteParticipantHandle = handle; }

   // DialogSetHandler
   virtual void onTrying(resip::AppDialogSetHandle, const resip::SipMessage& msg);
   virtual void onNonDialogCreatingProvisional(resip::AppDialogSetHandle, const resip::SipMessage& msg);

   // RedirectHandler
   virtual void onRedirectReceived(resip::AppDialogSetHandle, const resip::SipMessage& msg);

   // sipX Media Stuff
   virtual int getMediaConnectionId() { return mMediaConnectionId; }
   virtual int getConnectionPortOnBridge() { return mConnectionPortOnBridge; }

   void setIceRole(bool controlling);
   void setShortTermCredentials(sdpcontainer::SdpMediaLine::SdpMediaType mediaType, const resip::Data& username, const resip::Data& password);
   void setActiveDestination(sdpcontainer::SdpMediaLine::SdpMediaType mediaType, const char* address, unsigned short rtpPort, unsigned short rtcpPort, const sdpcontainer::SdpMediaLine::SdpCandidateList& candidates);
#ifdef USE_DTLS
   void startDtlsClient(const char* address, unsigned short rtpPort, unsigned short rtcpPort);
   void setRemoteSDPFingerprint(const resip::Data& fingerprint);
#endif // USE_DTLS
   void setSrtpEnabled(sdpcontainer::SdpMediaLine::SdpMediaType mediaType, bool enabled);
   bool createSRTPSession(sdpcontainer::SdpMediaLine::SdpMediaType mediaType, flowmanager::MediaStream::SrtpCryptoSuite cryptoSuite, const char* remoteKey, unsigned int remoteKeyLen);

   // Media Stream Processing
   void processMediaStreamReadyEvent(flowmanager::MediaStream* ms, const StunTuple& remoteRtpTuple, const StunTuple& remoteRtcpTuple);
   void processMediaStreamErrorEvent(flowmanager::MediaStream* ms, unsigned int errorCode);

   void sendInvite(resip::SharedPtr<resip::SipMessage> invite);
   void provideOffer(std::auto_ptr<resip::SdpContents> offer, resip::InviteSessionHandle& inviteSessionHandle, bool postOfferAccept);
   void provideAnswer(std::auto_ptr<resip::SdpContents> answer, resip::InviteSessionHandle& inviteSessionHandle, bool postAnswerAccept, bool postAnswerAlert);
   void accept(resip::InviteSessionHandle& inviteSessionHandle);
   ConversationManager::SecureMediaMode getSecureMediaMode() const { return mSecureMediaMode; }
   void setSecureMediaMode(ConversationManager::SecureMediaMode mode) { mSecureMediaMode = mode; }
   void setSecureMediaRequired(bool required) { mSecureMediaRequired = required; }
   flowmanager::MediaStream::SrtpCryptoSuite getSrtpCryptoSuite() const { return mSrtpCryptoSuite; }
   void setSrtpCryptoSuite(flowmanager::MediaStream::SrtpCryptoSuite crypto) { mSrtpCryptoSuite = crypto; }
   void setSrtpCryptoSuite(ConversationManager::SecureMediaCryptoSuite crypto)
   {
      switch(crypto)
      {
      case ConversationManager::SRTP_AES_CM_128_HMAC_SHA1_32:
         mSrtpCryptoSuite = flowmanager::MediaStream::SRTP_AES_CM_128_HMAC_SHA1_32;
         break;
      default:
         mSrtpCryptoSuite = flowmanager::MediaStream::SRTP_AES_CM_128_HMAC_SHA1_80;
         break;
      }
   }
   bool getSecureMediaRequired() { return mSecureMediaRequired; }
   unsigned int getNumDialogs() const { return mNumDialogs; }
   unsigned int getDialogCount() const { return mDialogs.size(); }
   ConversationManager::MediaDirection audioDirection() const { return mAudioDirection; }
   ConversationManager::MediaDirection& audioDirection() { return mAudioDirection; }
   ConversationManager::MediaDirection videoDirection() const { return mVideoDirection; }
   ConversationManager::MediaDirection& videoDirection() { return mVideoDirection; }
   bool audioActive() const { return (mAudioDirection != ConversationManager::MediaDirection_None &&
                                      mAudioDirection != ConversationManager::MediaDirection_Inactive); }
   bool videoActive() const { return (mVideoDirection != ConversationManager::MediaDirection_None &&
                                      mVideoDirection != ConversationManager::MediaDirection_Inactive); }
   bool isMediaActive(sdpcontainer::SdpMediaLine::SdpMediaType mt) const
   {
      if (mt == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO)
         return audioActive();
      if (mt == sdpcontainer::SdpMediaLine::MEDIA_TYPE_VIDEO)
         return videoActive();
      return false;
   }
   bool audioEnabled() const { return (mAudioDirection != ConversationManager::MediaDirection_None); }
   bool videoEnabled() const { return (mVideoDirection != ConversationManager::MediaDirection_None); }
   bool isMediaEnabled(sdpcontainer::SdpMediaLine::SdpMediaType mt) const
   {
      if (mt == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO)
         return audioEnabled();
      if (mt == sdpcontainer::SdpMediaLine::MEDIA_TYPE_VIDEO)
         return videoEnabled();
      return false;
   }

   const resip::Data& getLocalSrtpSessionKey() { return mLocalSrtpSessionKey; }

   boost::shared_ptr<RtpStream> getRtpStream( sdpcontainer::SdpMediaLine::SdpMediaType mediaType ) { return mRtpStreamMap[mediaType]; }
   typedef std::map<sdpcontainer::SdpMediaLine::SdpMediaType, boost::shared_ptr<RtpStream> > RtpStreamMap;
   const RtpStreamMap& getRtpStreams() const { return mRtpStreamMap; }

   // used to reset the ice-ufrag and ice-pwd to new values
   // when a remote ICE restart is detected or when we need to restart ICE
   void resetIceAttribs();
   resip::Data getIceUFrag() { return mIceUFrag; }

protected:
   virtual resip::SharedPtr<resip::UserProfile> selectUASUserProfile(const resip::SipMessage&); 

private:
   void setAddressFromStunResult(resip::SdpContents* sdp);
   void setIceUsernameAndPassword(resip::SdpContents::Session& session);
   void setIceCandidates(resip::SdpContents::Session::Medium& medium, 
      const resip::Data& hostIp, unsigned int hostPort, 
      const resip::Data& rtpSrflxIp, unsigned int rtpSrflxPort,
      const resip::Data& rtcpSrflxIp, unsigned int rtcpSrflxPort);

   sdpcontainer::SdpMediaLine::SdpMediaType getMediaStreamType(flowmanager::MediaStream* ms);
   bool allStreamsReady();

   ConversationManager& mConversationManager;   
   RemoteParticipant* mUACOriginalRemoteParticipant;
   std::list<ConversationHandle> mUACOriginalConversationHandles;
   unsigned int mNumDialogs;
   ConversationManager::ParticipantForkSelectMode mForkSelectMode;
   resip::DialogId mUACConnectedDialogId;
   resip::DialogId mUACRedirectedDialogId;
   ParticipantHandle mActiveRemoteParticipantHandle;
   std::map<resip::DialogId, RemoteParticipant*> mDialogs;

   // Local port map, port is accessed by way of the media type.
   std::map<sdpcontainer::SdpMediaLine::SdpMediaType, unsigned int> mLocalRTPPortMap;

   // RTP Stream map, one RTP stream mer media type
   std::map< sdpcontainer::SdpMediaLine::SdpMediaType, boost::shared_ptr<RtpStream> > mRtpStreamMap;

   // Media Stream stuff
   flowmanager::MediaStream::NatTraversalMode mNatTraversalMode;
   typedef std::map< sdpcontainer::SdpMediaLine::SdpMediaType, flowmanager::MediaStream* > MediaStreamMap;
   MediaStreamMap mMediaStreamMap;
   typedef std::map< sdpcontainer::SdpMediaLine::SdpMediaType, reTurn::StunTuple > StunTupleMap;
   StunTupleMap mRtpTupleMap; // for srflx
   StunTupleMap mRtcpTupleMap; // for srflx
   StunTupleMap mHostRtpTupleMap; // for host

   // SDP Negotiations that may need to be delayed due to FlowManager binding/allocation
   resip::SharedPtr<resip::SipMessage> mPendingInvite;
   void doSendInvite(resip::SharedPtr<resip::SipMessage> invite);
   class PendingOfferAnswer
   {
   public:
      PendingOfferAnswer() {}
      bool mOffer;
      std::auto_ptr<resip::SdpContents> mSdp;
      resip::InviteSessionHandle mInviteSessionHandle;
      bool mPostOfferAnswerAccept;
      bool mPostAnswerAlert;
   };
   PendingOfferAnswer mPendingOfferAnswer;
   void doProvideOfferAnswer(bool offer, std::auto_ptr<resip::SdpContents> sdp, resip::InviteSessionHandle& inviteSessionHandle, bool postOfferAnswerAccept, bool postAnswerAlert);
   sdpcontainer::Sdp* mProposedSdp;  // stored here vs RemoteParticipant, since each forked leg needs access to the original offer
   // used only if ConversationProfile::useRfc2543Hold() is set to true
   resip::Data mConnectionAddress;

   // only used for ICE
   resip::Data mIceUFrag;
   resip::Data mIcePwd;

   // Secure Media 
   resip::Data mLocalSrtpSessionKey;
   ConversationManager::SecureMediaMode mSecureMediaMode;
   bool mSecureMediaRequired;
   flowmanager::MediaStream::SrtpCryptoSuite mSrtpCryptoSuite;

   // Other participant-specific attributes
   ConversationManager::MediaDirection mAudioDirection;
   ConversationManager::MediaDirection mVideoDirection;
   //bool mAudioEnabled;
   //bool mVideoEnabled;

   // sipX media stuff
   int mMediaConnectionId; 
   int mConnectionPortOnBridge;

   virtual void onMediaStreamReady(flowmanager::MediaStream* ms, const StunTuple& remoteRtpTuple, const StunTuple& remoteRtcpTuple);
   virtual void onMediaStreamError(flowmanager::MediaStream* ms, unsigned int errorCode);
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
