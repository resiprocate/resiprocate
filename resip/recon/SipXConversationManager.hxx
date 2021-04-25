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
     from the passed in converation handle.  This allows participants to be
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

   // Override this to handle the callback
   virtual void onApplicationTimer(unsigned int timerId, unsigned int timerData) { }


   ///////////////////////////////////////////////////////////////////////
   // Conversation Manager Handlers //////////////////////////////////////
   ///////////////////////////////////////////////////////////////////////

   // !slg! Note: We should eventually be passing back a generic ParticipantInfo object
   //             and not the entire SipMessage for these callbacks

   /**
     Notifies an application about a new remote participant that is attempting
     to contact it.

     @param partHandle Handle of the new incoming participant
     @param msg Includes information about the caller such as name and address
     @param autoAnswer Set to true if auto answer has been requested
   */
   virtual void onIncomingParticipant(ParticipantHandle partHandle, const resip::SipMessage& msg, bool autoAnswer, ConversationProfile& conversationProfile) = 0;

   /**
     Notifies an application about a new remote participant that is trying 
     to be contacted.  This event is required to notify the application if a 
     call request has been initiated by a signaling mechanism other than
     the application, such as an out-of-dialog REFER request.

     @param partHandle Handle of the new remote participant
     @param msg Includes information about the destination requested
                to be attempted
   */
   virtual void onRequestOutgoingParticipant(ParticipantHandle partHandle, const resip::SipMessage& msg, ConversationProfile& conversationProfile) = 0;

   /**
     Notifies an application about a disconnect by a remote participant.  
     For SIP this could be a BYE or a CANCEL request.

     @param partHandle Handle of the participant that terminated
     @param statusCode The status Code for the termination.
   */
   virtual void onParticipantTerminated(ParticipantHandle partHandle, unsigned int statusCode) = 0;

   /**
     Notifies an application when a conversation has been destroyed.  
     This is useful for tracking conversations that get created when forking 
     occurs, and are destroyed when forked call is answered or ended.

     @param convHandle Handle of the destroyed conversation
   */
   virtual void onConversationDestroyed(ConversationHandle convHandle) = 0;

   /** 
     Notifies an application when a Participant has been destroyed.  This is 
     useful for tracking when audio playback via MediaResourceParticipants has 
     stopped.

     @param partHandle Handle of the destroyed participant
   */
   virtual void onParticipantDestroyed(ParticipantHandle partHandle) = 0;

   /**
     Notifies an applications that a outbound remote participant request has 
     forked.  A new Related Conversation and Participant are created.  
     Both new handles and original handles are conveyed to the application 
     so it can track related conversations.

     @param relatedConvHandle Handle of newly created related conversation
     @param relatedPartHandle Handle of the newly created related participant
     @param origConvHandle    Handle of the conversation that contained the
                              participant that forked
     @param origParticipant   Handle of the participant that forked
   */
   virtual void onRelatedConversation(ConversationHandle relatedConvHandle, ParticipantHandle relatedPartHandle, 
                                      ConversationHandle origConvHandle, ParticipantHandle origPartHandle) = 0;

   /**
     Notifies an application that a remote participant call attempt is 
     alerting the remote party.  

     @param partHandle Handle of the participant that is alerting
     @param msg SIP message that caused the alerting
   */
   virtual void onParticipantAlerting(ParticipantHandle partHandle, const resip::SipMessage& msg) = 0;

   /**
     Notifies an application that a remote participant call attempt is 
     now connected.

     @param partHandle Handle of the participant that is connected
     @param msg SIP message that caused the connection
   */
   virtual void onParticipantConnected(ParticipantHandle partHandle, const resip::SipMessage& msg) = 0;

   /**
     Notifies an application that a redirect request has succeeded.  
     Indicates blind transfer or attended transfer status. 

     @param partHandle Handle of the participant that was redirected
   */
   virtual void onParticipantRedirectSuccess(ParticipantHandle partHandle) = 0;

   /**
     Notifies an application that a redirect request has failed.  
     Indicates blind transfer or attended transfer status. 

     @param partHandle Handle of the participant that was redirected
   */
   virtual void onParticipantRedirectFailure(ParticipantHandle partHandle, unsigned int statusCode) = 0;

   /**
     Notifies an application when an RFC2833 DTMF event is received from a 
     particular remote participant.

     @param partHandle Handle of the participant that received the digit
     @param dtmf Integer representation of the DTMF tone received (from RFC2833 event codes)
     @param duration Duration (in milliseconds) of the DTMF tone received
     @param up Set to true if the DTMF key is up (otherwise down)
   */
   virtual void onDtmfEvent(ParticipantHandle partHandle, int dtmf, int duration, bool up) = 0;

   virtual void onParticipantRequestedHold(ParticipantHandle partHandle, bool held) = 0;

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

   // ServerSubscriptionHandler ///////////////////////////////////////////////////
   virtual void onNewSubscription(resip::ServerSubscriptionHandle, const resip::SipMessage& sub);
   virtual void onNewSubscriptionFromRefer(resip::ServerSubscriptionHandle, const resip::SipMessage& sub);
   virtual void onRefresh(resip::ServerSubscriptionHandle, const resip::SipMessage& sub);
   virtual void onTerminated(resip::ServerSubscriptionHandle);
   virtual void onReadyToSend(resip::ServerSubscriptionHandle, resip::SipMessage&);
   virtual void onNotifyRejected(resip::ServerSubscriptionHandle, const resip::SipMessage& msg);      
   virtual void onError(resip::ServerSubscriptionHandle, const resip::SipMessage& msg);      
   virtual void onExpiredByClient(resip::ServerSubscriptionHandle, const resip::SipMessage& sub, resip::SipMessage& notify);
   virtual void onExpired(resip::ServerSubscriptionHandle, resip::SipMessage& notify);
   virtual bool hasDefaultExpires() const;
   virtual UInt32 getDefaultExpires() const;

   // OutOfDialogHandler //////////////////////////////////////////////////////////
   virtual void onSuccess(resip::ClientOutOfDialogReqHandle, const resip::SipMessage& response);
   virtual void onFailure(resip::ClientOutOfDialogReqHandle, const resip::SipMessage& response);
   virtual void onReceivedRequest(resip::ServerOutOfDialogReqHandle, const resip::SipMessage& request);

   // RedirectHandler /////////////////////////////////////////////////////////////
   virtual void onRedirectReceived(resip::AppDialogSetHandle, const resip::SipMessage& response);
   virtual bool onTryingNextTarget(resip::AppDialogSetHandle, const resip::SipMessage& request);

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
