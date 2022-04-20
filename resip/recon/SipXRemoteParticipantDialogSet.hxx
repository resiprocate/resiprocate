#if !defined(SipXRemoteParticipantDialogSet_hxx)
#define SipXRemoteParticipantDialogSet_hxx

#include <map>
#include <list>
#include <resip/dum/AppDialogSet.hxx>
#include <resip/dum/AppDialog.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/DialogSetHandler.hxx>
#include <resip/dum/SubscriptionHandler.hxx>

#include "SipXConversationManager.hxx"
#include "ConversationProfile.hxx"
#include "Participant.hxx"
#include "RemoteParticipantDialogSet.hxx"

namespace resip
{
class DialogUsageManager;
class SipMessage;
}

namespace recon
{
class ConversationManager;
class RemoteParticipant;
class SipXRemoteParticipant;
class FlowManagerSipXSocket;

/**
  This class is used by Invite DialogSets.  Non-Invite DialogSets
  are managed by DefaultDialogSet.  This class contains logic
  to handle forking and RTP connections.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class SipXRemoteParticipantDialogSet : public RemoteParticipantDialogSet, private flowmanager::MediaStreamHandler
{
public:
   SipXRemoteParticipantDialogSet(ConversationManager& conversationManager,
                              SipXConversationManager& sipXConversationManager,
                              ConversationManager::ParticipantForkSelectMode forkSelectMode = ConversationManager::ForkSelectAutomatic,
                              std::shared_ptr<ConversationProfile> conversationProfile = nullptr);

   virtual ~SipXRemoteParticipantDialogSet();

   virtual resip::AppDialog* createAppDialog(const resip::SipMessage& msg);
   virtual unsigned int getLocalRTPPort();

   virtual void setActiveRemoteParticipantHandle(ParticipantHandle handle);

   virtual void setPeerExpectsSAVPF(bool value) { mPeerExpectsSAVPF = value; }
   virtual bool peerExpectsSAVPF() { return mPeerExpectsSAVPF; }

   // sipX Media Stuff
   virtual int getMediaConnectionId() { return mMediaConnectionId; }
   virtual int getConnectionPortOnBridge();
   virtual void freeMediaResources();

   void setActiveDestination(const char* address, unsigned short rtpPort, unsigned short rtcpPort);
   void startDtlsClient(const char* address, unsigned short rtpPort, unsigned short rtcpPort);
   void setRemoteSDPFingerprint(const resip::Data& fingerprint);
   bool createSRTPSession(resip::MediaConstants::SrtpCryptoSuite cryptoSuite, const char* remoteKey, unsigned int remoteKeyLen);

   // Media Stream Processing
   virtual void processMediaStreamReadyEvent(std::shared_ptr<MediaStreamReadyEvent::StreamParams> streamParams);

   resip::MediaConstants::SrtpCryptoSuite getSrtpCryptoSuite() { return mSrtpCryptoSuite; }

   const resip::Data& getLocalSrtpSessionKey() { return mLocalSrtpSessionKey; }

protected:
   virtual bool isAsyncMediaSetup();

   virtual void fixUpSdp(resip::SdpContents* sdp);

private:
   SipXConversationManager& mSipXConversationManager;
   unsigned int mLocalRTPPort;
   bool mAllocateLocalRTPPortFailed;
   std::shared_ptr<flowmanager::FlowContext> mFlowContext;
   bool mPeerExpectsSAVPF;

   // Media Stream stuff
   flowmanager::MediaStream::NatTraversalMode mNatTraversalMode;
   flowmanager::MediaStream* mMediaStream;
   reTurn::StunTuple mRtpTuple;
   reTurn::StunTuple mRtcpTuple;
   FlowManagerSipXSocket* mRtpSocket;
   FlowManagerSipXSocket* mRtcpSocket;

   // Secure Media 
   resip::Data mLocalSrtpSessionKey;
   resip::MediaConstants::SrtpCryptoSuite mSrtpCryptoSuite;

   // sipX media stuff
   virtual std::shared_ptr<SipXMediaInterface> getMediaInterface();
   std::shared_ptr<SipXMediaInterface> mMediaInterface;
   int mMediaConnectionId; 
   int mConnectionPortOnBridge;

   virtual void onMediaStreamReady(const StunTuple& remoteRtpTuple, const StunTuple& remoteRtcpTuple);
   virtual void onMediaStreamError(unsigned int errorCode);
};

}

#endif


/* ====================================================================

 Copyright (c) 2021, SIP Spectrum, Inc.
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
