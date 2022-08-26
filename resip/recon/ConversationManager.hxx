#if !defined(ConversationManager_hxx)
#define ConversationManager_hxx

#include "BridgeMixer.hxx"

#include <resip/stack/Uri.hxx>
#include <resip/stack/Contents.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/DialogSetHandler.hxx>
#include <resip/dum/SubscriptionHandler.hxx>
#include <resip/dum/OutOfDialogHandler.hxx>
#include <resip/dum/RedirectHandler.hxx>
#include <resip/dum/PagerMessageHandler.hxx>
#include <rutil/RWMutex.hxx>

#include <reflow/RTCPEventLoggingHandler.hxx>

#include "MediaResourceCache.hxx"
#include "MediaEvent.hxx"
#include "HandleTypes.hxx"

#include <atomic>
#include <memory>

namespace resip
{
class DialogUsageManager;
}

namespace recon
{
class Conversation;
class RelatedConversationSet;
class Participant;
class UserAgent;
class ConversationProfile;
class LocalParticipant;
class MediaResourceParticipant;
class MediaStackAdapter;
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

class ConversationManager : public resip::InviteSessionHandler,
   public resip::DialogSetHandler,
   public resip::OutOfDialogHandler,
   public resip::ClientSubscriptionHandler,
   public resip::ServerSubscriptionHandler,
   public resip::RedirectHandler,
   public resip::ClientPagerMessageHandler,
   public resip::ServerPagerMessageHandler
{
public:

   ConversationManager(std::shared_ptr<MediaStackAdapter> mediaStackAdapter);
   virtual ~ConversationManager();

   typedef enum
   {
      ParticipantType_Local,
      ParticipantType_Remote,
      ParticipantType_MediaResource,
      ParticipantType_RemoteIMPager,
      ParticipantType_RemoteIMSession
   } ParticipantType;

   typedef enum
   {
      // Create a conversation for each early fork. Accept the first fork 
      // from which a 200 is received.  Automatically kill other forks on
      // answer only.  In this mode, applications wishing to end a call 
      // before answer must destory each related participant/conversation 
      // seperately, causing a BYE to be sent to each leg that has 
      // established an early dialog, followed by a CANCEL after all 
      // related legs are destroyed
      ForkSelectAutomatic,
      // Create a conversation for each early fork. Let the application 
      // dispose of extra forks. ex: app may form conference. 
      ForkSelectManual,
      // Create a conversation for each early fork. Accept the first fork 
      // from which a 200 is received.  Automatically kill other forks on
      // answer or if the original participant is destroyed.  Also issues 
      // a single CANCEL request if original participant is destroyed before 
      // answer.
      ForkSelectAutomaticEx,
   } ParticipantForkSelectMode;

   typedef enum
   {
      // Never auto hold, only hold if holdParticipant API is used
      AutoHoldDisabled,
      // Default.  Automatically put a RemoteParticipant on-hold if there 
      // are no other participants in the conversation with them.
      AutoHoldEnabled,
      // Use this if you are broadcasting media to all participants and 
      // don't need to receive any inbound media.  All participants in
      // the conversation will be SIP held and will receive media from 
      // an added MediaParticipant. Remote offers with inactive will be
      // responded with sendonly as well. This option is useful for 
      // implementing music on hold servers. 
      AutoHoldBroadcastOnly
   } AutoHoldMode;

   typedef enum
   {
      RedirectSuccessOnTrying,    // Consider redirect to be successful on Notify SipFrag 100
      RedirectSuccessOnRinging,   // Consider redirect to be successful on Notify SipFrag 18x
      RedirectSuccessOnConnected  // Consider redirect to be successful on Notify SipFrag 2xx
   } RedirectSuccessCondition;

   ///////////////////////////////////////////////////////////////////////
   // Conversation methods  //////////////////////////////////////////////
   ///////////////////////////////////////////////////////////////////////

   /**
     Create a new empty Conversation to which participants
     can be added.

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
   virtual ConversationHandle createConversation(AutoHoldMode autoHoldMode = AutoHoldEnabled);

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
     @param conversationProfile - a specific ConversationProfile to use 
                                 for this session
     @param extraHeaders - a multimap of header names and values 
                           to add to the resulting INVITE request

     @return A handle to the newly created remote participant
   */
   virtual ParticipantHandle createRemoteParticipant(ConversationHandle convHandle, 
                                                     const resip::NameAddr& destination, 
                                                     ParticipantForkSelectMode forkSelectMode = ForkSelectAutomatic,
                                                     const std::shared_ptr<ConversationProfile>& conversationProfile = nullptr,
                                                     const std::multimap<resip::Data, resip::Data>& extraHeaders = std::multimap<resip::Data, resip::Data>());

   /**
     Creates a new remote IM participant that will be attempted to be reached
     when sendIMToParticipant is called.  This IM Participant will use Pager Mode
     IM sending.  ie:  Out-of-dialog SIP MESSAGE requests.
     Uses the specified conversation profile.

     @param destination Uri of the remote IM participant
     @param conversationProfile - a specific ConversationProfile to use 
                                  for this session

     @return A handle to the newly created remote IM participant
   */
   virtual ParticipantHandle createRemoteIMPagerParticipant(const resip::NameAddr& destination,
                                                            const std::shared_ptr<ConversationProfile>& conversationProfile = nullptr);

   /**
     Creates a new remote IM participant that will attempted to be reached
     immediately at the specified address.  This IM Partcipant will use an
     INVITE session with m=message specifier in the SDP. ForkSelectMode can 
     be set to either automatic or manual.  When ForkSelectMode is set to auto the
     conversation manager will automatically dispose of any related
     conversations that were created, due to forking.

     @param destination Uri of the remote participant to reach
     @param forkSelectMode Determines behavior if forking occurs
     @param conversationProfile - a specific ConversationProfile to use
                                  for this session
     @param extraHeaders - a multimap of header names and values
                           to add to the resulting INVITE request

     @return A handle to the newly created remote participant
   */
   virtual ParticipantHandle createRemoteIMSessionParticipant(const resip::NameAddr& destination,
      ParticipantForkSelectMode forkSelectMode = ForkSelectAutomatic,
      const std::shared_ptr<ConversationProfile>& callerProfile = nullptr,
      const std::multimap<resip::Data, resip::Data>& extraHeaders = std::multimap<resip::Data, resip::Data>());

   /**
     Creates a new media resource participant in the specified conversation.
     Media is played from a source specified by the url and may be a local
     audio file or built-in tone.  The URL can contain
     parameters that specify properties of the media playback, such as
     number of repeats.

     Media Urls are of the following format:
     tone:<tone> - Tones can be any DTMF digit 0-9,*,#,A-D or a special tone:
                   dialtone, busy, fastbusy, ringback, ring, backspace, callwaiting,
                   holding, or loudfastbusy
     file:<filepath> - If filename only, then reads from application directory
                       (Use | instead of : for drive specifier)
     cache:<cache-name> - You can play from a memory buffer/cache any items you
                          have added with the addBufferToMediaResourceCache api.
     record:<filepath> - If filename only, then writes to application directory
                         (Use | instead of : for drive specifier)
                         ;duration parameter specified max recording length in Ms
                         ;append parameter specifies to append to an existing recording
                         ;silencetime parameter specifies ms of silence to end recording

     optional arguments are: [;duration=<duration>][;repeat]

     @note 'repeat' option only makes sense for file and cache playback
     @note audio files may be AU, WAV or RAW formats.  Audiofiles
           should be 16bit mono, 8khz, PCM to avoid runtime conversion.
     @note http referenced audio files must be WAV files,
           16 or 8bit, 8Khz, Mono.

     Sample mediaUrls:
        tone:0                             - play DTMF tone 0 until participant is destroyed
        tone:1;duration=1000               - play DTMF tone 1 for 1000ms, then automatically destroy participant
        tone:ringback                      - play special tone "Ringback" to conversation until participant is manually destroyed
        file://ringback.wav                - play the file ringback.wav until completed (automatically destroyed) or participant is manually destroyed
        file://ringback.wav;duration=1000  - play the file ringback.wav for 1000ms (or until completed, if shorter), then automatically destroy participant
        file://ringback.wav;repeat         - play the file ringback.wav, repeating when complete until participant is destroyed
        file://hi.wav;repeat;duration=9000 - play the file hi.wav for 9000ms, repeating as required, then automatically destroy the participant
        cache:welcomeprompt                - plays a prompt from the media cache with key/name "welcomeprompt"
        record:recording.wav               - records all participants audio mixed togehter in a WAV file, must be manually destroyed
        record:recording.wav;duration=30000;silencetime=5000 - records all participants audio mixed togehter in a WAV file, for up to 5 mins, stop 
                                                               automatically when voice is missing for 5 seconds

     @param convHandle Handle of the conversation to create the MediaParticipant in
     @param mediaUrl   Url of media to play.  See above.

     @return A handle to the newly created media participant
   */
   virtual ParticipantHandle createMediaResourceParticipant(ConversationHandle convHandle, const resip::Uri& mediaUrl);

   /**
     Creates a new local participant in the specified conversation (if supported).
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

     @note When running in sipXConversationMediaInterfaceMode you can
           only add a non-local participant to multiple conversations if
           they share the same media interface.
   */
   virtual void addParticipant(ConversationHandle convHandle, ParticipantHandle partHandle);

   /**
     Removed the specified participant from the specified conversation.
     The participants media to/from the conversation is stopped.  If the
     participant no longer exists in any conversation, then they are destroyed.
     For a remote participant this means the call will be released.

     @param convHandle Handle of the conversation to remove from
     @param partHandle Handle of the participant to remove
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

     @note When running in sipXConversationMediaInterfaceMode you can
           only move a non-local participant between conversations if
           they share the same media interface.
   */
   virtual void moveParticipant(ParticipantHandle partHandle, ConversationHandle sourceConvHandle, ConversationHandle destConvHandle);

   /**
     Modifies how the participant contributes to the particular conversation.
     The send and receive gain can be set to a number between 0 and 100.

     @param convHandle Handle of the conversation to apply modification to
     @param partHandle Handle of the participant to modify
   */
   virtual void modifyParticipantContribution(ConversationHandle convHandle, ParticipantHandle partHandle, unsigned int inputGain, unsigned int outputGain);

   /**
     Logs a multiline representation of the current state
     of the mixing matrix.

     @param convHandle - if sipXGlobalMediaInterfaceMode is used then 0
                         is the only valid value.  Otherwise you must
                         specify a specific conversation to view.
   */
   virtual void outputBridgeMatrix(ConversationHandle convHandle = 0);

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
     @param redirectCode Only used if call isn't answered yet, must be >= 300 and <= 399.
     @param successCondition Only used if call is answered and REFER is sent
                             Specifies when an onParticipantRedirectSuccess is generated
   */
   virtual void redirectParticipant(ParticipantHandle partHandle, const resip::NameAddr& destination, unsigned int redirectCode = 302, RedirectSuccessCondition successCondition = RedirectSuccessOnConnected);

   /**
     This is used for attended transfer scenarios where both participants
     are no longer managed by the conversation manager - for SIP this will
     send a REFER with embedded Replaces header.  Note:  Replaces option cannot
     be used with early dialogs in SIP.

     @param partHandle Handle of the participant to redirect
     @param destPartHandle Handle ot the participant to redirect to
     @param successCondition Specifies when an onParticipantRedirectSuccess is generated
   */
   virtual void redirectToParticipant(ParticipantHandle partHandle, ParticipantHandle destPartHandle, RedirectSuccessCondition successCondition = RedirectSuccessOnConnected);

   /**
     Manually puts a paricipant on hold, or takes off hold without needing to
     move it in and out of any conversations.

     @param partHandle Handle of the participant to redirect
     @param hold true to put the remote participant on hold, false to unhold
   */
   virtual void holdParticipant(ParticipantHandle partHandle, bool hold);

   /**
     Sends a MESSAGE with specified Contents to RemoteParticipant, 
     RemoteIMPagerParticipant, or a RemoteIMSessionParticipant

     @param partHandle Handle of the participant to send to
     @param contents Body of SIP MESSAGE request to send
   */
   virtual void sendIMToParticipant(ParticipantHandle partHandle, std::unique_ptr<resip::Contents> contents);

   /**
     This function is used to add a chunk of memory to a media/prompt cache.
     Cached prompts can later be played back via createMediaParticipant.
     Expected format is 1-channel 16-bit 8Khz linear PCM (Assuming sipX and  
     the media framework running at 8Khz).
     Note:  The caller is free to dispose of the memory upon return.

     @param name   name of the cached item - used for playback
     @param buffer Data object containing the media
     @param type   Type of media that is being added. (RAW_PCM_16 = 0)
   */
   virtual void addBufferToMediaResourceCache(const resip::Data& name, const resip::Data& buffer, int type);

   /**
     This function is used to retrieve a chunk of memory from the media/prompt cache.
     This method is also called internally.  So appliations wishing to provide their own
     cache logic can override this method.

     @param name   name of the cached item - used for playback
     @param buffer A pointer to Data object pointer object containing the media
     @param type   A pointer to the Type of media from the cache. 
                   (Currently always: RAW_PCM_16 = 0)
   */
   virtual bool getBufferFromMediaResourceCache(const resip::Data& name, resip::Data** buffer, int* type);
   
   /**
     This function is used to start a timer on behalf of recon based application.
     The onApplicationTimer callback will get called when the timer expires.
     Note:  You cannot stop a running timer, so you may want to use a sequence
            number as the timer data and ignore timers when they fire, if
            they should be cancelled.

     @param timerId    Application specified id for this timer instance returned in callback
     @param timerData1 Application specified generic data returned in callback
     @param timerData2 Application specified generic data returned in callback
   */
   virtual void startApplicationTimer(unsigned int timerId, unsigned int timerData1, unsigned int timerData2, unsigned int durationMs);

   // Override this to handle the callback
   virtual void onApplicationTimer(unsigned int timerId, unsigned int timerData1, unsigned int timerData2) { }


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
     Notifies an application about a new remote IM pager mode participant that has sent a message

     @param partHandle Handle of the new incoming IM participant
     @param msg Includes information about the sender such as name and address, and the body of the initial IM message.
                Note:  If answerParticpant is called then the same message will be sent to the app again in a call to 
                       onReceiveIMFromParticipant.  Content processing should be done in the onReceiveIMFromParticipant
                       callback.
   */
   virtual void onIncomingIMPagerParticipant(ParticipantHandle partHandle, const resip::SipMessage& msg, ConversationProfile& conversationProfile) { rejectParticipant(partHandle, 501); }

   /**
     Notifies an application about a new remote IM session mode participant that has sent an INVITE

     @param partHandle Handle of the new incoming IM participant
     @param msg Includes information about the sender such as name and address
     @param autoAnswer Set to true if auto answer has been requested
   */
   virtual void onIncomingIMSessionParticipant(ParticipantHandle partHandle, const resip::SipMessage& msg, bool autoAnswer, ConversationProfile& conversationProfile) { rejectParticipant(partHandle, 501); }

   /**
     Notifies an application about a subsequent message from remote and existing IM participant

     @param partHandle Handle of the remote IM participant that sent the message
     @param msg Includes information about the sender such as name and address, and the body of the IM message

     @return bool indicating if message should be relayed to other appropriate participants in any conversations
             this participant belongs to.
   */
   virtual bool onReceiveIMFromParticipant(ParticipantHandle partHandle, const resip::SipMessage& msg) { return false; }

   /**
     Notifies an application that a send IM call has failed.

     @param partHandle Handle of the participant that had the failure.  Can be a
                       message from an IM relay if application returns true from
                       onReceiveIMFromParticipant.
     @param status     The SIP message indicating the failure status.
     @param contents   The contents of the message that failed to send.
   */
   virtual void onParticipantSendIMFailure(ParticipantHandle partHandle, const resip::SipMessage& status, std::unique_ptr<resip::Contents> contents) {}

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
     proceeding at the first hop.  ie: SIP 100/trying

     @param partHandle Handle of the participant that is alerting
     @param msg SIP message that caused the proceeding
   */
   virtual void onParticipantProceeding(ParticipantHandle partHandle, const resip::SipMessage& msg) { }

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
     Notifies an application that an inbound remote participant call 
     now fully connected after answering (ie: ACK was received).

     @param partHandle Handle of the participant that is connected confirmed
     @param msg SIP message that caused the connection
   */
   virtual void onParticipantConnectedConfirmed(ParticipantHandle partHandle, const resip::SipMessage& msg) {}

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

   /**
     Notifies an application when voice activity is detected to on or off from a remote participant.

     @param partHandle Handle of the remote participant, -1 if local mic or speaker
     @param on         true if voice is detected, false when voice detection has stopped
     @param inbound    true if this event if for an inbound RTP stream, false for outbound RTP streams
   */
   virtual void onParticipantVoiceActivity(recon::ParticipantHandle partHandle, bool on, bool inbound) {}

   /**
     Notifies an application about a failure in a media resource participant.

     @param partHandle Handle of the participant that terminated
     @param statusCode The status Code for the failure.
   */
   virtual void onMediaResourceParticipantFailed(ParticipantHandle partHandle) {}

   virtual RemoteParticipant* createAppropriateRemoteParticipantInstance(resip::DialogUsageManager& dum, RemoteParticipantDialogSet& rpds);
   virtual RemoteParticipant* createAppropriateRemoteParticipantInstance(ParticipantHandle partHandle, resip::DialogUsageManager& dum, RemoteParticipantDialogSet& rpds);
   virtual RemoteParticipantDialogSet* createRemoteIMSessionParticipantDialogSetInstance(
      ConversationManager::ParticipantForkSelectMode forkSelectMode = ConversationManager::ForkSelectAutomatic,
      std::shared_ptr<ConversationProfile> conversationProfile = nullptr);

   virtual void setMediaStackAdapter(std::shared_ptr<MediaStackAdapter> mediaStackAdapter);
   MediaStackAdapter& getMediaStackAdapter() { return *mMediaStackAdapter; };

   virtual void onRemoteParticipantConstructed(RemoteParticipant *rp) = 0;

protected:

   // Invite Session Handler /////////////////////////////////////////////////////
   using InviteSessionHandler::onReadyToSend;
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
   using ClientSubscriptionHandler::onReadyToSend;
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

   // ClientPagerMessageHandler ///////////////////////////////////////////////////
   virtual void onSuccess(resip::ClientPagerMessageHandle, const resip::SipMessage& status);
   virtual void onFailure(resip::ClientPagerMessageHandle, const resip::SipMessage& status, std::unique_ptr<resip::Contents> contents);

   // ServerPagerMessageHandler ///////////////////////////////////////////////////
   virtual void onMessageArrived(resip::ServerPagerMessageHandle, const resip::SipMessage& message);


   UserAgent* getUserAgent() { return mUserAgent; }

   ParticipantHandle getNewParticipantHandle();    // thread safe

   void post(resip::Message *message);
   void post(resip::ApplicationMessage& message, unsigned int ms=0);

   virtual void setUserAgent(UserAgent *userAgent);

   friend class MediaStackAdapter;
   std::shared_ptr<BridgeMixer>& getBridgeMixer() { return mBridgeMixer; }

   ConversationHandle getNewConversationHandle();  // thread safe
   Conversation* getConversation(ConversationHandle convHandle);

   std::set<ConversationHandle> getConversationHandles() const;  // thread safe
   std::set<ParticipantHandle> getParticipantHandlesByType(ParticipantType participantType) const;  // thread safe

   bool isShuttingDown() { return mShuttingDown; }

   Participant* getParticipant(ParticipantHandle partHandle); // FIXME, should be private

private:
   friend class DefaultDialogSet;
   friend class Subscription;

   friend class UserAgentShutdownCmd;
   void shutdown(void);

   // Note:  In general the following fns are not thread safe and must be called from dum process 
   //        loop only
   friend class Conversation;
   friend class SipXConversation;
   friend class OutputBridgeMixWeightsCmd;
   void registerConversation(Conversation *);
   void unregisterConversation(Conversation *);

   friend class Participant;
   void registerParticipant(Participant *);
   void unregisterParticipant(Participant *);

   friend class RemoteParticipant;
   friend class RemoteIMPagerParticipant;
   friend class SipXRemoteParticipant;
   friend class RemoteIMSessionParticipant;
   friend class UserAgent;

   /* Called periodically in the event loop to give the ConversationManager
      the opportunity to do any pending work */
   virtual void process();

   friend class DtmfEvent;
   friend class MediaEvent;
   void notifyMediaEvent(ParticipantHandle partHandle, MediaEvent::MediaEventType eventType, MediaEvent::MediaDirection direction);

   /**
     Notifies ConversationManager when an RFC2833 DTMF event is received from a
     particular remote participant.

     @param partHandle Handle of the remote participant that received the digit
     @param dtmf Integer representation of the DTMF tone received (from RFC2833 event codes)
     @param duration Duration (in milliseconds) of the DTMF tone received
     @param up Set to true if the DTMF key is up (otherwise down)
   */
   void notifyDtmfEvent(ParticipantHandle partHandle, int dtmf, int duration, bool up);

   friend class RemoteParticipantDialogSet;
   friend class SipXRemoteParticipantDialogSet;
   friend class MediaResourceParticipant;
   friend class SipXMediaResourceParticipant;
   friend class LocalParticipant;
   friend class BridgeMixer;
   friend class SipXMediaInterface;

   // exists here (as opposed to RemoteParticipant) - since it is required for OPTIONS responses
   virtual void buildSdpOffer(ConversationProfile* profile, resip::SdpContents& offer);

   friend class OutputBridgeMixWeightsCmd;

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
   friend class CreateRemoteIMPagerParticipantCmd;
   friend class SendIMToParticipantCmd;

   UserAgent* mUserAgent;
   std::shared_ptr<MediaStackAdapter> mMediaStackAdapter;
   bool mShuttingDown;

   typedef std::map<ConversationHandle, Conversation *> ConversationMap;
   ConversationMap mConversations;
   mutable resip::RWMutex mConversationHandlesMutex;
   std::atomic<ConversationHandle> mCurrentConversationHandle;
   std::set<ConversationHandle> mConversationHandles;

   typedef std::map<ParticipantHandle, Participant *> ParticipantMap;
   ParticipantMap mParticipants;
   mutable resip::RWMutex mParticipantHandlesMutex;
   std::atomic<ParticipantHandle> mCurrentParticipantHandle;
   std::map<ParticipantType, std::set<ParticipantHandle>> mParticipantHandlesByType;

   MediaResourceCache mMediaResourceCache;

   std::shared_ptr<BridgeMixer> mBridgeMixer;
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
