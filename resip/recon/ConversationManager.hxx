#if !defined(ConversationManager_hxx)
#define ConversationManager_hxx

#include <boost/function.hpp>

#include "BridgeMixer.hxx"

#include <resip/stack/Uri.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/DialogSetHandler.hxx>
#include <resip/dum/SubscriptionHandler.hxx>
#include <resip/dum/OutOfDialogHandler.hxx>
#include <resip/dum/RedirectHandler.hxx>
#include <rutil/Mutex.hxx>
#include <rutil/SharedPtr.hxx>

#include "MediaResourceCache.hxx"
#include "MediaEvent.hxx"
#include "HandleTypes.hxx"
#include "MediaInterface.hxx"

#include "reflow/FlowManager.hxx"

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
class RemoteParticipant;


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

class ConversationManager  : public resip::InviteSessionHandler,
                             public resip::DialogSetHandler,
                             public resip::OutOfDialogHandler,
                             public resip::ClientSubscriptionHandler,
                             public resip::ServerSubscriptionHandler,
                             public resip::RedirectHandler
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

   ConversationManager(bool localAudioEnabled=true, MediaInterfaceMode mediaInterfaceMode = sipXGlobalMediaInterfaceMode);
   ConversationManager(bool localAudioEnabled, MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate);
   virtual ~ConversationManager();

   typedef enum 
   {
      ForkSelectAutomatic, // create a conversation for each early fork. accept the first fork from which a 200 is received.  automatically kill other forks 
      ForkSelectManual     // create a conversation for each early fork. let the application dispose of extra forks. ex: app may form conference. 
   } ParticipantForkSelectMode;

   ///////////////////////////////////////////////////////////////////////
   // Conversation methods  //////////////////////////////////////////////
   ///////////////////////////////////////////////////////////////////////

   /**
     Create a new empty Conversation to which participants 
     can be added.

     @param broadcastOnly - if set to true, then all participants in
                            the conversation will be SIP held and will
                            receive media from an added MediaParticipant.
                            This option is useful for implementing music
                            on hold servers.

     @return A handle to the newly created conversation.
   */   
   virtual ConversationHandle createConversation(bool broadcastOnly=false);

   /**
     Destroys an existing Conversation, and ends all 
     participants that solely belong to this conversation.

     @param handle Handle of conversation to destroy
   */   
   virtual void destroyConversation(ConversationHandle convHandle);     

   /**
     Joins all participants from source conversation into 
     destination conversation and destroys source conversation.

     @param sourceConvHandle Handle of source conversation
     @param destConvHandle   Handle of destination conversation

     @note This API cannot be used when sipXConversationMediaInterfaceMode 
           is enabled.
   */   
   virtual void joinConversation(ConversationHandle sourceConvHandle, ConversationHandle destConvHandle);

   ///////////////////////////////////////////////////////////////////////
   // Participant methods  ///////////////////////////////////////////////
   ///////////////////////////////////////////////////////////////////////

   /**
     Creates a new remote participant in the specified conversation 
     that is attempted to be reached at the specified address.  
     For SIP the address is a URI.  ForkSelectMode can be set to either 
     automatic or manual.  When ForkSelectMode is set to auto the 
     conversation manager will automatically dispose of any related 
     conversations that were created, due to forking.

     @param convHandle Handle of the conversation to create the 
                       RemoteParticipant in
     @param destination Uri of the remote participant to reach
     @param forkSelectMode Determines behavior if forking occurs

     @return A handle to the newly created remote participant
   */   
   virtual ParticipantHandle createRemoteParticipant(ConversationHandle convHandle, const resip::NameAddr& destination, ParticipantForkSelectMode forkSelectMode = ForkSelectAutomatic);

   virtual ParticipantHandle createRemoteParticipant(ConversationHandle convHandle, const resip::NameAddr& destination, ParticipantForkSelectMode forkSelectMode, resip::SharedPtr<resip::UserProfile>& callerProfile, const std::multimap<resip::Data,resip::Data>& extraHeaders);

   /**
     Creates a new media resource participant in the specified conversation.  
     Media is played from a source specified by the url and may be a local 
     audio file, audio file fetched via HTTP or tones.  The URL can contain 
     parameters that specify properties of the media playback, such as 
     number of repeats.  

     Media Urls are of the following format: 
     tone:<tone> - Tones can be any DTMF digit 0-9,*,#,A-D or a special tone: 
                   dialtone, busy, fastbusy, ringback, ring, backspace, callwaiting, 
                   holding, or loudfastbusy
     file:<filepath> - If filename only, then reads from application directory 
                       (Use | instead of : for drive specifier)
     http:<http-url> - Standard HTTP url that reference an audio file to be fetched
     cache:<cache-name> - You can play from a memory buffer/cache any items you 
                          have added with the addBufferToMediaResourceCache api.

     optional arguments are: [;duration=<duration>][;local-only][;remote-only][;repeat][;prefetch]
          
     @note 'repeat' option only makes sense for file and http URLs
     @note 'prefetch' option only makes sense for http URLs
     @note audio files may be AU, WAV or RAW formats.  Audiofiles 
           should be 16bit mono, 8khz, PCM to avoid runtime conversion.
     @note http referenced audio files must be WAV files, 
           16 or 8bit, 8Khz, Mono.

     Sample mediaUrls:
        tone:0                             - play DTMF tone 0 until participant is destroyed
        tone:*;duration=1000               - play DTMF tone 1 for 1000ms, then automatically destroy participant
        tone:dialtone;local-only           - play special tone "Dialtone" to local speaker only, until participant is destroyed
        tone:ringback;remote-only          - play special tone "Ringback" to remote participants only, until participant is destroyed
        file://ringback.wav;local-only     - play the file ringback.wav to local speaker only, until completed (automatically destroyed) or participant is manually destroyed
        file://ringback.wav;duration=1000  - play the file ringback.wav for 1000ms (or until completed, if shorter), then automatically destroy participant
        file://ringback.wav;repeat         - play the file ringback.wav, repeating when complete until participant is destroyed
        file://hi.wav;repeat;duration=9000 - play the file hi.wav for 9000ms, repeating as required, then automatically destroy the participant
        cache:welcomeprompt;local-only     - plays a prompt from the media cache with key/name "welcomeprompt" to the local speaker only
        http://www.wav.com/test.wav;repeat - play the file test.wav, repeating when complete until participant is destroyed
        http://www.wav.com/test.wav;prefetch - play the file test.wav, ensure that some audio is prefetched in order to assure smooth playback, 
                                               until completed (automatically destroyed) or participant is manually destroyed

     @param convHandle Handle of the conversation to create the 
                       RemoteParticipant in
     @param mediaUrl   Url of media to play.  See above.

     @return A handle to the newly created media participant
   */   
   virtual ParticipantHandle createMediaResourceParticipant(ConversationHandle convHandle, const resip::Uri& mediaUrl);

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
     Ends connections to the participant and removes it from all active 
     conversations.

     @param partHandle Handle of the participant to destroy
   */
   virtual void destroyParticipant(ParticipantHandle partHandle);

   /**
     Adds the specified participant to the specified conversation.       

     @param convHandle Handle of the conversation to add to
     @param partHandle Handle of the participant to add

     @note When running in sipXConversationMediaInterfaceMode you cannot
           add a participant to more than one conversation.
   */
   virtual void addParticipant(ConversationHandle convHandle, ParticipantHandle partHandle);  

   /**
     Removed the specified participant from the specified conversation.       
     The participants media to/from the conversation is stopped.  If the 
     participant no longer exists in any conversation, then they are destroyed.  
     For a remote participant this means the call will be released.

     @param convHandle Handle of the conversation to remove from
     @param partHandle Handle of the participant to remove

     @note When running in sipXConversationMediaInterfaceMode this method
           can only be used on the LocalPartipant.          
   */
   virtual void removeParticipant(ConversationHandle convHandle, ParticipantHandle partHandle);

   /**
     Removed the specified participant from the specified conversation.       
     The participants media to/from the conversation is stopped.  If the 
     participant no longer exists in any conversation, then they are destroyed.  
     For a remote participant this means the call will be released.

     @param partHandle Handle of the participant to move
     @param sourceConvHandle Handle of the conversation to move from
     @param destConvHandle   Handle of the conversation to move to

     @note When running in sipXConversationMediaInterfaceMode this method
           can only be used on the LocalPartipant.          
   */
   virtual void moveParticipant(ParticipantHandle partHandle, ConversationHandle sourceConvHandle, ConversationHandle destConvHandle);

   /**
     Modifies how the participant contributes to the particular conversation.  
     The send and receive gain can be set to a number between 0 and 100.

     @param convHandle Handle of the conversation to apply modication to
     @param partHandle Handle of the participant to modify
   */
   virtual void modifyParticipantContribution(ConversationHandle convHandle, ParticipantHandle partHandle, unsigned int inputGain, unsigned int outputGain);

   /**
     Logs a multiline representation of the current state
     of the mixing matrix.
   */
   virtual void outputBridgeMatrix();

   /**
     Signal to the participant that it should provide ringback.  Only 
     applicable to RemoteParticipants.  For SIP this causes a 180 to be sent.  
     The early flag indicates if we are sending early media or not.  
     (ie.  For SIP - SDP in 180).  

     @param partHandle Handle of the participant to alert
     @param earlyFlag Set to true to send early media
   */
   virtual void alertParticipant(ParticipantHandle partHandle, bool earlyFlag = true);      

   /**
     Signal to the participant that the call is answered.  Only applicable 
     to RemoteParticipants.  For SIP this causes a 200 to be sent.   

     @param partHandle Handle of the participant to answer
   */
   virtual void answerParticipant(ParticipantHandle partHandle);

   /**
     Rejects an incoming remote participant with the specified code.  
     Can also be used to reject an outbound participant request (due to REFER).   

     @param partHandle Handle of the participant to reject
     @param rejectCode Code sent to remote participant for rejection
   */
   virtual void rejectParticipant(ParticipantHandle partHandle, unsigned int rejectCode);

   /**
     Redirects the participant to another endpoint.  For SIP this would 
     either be a 302 response or would initiate blind transfer (REFER) 
     request, depending on the state.   

     @param partHandle Handle of the participant to redirect
     @param destination Uri of destination to redirect to
   */
   virtual void redirectParticipant(ParticipantHandle partHandle, const resip::NameAddr& destination);

   /**
     This is used for attended transfer scenarios where both participants 
     are no longer managed by the conversation manager - for SIP this will 
     send a REFER with embedded Replaces header.  Note:  Replace option cannot 
     be used with early dialogs in SIP.  

     @param partHandle Handle of the participant to redirect
     @param destPartHandle Handle ot the participant to redirect to
   */
   virtual void redirectToParticipant(ParticipantHandle partHandle, ParticipantHandle destPartHandle);

   /**
     This function is used to add a chunk of memory to a media/prompt cache.
     Cached prompts can later be played back via createMediaParticipant.
     Note:  The caller is free to dispose of the memory upon return.

     @param name   name of the cached item - used for playback
     @param buffer Data object containing the media
     @param type   Type of media that is being added. (RAW_PCM_16 = 0)
   */
   virtual void addBufferToMediaResourceCache(const resip::Data& name, const resip::Data& buffer, int type);

   /**
     Builds a session capabilties SDPContents based on the passed in ipaddress
     and codec ordering.
     Note:  Codec ordering is an array of sipX internal codecId's.  Id's for 
            codecs not loaded are ignored.
   */
   virtual void buildSessionCapabilities(const resip::Data& ipaddress, unsigned int numCodecIds, 
                                         unsigned int codecIds[], resip::SdpContents& sessionCaps);

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

   UserAgent* getUserAgent() { return mUserAgent; }

