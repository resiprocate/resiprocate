#if !defined(SipXConversationManager_hxx)
#define SipXConversationManager_hxx

#include <boost/function.hpp>

#include "BridgeMixer.hxx"

#include <resip/stack/Uri.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/DialogSetHandler.hxx>
#include <resip/dum/SubscriptionHandler.hxx>
#include <resip/dum/OutOfDialogHandler.hxx>
#include <resip/dum/RedirectHandler.hxx>
#include <rutil/Mutex.hxx>

#include "RTPPortManager.hxx"
#include "MediaResourceCache.hxx"
#include "MediaEvent.hxx"
#include "HandleTypes.hxx"
#include "ConversationManager.hxx"
#include "SipXMediaInterface.hxx"

#include "reflow/FlowManager.hxx"

#include <memory>

class CpMediaInterfaceFactory;

namespace resip
{
class DialogUsageManager;
}

namespace recon
{
class Conversation;
class Participant;
class UserAgent;
class ConversationProfile;
class LocalParticipant;
class MediaResourceParticipant;
class RemoteParticipant;
class RemoteParticipantDialogSet;


/**
  This class is one of two main classes of concern to an application
  using the UserAgent library.  This class should be subclassed by 
  the application and the ConversationManager handlers should be 
  implemented by it.

  This class is responsible for handling tasks that are directly
  related to managing Conversations.  All non-conversation
  handling is done via the UserAgent class.

  This class handles tasks such as:
  -Creation and destruction of Conversations
  -Participant management and creation
  -Placing and receiving calls
  -Playing audio and/or tones into a conversation
  -Managing local audio properties

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class SipXConversationManager : public ConversationManager
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

          sipXConversationMediaInterfaceMode - by default uses 1 sipXtapi media
          interface per conversation.  If you use createSharedMediaInterfaceConversation
          instead of createConversation then you can specify that 2 (multiple)
          conversations can share the same media interface. This allows participants
          to be moved between any conversations that share the same media interface.
          Using this mode, participants can only exist in multiple conversations
          at the same time if those conversations share the same media interface.
          This means the limit of 7 participants is no longer global, it now applies
          to each media interface.  A separate media participant for each media
          interface can also exist.  This architecture/mode is appropriate for server
          applications, such as multi-party conference servers (up to 7 participants
          per conference), music on hold servers and call park servers.
          API restrictions in this mode:
            -joinConversation - restricted to functioning only if both source and
                                destination conversations share the same media inteface
            -addParticipant - can only add a participant to multiple conversations if
                              the conversations share the same media interface
            -moveParticipant - for non-local participants, restricted to functioning
                               only if both source and destination conversations share
                               the same media inteface
            -alertParticipant - if you are using the EarlyFlag then the
                                RemoteParticipant must be added to a conversation
                                before calling this
            -answerParticipant - RemoteParticipant must be added to a conversation
                                before calling this
   */
   typedef enum
   {
      sipXGlobalMediaInterfaceMode,
      sipXConversationMediaInterfaceMode
   } MediaInterfaceMode;

   SipXConversationManager(bool localAudioEnabled = true, MediaInterfaceMode mediaInterfaceMode = sipXGlobalMediaInterfaceMode);
   SipXConversationManager(bool localAudioEnabled, MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate);
   virtual ~SipXConversationManager();

   ///////////////////////////////////////////////////////////////////////
   // Conversation methods  //////////////////////////////////////////////
   ///////////////////////////////////////////////////////////////////////

   /**
     Only applicable if sipXConversationMediaInterfaceMode is used.

     Create a new empty Conversation to which participants
     can be added.  This new conversation will share the media interface
     from the passed in conversation handle.  This allows participants to be
     moved between these two conversations, or any conversations that share the
     same media interface.  This method will fail (return 0) if this
     ConversationManager is in sipXGlobalMediaInterfaceMode.

     @param autoHoldMode - enum specifying one of the following:
      AutoHoldDisabled - Never auto hold, only hold if holdParticipant API
                         is used
      AutoHoldEnabled - Default.  Automatically put a RemoteParticipant
                        on-hold if there are no other participants in the
                        conversation with them.
      AutoHoldBroadcastOnly - Use this if you are broadcasting media to all
                              participants and don't need to receive any
                              inbound media.  All participants in the
                              conversation will be SIP held and will receive
                              media from an added MediaParticipant. Remote
                              offers with inactive will be responded with
                              sendonly as well.  This option is useful for
                              implementing music on hold servers.

     @return A handle to the newly created conversation.
   */
   virtual ConversationHandle createSharedMediaInterfaceConversation(ConversationHandle sharedMediaInterfaceConvHandle, AutoHoldMode autoHoldMode = AutoHoldEnabled);

   ///////////////////////////////////////////////////////////////////////
   // Participant methods  ///////////////////////////////////////////////
   ///////////////////////////////////////////////////////////////////////

   /**
     Creates a new local participant in the specified conversation.
     A local participant is a representation of the local source (speaker)
     and sink (microphone).  The local participant is generally only
     created once and is added to conversations in which the local speaker
     and/or microphone should be involved.

     @return A handle to the newly created local participant
   */
   virtual ParticipantHandle createLocalParticipant();

