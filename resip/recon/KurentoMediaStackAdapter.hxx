#if !defined(KurentoMediaStackAdapter_hxx)
#define KurentoMediaStackAdapter_hxx

#include <boost/function.hpp>

#include "BridgeMixer.hxx"

#include <resip/stack/Uri.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/DialogSetHandler.hxx>
#include <resip/dum/SubscriptionHandler.hxx>
#include <resip/dum/OutOfDialogHandler.hxx>
#include <resip/dum/RedirectHandler.hxx>
#include <rutil/Mutex.hxx>

#include <media/kurento/KurentoManager.hxx>
#include <media/kurento/Object.hxx>

#include "MediaStackAdapter.hxx"
#include "HandleTypes.hxx"
#include "ConversationManager.hxx"

#include <memory>

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
class KurentoRemoteParticipant;
class LocalParticipant;
class MediaResourceParticipant;
class RemoteParticipant;
class RemoteParticipantDialogSet;
class KurentoRemoteParticipantDialogSet;

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
*/

class KurentoMediaStackAdapter : public MediaStackAdapter, public kurento::KurentoConnectionObserver
{
public:

   /**
     Note:  Kurentotapi Media Interfaces have a finite number of supported endpoints
          that are allowed in each bridge mixer - by default this value is 10
          (1 used for local mic / speaker, 1 for a WAV Player/Recorder and 1 for a 
          Tone Player, leaving 7 remaining for RemoteParticipants - 8 if local audio  
          is disabled).  Also, if enableExtraPlayAndRecordResources is passed in as true
          then an extra slot if taken for an additional player and recorder.

          The limit of 10 is controlled by the preprocessor define 
          DEFAULT_BRIDGE_MAX_IN_OUTPUTS (see
          http://www.resiprocate.org/Limitations_with_Kurentotapi_media_Integration
          for more details.

          KurentoGlobalMediaInterfaceMode - uses 1 global Kurentotapi media interface and
          allows for participants to exist in multiple conversations at the same
          time and have the bridge mixer properly control their mixing.  In this
          mode, there can only be a single MediaParticipant at a time performing
          each operation (ie.  1 playing a WAV, 1 recording and one playing a tone
          is allowed, but only 1 of each type) for all conversations.  If 
          enableExtraPlayAndRecordResources is enabled then you are allowed to have 2
          players and 2 recorders.  This architecture/mode is appropriate for single 
          user agent devices (ie. sip phones).

          KurentoConversationMediaInterfaceMode - by default uses 1 Kurentotapi media
          interface per conversation.  If you use createSharedMediaInterfaceConversation
          instead of createConversation then you can specify that 2 (multiple)
          conversations can share the same media interface. This allows participants
          to be moved between any conversations that share the same media interface.
          Using this mode, participants can only exist in multiple conversations
          at the same time if those conversations share the same media interface.
          This means the limit of remote participants is no longer global, it now 
          applies to each media interface.  The limit on media participants is per 
          media interface.  This architecture/mode is appropriate for server applications, 
          such as multi-party conference servers (up to 8 participants per conference), 
          music on hold servers and call park servers. 
          API restrictions in this mode:
            -joinConversation  -restricted to functioning only if both source and
                                destination conversations share the same media inteface
            -addParticipant    -can only add a participant to multiple conversations if
                                the conversations share the same media interface
            -moveParticipant   -can only move a participant to another conversation if
                                both source and destination conversations share the same 
                                media inteface
            -alertParticipant  -if you are using the EarlyFlag then the
                                RemoteParticipant must be added to a conversation
                                before calling this
            -answerParticipant -RemoteParticipant must be added to a conversation
                                before calling this
          Note: Only LocalParticipants can only exist outside of a conversation.  If they
                exist in multiple conversations then those conversations must have the
                same media interface.
   */

   KurentoMediaStackAdapter(ConversationManager& conversationManager, const resip::Data& kurentoUri);
   KurentoMediaStackAdapter(ConversationManager& conversationManager, const resip::Data& kurentoUri, int defaultSampleRate, int maxSampleRate);
   virtual ~KurentoMediaStackAdapter();

   virtual void onConnected() override;

   virtual void conversationManagerReady(ConversationManager* conversationManager) override;

   virtual void shutdown() override;

   ///////////////////////////////////////////////////////////////////////
   // Conversation methods  //////////////////////////////////////////////
   ///////////////////////////////////////////////////////////////////////