private:
   void init(int defaultSampleRate = 0, int maxSampleRate = 0);

   friend class DefaultDialogSet;
   friend class Subscription;

   friend class UserAgentShutdownCmd;
   void shutdown(void);

   // Note:  In general the following fns are not thread safe and must be called from dum process 
   //        loop only
   friend class Conversation;
   friend class OutputBridgeMixWeightsCmd;
   void registerConversation(Conversation *);
   void unregisterConversation(Conversation *);

   friend class Participant;
   void registerParticipant(Participant *);
   void unregisterParticipant(Participant *);

   friend class RemoteParticipant;
   friend class UserAgent;
   void setUserAgent(UserAgent *userAgent);

   friend class DtmfEvent;
   friend class MediaEvent;
   void notifyMediaEvent(ConversationHandle conversationHandle, int mediaConnectionId, MediaEvent::MediaEventType eventType);

   /**
     Notifies ConversationManager when an RFC2833 DTMF event is received from a
     particular remote participant.

     @param conversationHandle Handle of the conversation that received the digit
     @param mediaConnectionId sipX media connectionId for the participant who sent the signal
     @param dtmf Integer representation of the DTMF tone received (from RFC2833 event codes)
     @param duration Duration (in milliseconds) of the DTMF tone received
     @param up Set to true if the DTMF key is up (otherwise down)
   */
   void notifyDtmfEvent(ConversationHandle conversationHandle, int connectionId, int dtmf, int duration, bool up);

   friend class RemoteParticipantDialogSet;
   friend class MediaResourceParticipant;
   friend class LocalParticipant;
   friend class BridgeMixer;
   friend class MediaInterface;
   unsigned int allocateRTPPort();
   void freeRTPPort(unsigned int port);

   flowmanager::FlowManager& getFlowManager() { return mFlowManager; }

   // exists here (as opposed to RemoteParticipant) - since it is required for OPTIONS responses
   virtual void buildSdpOffer(ConversationProfile* profile, resip::SdpContents& offer);

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