   /**
     Logs a multiline representation of the current state
     of the mixing matrix.

     @param convHandle - if sipXGlobalMediaInterfaceMode is used then 0
                         is the only valid value.  Otherwise you must
                         specify a specific conversation to view.
   */
   virtual void outputBridgeMatrix(ConversationHandle convHandle = 0);

   /**
     Builds a session capabilties SDPContents based on the passed in ipaddress
     and codec ordering.
     Note:  Codec ordering is an array of sipX internal codecId's.  Id's for
            codecs not loaded are ignored.
   */
   virtual void buildSessionCapabilities(const resip::Data& ipaddress, unsigned int numCodecIds,
      unsigned int codecIds[], resip::SdpContents& sessionCaps);

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

   virtual Conversation *createConversationInstance(ConversationHandle handle,
      RelatedConversationSet* relatedConversationSet,  // Pass NULL to create new RelatedConversationSet
      ConversationHandle sharedMediaInterfaceConvHandle,
      ConversationManager::AutoHoldMode autoHoldMode) override;
   virtual LocalParticipant *createLocalParticipantInstance(ParticipantHandle partHandle);
   virtual MediaResourceParticipant *createMediaResourceParticipantInstance(ParticipantHandle partHandle, resip::Uri mediaUrl);
   virtual RemoteParticipant *createRemoteParticipantInstance(resip::DialogUsageManager& dum, RemoteParticipantDialogSet& rpds);
   virtual RemoteParticipant *createRemoteParticipantInstance(ParticipantHandle partHandle, resip::DialogUsageManager& dum, RemoteParticipantDialogSet& rpds);
   virtual RemoteParticipantDialogSet *createRemoteParticipantDialogSetInstance(
         ConversationManager::ParticipantForkSelectMode forkSelectMode = ConversationManager::ForkSelectAutomatic,
         std::shared_ptr<ConversationProfile> conversationProfile = nullptr);

   MediaInterfaceMode getMediaInterfaceMode() const { return mMediaInterfaceMode; }

   virtual bool supportsMultipleConversations();
   virtual bool supportsJoin(ConversationHandle sourceConvHandle, ConversationHandle destConvHandle);

protected:
   virtual void setUserAgent(UserAgent *userAgent);

private:
   void init(int defaultSampleRate = 0, int maxSampleRate = 0);

   friend class DefaultDialogSet;
   friend class Subscription;

   friend class UserAgentShutdownCmd;

   // Note:  In general the following fns are not thread safe and must be called from dum process 
   //        loop only
   friend class Conversation;
   friend class OutputBridgeMixWeightsCmd;

   friend class Participant;
   friend class SipXParticipant;

   friend class RemoteParticipant;
   friend class SipXRemoteParticipant;
   friend class UserAgent;

   friend class DtmfEvent;
   friend class MediaEvent;

   friend class RemoteParticipantDialogSet;
   friend class SipXRemoteParticipantDialogSet;
   friend class MediaResourceParticipant;
   friend class LocalParticipant;
   friend class BridgeMixer;
   friend class SipXMediaInterface;

   flowmanager::FlowManager& getFlowManager() { return mFlowManager; }

   // exists here (as opposed to RemoteParticipant) - since it is required for OPTIONS responses
   virtual void buildSdpOffer(ConversationProfile* profile, resip::SdpContents& offer);

   friend class OutputBridgeMixWeightsCmd;
   void outputBridgeMatrixImpl(ConversationHandle convHandle = 0);

   friend class MediaResourceParticipantDeleterCmd;
   friend class CreateConversationCmd;
   friend class DestroyConversationCmd;
   friend class JoinConversationCmd;
   friend class CreateRemoteParticipantCmd;
   friend class CreateMediaResourceParticipantCmd;
   friend class CreateLocalParticipantCmd;
   friend class DestroyParticipantCmd;
   friend class AddParticipantCmd;
   friend class RemoveParticipantCmd;
   friend class MoveParticipantCmd;
   friend class ModifyParticipantContributionCmd;
   friend class AlertParticipantCmd;
   friend class AnswerParticipantCmd;
   friend class RejectParticipantCmd;
   friend class RedirectParticipantCmd;
   friend class RedirectToParticipantCmd;
   friend class HoldParticipantCmd;

   bool mLocalAudioEnabled;
   MediaInterfaceMode mMediaInterfaceMode;

   std::shared_ptr<RTPPortManager> mRTPPortManager;

   // FlowManager Instance
   flowmanager::FlowManager mFlowManager;

   // sipX Media related members
   void createMediaInterfaceAndMixer(bool giveFocus,
                                     std::shared_ptr<SipXMediaInterface>& mediaInterface,
                                     std::shared_ptr<BridgeMixer>& bridgeMixer);
   std::shared_ptr<SipXMediaInterface> getMediaInterface() const { resip_assert(mMediaInterface.get()); return mMediaInterface; }
   CpMediaInterfaceFactory* getMediaInterfaceFactory() { return mMediaFactory; }
   CpMediaInterfaceFactory* mMediaFactory;
   std::shared_ptr<SipXMediaInterface> mMediaInterface;
   int mSipXTOSValue;
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
