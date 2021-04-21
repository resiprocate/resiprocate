#if !defined(SipXConversationManager_hxx)
#define SipXConversationManager_hxx

#include "ConversationManager.hxx"
#include "MediaInterface.hxx"
#include "RTPPortManager.hxx"
#include "SipXBridgeMixer.hxx"
#include "UserAgent.hxx"


class CpMediaInterfaceFactory;

namespace recon
{

class SipXConversationManager  : public recon::ConversationManager
{
public:
   /**
     Note:  sipXtapi Media Interfaces have a finite number of supported endpoints
          that are allowed in each bridge mixer - by default this value is 10
          (2 are used for local mic / speaker, 1 for a MediaParticipant, leaving
           7 remaining for RemoteParticipants).  This limit is controlled by the
          preprocessor define DEFAULT_BRIDGE_MAX_IN_OUTPUTS (see
          http://www.resiprocate.org/Limitations_with_sipXtapi_media_Integration
          for more details.

          sipXGlobalMediaInterfaceMode - uses 1 global sipXtapi media interface  and
          allows for participants to exist in multiple conversations at the same
          time and have the bridge mixer properly control their mixing.  In this
          mode, there can only be a single MediaParticipant for all conversations.
          This architecture/mode is appropriate for single user agent devices (ie.
          sip phones).

          sipXConversationMediaInterfaceMode - uses 1 sipXtapi media interface per
          conversation.  Using this mode, participants cannot exist in multiple
          conversations at the same time, however the limit of 7 participants is
          no longer global, it now applies to each conversation.  A separate
          media participant for each conversation can also exist.
          This architecture/mode is appropriate for server applications, such as
          multi-party conference servers (up to 7 participants per conference),
          music on hold servers and call park servers.  Other API's that won't
          function in this mode are:
            -joinConversation
            -addParticipant - only applies to the LocalParticipant
            -moveParticipant - only applies to the LocalParticipant
          Note:  For inbound (UAS) Remote Participant sessions, ensure that you
                 add the Remote Participant to a conversation before you return
                 call accept() for alert() with early media.
   */
   typedef enum
   {
      sipXGlobalMediaInterfaceMode,
      sipXConversationMediaInterfaceMode
   } MediaInterfaceMode;

   SipXConversationManager(bool localAudioEnabled=true, MediaInterfaceMode mediaInterfaceMode = sipXGlobalMediaInterfaceMode);
   SipXConversationManager(bool localAudioEnabled, MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate);
   virtual ~SipXConversationManager();

   /**
     Builds a session capabilties SDPContents based on the passed in ipaddress
     and codec ordering.
     Note:  Codec ordering is an array of sipX internal codecId's.  Id's for
            codecs not loaded are ignored.
   */
   virtual void buildSessionCapabilities(const resip::Data& ipaddress, unsigned int numCodecIds,
                                         unsigned int codecIds[], resip::SdpContents& sessionCaps);

   virtual void outputBridgeMatrix();

   virtual ParticipantHandle createLocalParticipant();

   ///////////////////////////////////////////////////////////////////////
   // Media Related Methods - this may not be the right spot for these - move to LocalParticipant?
   ///////////////////////////////////////////////////////////////////////

   virtual void setSpeakerVolume(int volume);
   virtual void setMicrophoneGain(int gain);
   virtual void muteMicrophone(bool mute);
   virtual void enableEchoCancel(bool enable);
   virtual void enableAutoGainControl(bool enable);
   virtual void enableNoiseReduction(bool enable);
   virtual void setSipXTOSValue(int tos) { mSipXTOSValue = tos; }
   virtual std::shared_ptr<RTPPortManager> getRTPPortManager() { return mRTPPortManager; }

   flowmanager::FlowManager& getFlowManager() { return mFlowManager; }

   // exists here (as opposed to RemoteParticipant) - since it is required for OPTIONS responses
   virtual void buildSdpOffer(ConversationProfile* profile, resip::SdpContents& offer);

   virtual bool supportsJoin();

   virtual LocalParticipant *createLocalParticipantImpl(ParticipantHandle partHandle);

   virtual RemoteParticipantDialogSet *createRemoteParticipantDialogSet(
         ConversationManager::ParticipantForkSelectMode forkSelectMode = ConversationManager::ForkSelectAutomatic,
         std::shared_ptr<ConversationProfile> conversationProfile = nullptr);

private:
   void init(int defaultSampleRate = 0, int maxSampleRate = 0);

   virtual void setUserAgent(UserAgent *userAgent);

   bool mLocalAudioEnabled;
   MediaInterfaceMode mMediaInterfaceMode;
   MediaInterfaceMode getMediaInterfaceMode() const { return mMediaInterfaceMode; }

   std::shared_ptr<RTPPortManager> mRTPPortManager;

   // FlowManager Instance
   flowmanager::FlowManager mFlowManager;

   // sipX Media related members
   void createMediaInterfaceAndMixer(bool giveFocus, ConversationHandle ownerConversationHandle,
                                     std::shared_ptr<MediaInterface>& mediaInterface, SipXBridgeMixer** bridgeMixer);
   std::shared_ptr<MediaInterface> getMediaInterface() const { resip_assert(mMediaInterface.get()); return mMediaInterface; }
   CpMediaInterfaceFactory* getMediaInterfaceFactory() { return mMediaFactory; }
   BridgeMixer* getBridgeMixer() { return mBridgeMixer; }
   CpMediaInterfaceFactory* mMediaFactory;
   std::shared_ptr<MediaInterface> mMediaInterface;
   SipXBridgeMixer* mBridgeMixer;
   int mSipXTOSValue;

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