private:  
   UserAgent* mUserAgent;

   typedef std::map<ConversationHandle, Conversation *> ConversationMap;
   ConversationMap mConversations;
   resip::Mutex mConversationHandleMutex;
   ConversationHandle mCurrentConversationHandle;
   ConversationHandle getNewConversationHandle();  // thread safe
   Conversation* getConversation(ConversationHandle convHandle);

   typedef std::map<ParticipantHandle, Participant *> ParticipantMap;
   ParticipantMap mParticipants;
   resip::Mutex mParticipantHandleMutex;
   ParticipantHandle mCurrentParticipantHandle;
   ParticipantHandle getNewParticipantHandle();    // thread safe
   Participant* getParticipant(ParticipantHandle partHandle);

   bool mLocalAudioEnabled;
   MediaInterfaceMode mMediaInterfaceMode;
   MediaInterfaceMode getMediaInterfaceMode() const { return mMediaInterfaceMode; }

   void post(resip::Message *message);
   void post(resip::ApplicationMessage& message, unsigned int ms=0);

   std::deque<unsigned int> mRTPPortFreeList;
   void initRTPPortFreeList();

   MediaResourceCache mMediaResourceCache;

   // FlowManager Instance
   flowmanager::FlowManager mFlowManager;

   // sipX Media related members
   void createMediaInterfaceAndMixer(bool giveFocus, ConversationHandle ownerConversationHandle, 
                                     resip::SharedPtr<MediaInterface>& mediaInterface, BridgeMixer** bridgeMixer);
   resip::SharedPtr<MediaInterface> getMediaInterface() const { resip_assert(mMediaInterface.get()); return mMediaInterface; }
   CpMediaInterfaceFactory* getMediaInterfaceFactory() { return mMediaFactory; }
   BridgeMixer* getBridgeMixer() { return mBridgeMixer; }
   CpMediaInterfaceFactory* mMediaFactory;
   resip::SharedPtr<MediaInterface> mMediaInterface;  
   BridgeMixer* mBridgeMixer;
   int mSipXTOSValue;
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