   /**
     Only applicable if KurentoConversationMediaInterfaceMode is used.

     Create a new empty Conversation to which participants
     can be added.  This new conversation will share the media interface
     from the passed in conversation handle.  This allows participants to be
     moved between these two conversations, or any conversations that share the
     same media interface.  This method will fail (return 0) if this
     ConversationManager is in KurentoGlobalMediaInterfaceMode.

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
   virtual ConversationHandle createSharedMediaInterfaceConversation(ConversationHandle sharedMediaInterfaceConvHandle, ConversationManager::AutoHoldMode autoHoldMode = ConversationManager::AutoHoldEnabled); // FIXME Kurento - only for sipXtapi?

   ///////////////////////////////////////////////////////////////////////
   // Participant methods  ///////////////////////////////////////////////
   ///////////////////////////////////////////////////////////////////////

   /**
     Builds a session capabilties SDPContents based on the passed in ipaddress
     and codec ordering.
     Note:  Codec ordering is an array of sipX internal codecId's.  Id's for
            codecs not loaded are ignored.
   */
   virtual void buildSessionCapabilities(const resip::Data& ipaddress,
      const std::vector<unsigned int>& codecIds, resip::SdpContents& sessionCaps) override;


   ///////////////////////////////////////////////////////////////////////
   // Media Related Methods - this may not be the right spot for these - move to LocalParticipant?
   ///////////////////////////////////////////////////////////////////////

   virtual void setKurentoTOSValue(int tos) { mKurentoTOSValue = tos; }

   virtual Conversation *createConversationInstance(ConversationHandle handle,
      RelatedConversationSet* relatedConversationSet,  // Pass NULL to create new RelatedConversationSet
      ConversationHandle sharedMediaInterfaceConvHandle,
      ConversationManager::AutoHoldMode autoHoldMode) override;
   virtual LocalParticipant *createLocalParticipantInstance(ParticipantHandle partHandle) override;
   virtual MediaResourceParticipant *createMediaResourceParticipantInstance(ParticipantHandle partHandle, const resip::Uri& mediaUrl, const std::shared_ptr<resip::Data>& audioBuffer, void* recordingCircularBuffer) override;
   virtual RemoteParticipant *createRemoteParticipantInstance(resip::DialogUsageManager& dum, RemoteParticipantDialogSet& rpds) override;
   virtual RemoteParticipant *createRemoteParticipantInstance(ParticipantHandle partHandle, resip::DialogUsageManager& dum, RemoteParticipantDialogSet& rpds) override;
   virtual RemoteParticipantDialogSet *createRemoteParticipantDialogSetInstance(
         ConversationManager::ParticipantForkSelectMode forkSelectMode = ConversationManager::ForkSelectAutomatic,
         std::shared_ptr<ConversationProfile> conversationProfile = nullptr) override;

   virtual bool supportsMultipleMediaInterfaces() override;
   virtual bool canConversationsShareParticipants(Conversation* conversation1, Conversation* conversation2) override;
   virtual bool supportsLocalAudio() override { return false; }

protected:
   virtual void setUserAgent(UserAgent *userAgent) override;
   virtual void configureRemoteParticipantInstance(KurentoRemoteParticipant* krp);

private:
   void init(int defaultSampleRate = 0, int maxSampleRate = 0);

   friend class DefaultDialogSet;
   friend class Subscription;

   friend class UserAgentShutdownCmd;

   // Note:  In general the following fns are not thread safe and must be called from dum process 
   //        loop only
   friend class Conversation;
   friend class KurentoConversation;
   friend class OutputBridgeMixWeightsCmd;

   friend class Participant;
   friend class KurentoParticipant;

   friend class RemoteParticipant;
   friend class KurentoRemoteParticipant;
   friend class UserAgent;

   friend class DtmfEvent;
   friend class MediaEvent;

   friend class RemoteParticipantDialogSet;
   friend class KurentoRemoteParticipantDialogSet;
   friend class MediaResourceParticipant;
   friend class LocalParticipant;
   friend class BridgeMixer;
   friend class KurentoMediaInterface;

   virtual void process() override;

   virtual void initializeDtlsFactory(const resip::Data& defaultAoR) override;

   friend class OutputBridgeMixWeightsCmd;
   void outputBridgeMatrixImpl(ConversationHandle convHandle = 0) override;

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

   // Kurento Media related members
   resip::Data mKurentoUri;
   kurento::KurentoManager mKurentoManager;
   kurento::KurentoConnection::ptr mKurentoConnection; // FIXME - make sure it is valid whenever it is used
   std::shared_ptr<kurento::MediaPipeline> mPipeline;
   int mKurentoTOSValue;  // FIXME Kurento - need to pass to Kurento, maybe move to superclass too
};

}

#endif


/* ====================================================================

 Copyright (c) 2021-2022, SIP Spectrum, Inc. www.sipspectrum.com
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
