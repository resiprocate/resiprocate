#if !defined(ConversationManager_hxx)
#define ConversationManager_hxx

#ifdef WIN32
#define BOOST__STDC_CONSTANT_MACROS_DEFINED  // eliminate duplicate define warnings under windows
#endif

#include "media/MediaStack.hxx"
#include "media/CodecFactory.hxx"

#include "BridgeMixer.hxx"

#include <resip/stack/Uri.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/DialogSetHandler.hxx>
#include <resip/dum/SubscriptionHandler.hxx>
#include <resip/dum/OutOfDialogHandler.hxx>
#include <resip/dum/RedirectHandler.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <rutil/Mutex.hxx>

#include "UserAgentMasterProfile.hxx"
#include "MediaResourceCache.hxx"
#include "MediaEvent.hxx"
#include "FlowManager.hxx"
#include "ConversationProfile.hxx"
#include "RTPPortAllocator.hxx"
#include "RegistrationManager.hxx"

namespace recon
{
class Conversation;
class Participant;
class ConversationProfile;
class RemoteParticipant;
class UserAgent;
class RemoteParticipantDialogSet;

typedef unsigned int ConversationHandle;
typedef unsigned int ParticipantHandle;

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
   ConversationManager(UserAgent& ua);
   virtual ~ConversationManager();

   typedef enum
   {
      ForkSelectAutomatic, // create a conversation for each early fork. accept the first fork from which a 200 is received.  automatically kill other forks
      ForkSelectManual     // create a conversation for each early fork. let the application dispose of extra forks. ex: app may form conference.
   } ParticipantForkSelectMode;

   /**
     Destroys each conversation and each participant; should be called by the UserAgent at shutdown time.
   */
   void shutdown(void);

   resip::AppDialogSetFactory* getAppDialogSetFactory();

   ///////////////////////////////////////////////////////////////////////
   // Conversation Profile methods  //////////////////////////////////////
   ///////////////////////////////////////////////////////////////////////

   /**
     Adds a Conversation Profile to be managed, by the user agent.  SIP Registration
     is performed, if required.

     @param conversationProfile Profile to add
     @param defaultOutgoing Set to true to set this profile as the default
                            profile to use for outbound calls.
   */
   ConversationProfileHandle addConversationProfile(resip::SharedPtr<ConversationProfile> conversationProfile, bool defaultOutgoing=true); // thread safe

   /**
     Sets an existing Conversation Profile to the default profile to
     use for outbound calls.

     @param handle ConversationProfile handle to use
   */
   void setDefaultOutgoingConversationProfile(ConversationProfileHandle handle);

   /**
     Destroys an existing Conversation Profile.  SIP un-registration is
     performed, if required.

     @param handle ConversationProfile handle to use

     @note If this ConversationProfile is currently the default outbound
           profile, then the next profile in the list will become the default
   */
   void destroyConversationProfile(ConversationProfileHandle handle);

   /**
    * Creates a new conversation profile which is based on the first conversation
    * profile. This is required in order to modify some parameters from a base
    * handle (depending on your calling model).
    */
   ConversationProfileHandle cloneConversationProfile( ConversationProfileHandle handle );

   ///////////////////////////////////////////////////////////////////////
   // Conversation methods  //////////////////////////////////////////////
   ///////////////////////////////////////////////////////////////////////

   /**
     Create a new empty Conversation to which participants
     can be added. This method uses the default outgoing conversation
     profile. If you need a different profile, use the method below.

     @return A handle to the newly created conversation.
   */
   virtual ConversationHandle createConversation();

   /**
     Create a new empty Conversation to which participants
     can be added.

     @param cpHandle A handle to the conversation profile which should be used to
            create this conversation.
     @return A handle to the newly created conversation.
   */
   virtual ConversationHandle createConversation(ConversationProfileHandle cpHandle);

   // Note that it is possible for a media to be enabled, but inactive
   enum MediaDirection
   {
      MediaDirection_None,
      MediaDirection_SendReceive,
      MediaDirection_SendOnly,
      MediaDirection_ReceiveOnly,
      MediaDirection_Inactive
   };

   enum SecureMediaMode
   {
      NoSecureMedia, // Will accept secure media offers, but will not offer secure media in calls placed
      Srtp,       // SRTP with keying outside of media stream - ie. SDES key negotiation via SDP
      SrtpDtls    // SRTP with DTLS key negotiation
   };

   enum SecureMediaCryptoSuite
   {
      SRTP_AES_CM_128_HMAC_SHA1_32,
      SRTP_AES_CM_128_HMAC_SHA1_80
   };

   struct MediaAttributes
   {
      MediaAttributes()
         : audioDirection(MediaDirection_SendReceive),
           videoDirection(MediaDirection_None),
           secureMediaRequired(false),
           secureMediaMode(NoSecureMedia),
           secureMediaDefaultCryptoSuite(SRTP_AES_CM_128_HMAC_SHA1_80)
      {
      }

      MediaAttributes(const MediaAttributes& rhs)
         : audioDirection(rhs.audioDirection),
           videoDirection(rhs.videoDirection),
           secureMediaRequired(rhs.secureMediaRequired),
           secureMediaMode(rhs.secureMediaMode),
           secureMediaDefaultCryptoSuite(rhs.secureMediaDefaultCryptoSuite)
      {
      }

      MediaDirection audioDirection;  /* media direction */
      MediaDirection videoDirection;  /* video media direction */

      /** 
        Get/Set the whether Secure Media is required (default is false).
        - if required then SAVP transport protocol is signalled in SDP offers
        - if not required then AVP transport protocol is signalled in SDP offers 
          and encryption=optional attribute is added
      */
      bool secureMediaRequired;

      /** 
        Get/Set the secure media mode that will be used for sending/receiving media packets.
        NoSecureMedia - don't use any secure media strategies - RTP packets are sent 
                        unencrypted via the specified transport.
        Srtp          - use SRTP with keying outside of media stream - ie. SDES key negotiation via SDP (default)
        SrtpDtls      - use SRTP with DTLS key negotiation

        @note If TurnTlsAllocation NatTraversalMode is used, then media will be secured from 
              this UA to the TURN the turn server, even if NoSecureMedia is used.
      */
      SecureMediaMode secureMediaMode;

      /** 
        Get/Set the secure media default crypto suite.  The default crypto suite is used when
        forming SDP offers (SDES only - does not apply to DTLS-SRTP).
        SRTP_AES_CM_128_HMAC_SHA1_32 - Counter Mode AES 128 bit encryption with 
                                       32bit authenication code 
        SRTP_AES_CM_128_HMAC_SHA1_80 - Counter Mode AES 128 bit encryption with 
                                       80bit authenication code (default)
      */
      SecureMediaCryptoSuite secureMediaDefaultCryptoSuite;
   };

   struct CallAttributes
   {
      CallAttributes() : forkSelectMode(ForkSelectAutomatic),
                         isAnonymous(false),
                         requestAutoAnswer(false),
                         replacesDialogId(resip::Data::Empty,resip::Data::Empty,resip::Data::Empty),
                         joinDialogId(resip::Data::Empty,resip::Data::Empty,resip::Data::Empty)
      {
      }

      CallAttributes(const CallAttributes& rhs) : forkSelectMode(rhs.forkSelectMode),
                                                  isAnonymous(rhs.isAnonymous),
                                                  requestAutoAnswer(rhs.requestAutoAnswer),
                                                  replacesDialogId(rhs.replacesDialogId),
                                                  joinDialogId(rhs.joinDialogId)
      {
      }

      /** Determine behaviour if forking occurs */
      ParticipantForkSelectMode forkSelectMode;
      
      /** Enable basic support for RFC3323 */
      bool isAnonymous; 

      /** Include an Answer-Mode header */
      bool requestAutoAnswer;
      
      /** If set, a Replaces header is added to the initial INVITE for this call */
      resip::DialogId replacesDialogId;
      
      /** If set, a Join heaer is added to the initial INVITE for this call */
      resip::DialogId joinDialogId;
   };

   /**
     Changes the direction of each supported media type.

     @param partHandle The (remote) participant to whom we wish to change the media
     @param mediaAttributes The new media directions for each supported media type
     @param sendOffer If true, send an offer immediately; otherwise just apply the preferences
                      specified in mediaAttribs to the next offer/answer
   */
   virtual void updateMedia(ParticipantHandle partHandle, const MediaAttributes& mediaAttribs, bool sendOffer=true);

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
     @param mediaAttributes Determines which media types are offered
     @param callAttributes Sets various call attributes including fork select mode, anonymous, Replaces/Join headers

     @return A handle to the newly created remote participant
   */
   virtual ParticipantHandle createRemoteParticipant(ConversationHandle convHandle, resip::NameAddr& destination, const MediaAttributes& mediaAttributes, const CallAttributes& callAttributes);

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
   virtual ParticipantHandle createMediaResourceParticipant(ConversationHandle convHandle, resip::Uri& mediaUrl);

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
     @param appDefinedReason Optional text to include in a Reason header
   */
   virtual void destroyParticipant(ParticipantHandle partHandle, const resip::Data& appDefinedReason = resip::Data::Empty);

   /**
     Adds the specified participant to the specified conversation.

     @param convHandle Handle of the conversation to add to
     @param partHandle Handle of the participant to add
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
     @param bTriggerHold true, if a hold should be triggered
   */
   virtual void moveParticipant(ParticipantHandle partHandle, ConversationHandle sourceConvHandle, ConversationHandle destConvHandle, bool bTriggerHold = false);

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
     Signal to the participant that the call is answered, or that the
     media changes requested in an updated offer are accepted.  Only applicable
     to RemoteParticipants.  For SIP this causes a 200 to be sent.

     @param partHandle Handle of the participant to answer
   */
   virtual void answerParticipant(ParticipantHandle partHandle, MediaAttributes mediaAttributes);

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
   virtual void redirectParticipant(ParticipantHandle partHandle, resip::NameAddr& destination);

   /**
     This is used for attended transfer scenarios where both participants
     are no longer managed by the conversation manager  – for SIP this will
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
   virtual void addBufferToMediaResourceCache(resip::Data& name, resip::Data& buffer, int type);

   /**
     Builds a session capabilities SDPContents based on the passed in ipaddress.
     Invokes the CodecFactory for the (ordered) list of codecs.
   */
   virtual void buildSessionCapabilities(bool includeAudio, bool includeVideo, resip::Data& ipaddress, resip::SdpContents& sessionCaps, const resip::Data& sessionName=" ");

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
   virtual void onIncomingParticipant(ParticipantHandle partHandle, const resip::SipMessage& msg, bool autoAnswer) = 0;

   /**
     Notifies an application about a new remote participant that is requesting
     to join an existing conversation.

     @param newPartHandle Handle of the new incoming participant
     @param existingPartHandle Handle of the participant in the existing conversation
     @param msg Includes information about the caller such as name and address
   */
   virtual void onIncomingJoinRequest(ParticipantHandle newPartHandle, ParticipantHandle existingPartHandle, const resip::SipMessage& msg) = 0;

   /**
     Notifies an application about a new remote participant that is requesting
     to replace an existing participant in a conversation.

     @param newPartHandle Handle of the new incoming participant
     @param existingPartHandle Handle of the participant in the existing conversation
     @param msg Includes information about the caller such as name and address
   */
   virtual void onIncomingTransferRequest(ParticipantHandle newPartHandle, ParticipantHandle existingPartHandle, const resip::SipMessage& msg) = 0;

   /**
     Notifies an application when a new RtpStream is created.
     This allows the application to correlate media streams and participants.

     @param partHandle Handle of the participant whose media stream has been created
     @param rtpStream Media stream object
   */
   virtual void onMediaStreamCreated(ParticipantHandle partHandle, boost::shared_ptr<RtpStream> rtpStream) = 0;

   /**
     Notifies an application about a new remote participant that is trying
     to be contacted.  This event is required to notify the application if a
     call request has been initiated by a signaling mechanism other than
     the application, such as an out-of-dialog REFER request.

     @param partHandle Handle of the new incoming participant
     @param msg Includes information about the destination requested
                to be attempted
   */
   virtual void onRequestOutgoingParticipant(ParticipantHandle partHandle, const resip::SipMessage& msg) = 0;
   virtual void onRequestOutgoingParticipant(recon::ParticipantHandle transferTargetHandle, recon::ParticipantHandle transfererHandle, const resip::SipMessage& msg) = 0;

   /**
     Notifies an application about a disconnect by a remote participant.
     For SIP this could be a BYE or a CANCEL request.

     @param partHandle Handle of the participant that terminated
     @param statusCode The status Code for the termination.
   */
   virtual void onParticipantTerminated(ParticipantHandle partHandle, unsigned int statusCode, resip::InviteSessionHandler::TerminatedReason terminateReason, const resip::SipMessage* msg) = 0;

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
     Notifies an application that the remote participant is redirecting them; the new
     participant (with possibly a new ParticipantHandle) will be available in a subsequent
     onNewOutgoingParticipant callback

     @param partHandle Handle of the participant that is redirecting us
     @param msg 3xx
   */
   virtual void onLocalParticipantRedirected(ParticipantHandle partHandle, const resip::SipMessage& msg) = 0;

   /**
     Notifies an application that an attempt is being made to contact a new RemoteParticipant.

     @param Handle of the new RemoteParticipant that is being contacted
     @param msg INVITE
   */
   virtual void onNewOutgoingParticipant(ParticipantHandle partHandle, const resip::SipMessage& msg) = 0;

   /**
     Notifies an application that a remote participant call attempt is
     alerting the remote party.

     @param partHandle Handle of the participant that is alerting
     @param msg SIP message that caused the alerting
   */
   virtual void onParticipantAlerting(ParticipantHandle partHandle, const resip::SipMessage& msg) = 0;

   /**
     Notifies an application that a remote participant is sending early media.

     @param partHandle Handle of the participant that is sending early media
     @param msg SIP message that contains the early media session description
   */
   virtual void onParticipantEarlyMedia(ParticipantHandle partHandle, const resip::SipMessage& msg) = 0;

   /**
     Notifies an application that a remote participant call attempt is
     now connected.

     @param partHandle Handle of the participant that is connected
     @param msg SIP message that caused the connection
   */
   virtual void onParticipantConnected(ParticipantHandle partHandle, const resip::SipMessage& msg) = 0;

   /**
     Notifies an application that a redirect request is progressing.
     Implies a NOTIFY w/SipFrag status < 200.  Safe to ignore.

     @param partHandle Handle of the participant that was redirected
   */
   virtual void onParticipantRedirectProgress(ParticipantHandle partHandle, const resip::SipMessage* msg=NULL) {}

   /**
     Notifies an application that a redirect request has succeeded.
     Indicates blind transfer or attended transfer status.

     @param partHandle Handle of the participant that was redirected
   */
   virtual void onParticipantRedirectSuccess(ParticipantHandle partHandle, const resip::SipMessage* msg=NULL) = 0;

   /**
     Notifies an application that a redirect request has failed.
     Indicates blind transfer or attended transfer status.

     @param partHandle Handle of the participant that was redirected
   */
   virtual void onParticipantRedirectFailure(ParticipantHandle partHandle, unsigned int statusCode, const resip::Data& reasonPhrase=resip::Data::Empty, const resip::SipMessage* msg=NULL) = 0;

   /**
     Notifies an application when an RFC2833 DTMF event is received from a
     particular remote participant.

     @param partHandle Handle of the participant that received the digit
     @param dtmf Integer representation of the DTMF tone received
     @param duration Duration of the DTMF tone received
     @param up Set to true if the DTMF key is up (otherwise down)
   */
   virtual void onDtmfEvent(ParticipantHandle partHandle, int dtmf, int duration, bool up) = 0;

   /**
     Notifies an application that a sip info message has been received from
     a specific remote participant.

     !jjg! not super useful, since there's no way to accept/reject the INFO unless we expose the InviteSessionHandle;
           so probably just overriding the InviteSessionHandler version of onInfo is good enough for most apps

     @param partHandle Handle of the participant that sent the INFO
     @param msg the corresponding sip message
   */
   virtual void onInfo(ParticipantHandle partHandle, const resip::SipMessage& msg) {}

   /**
    Notifies an application that a RemoteParticipant is requesting changes to the state of media
    streams associated with this call.

    !jjg! currently this change MUST be accepted or rejected manually by calling
          ConversationManager::answerParticipant(..) or ConversationManager::rejectParticipant(..)

    @param partHandle Handle of the participant that is requesting the media change
    @param remoteAudioState The requested audio direction at the remote participant side
    @param removeVideoState The requested video direction at the remote participant side
   */
   virtual void onParticipantMediaChangeRequested(ParticipantHandle partHandle, ConversationManager::MediaDirection remoteAudioState, ConversationManager::MediaDirection remoteVideoState, const resip::SipMessage* msg) = 0;

   /**
    Notifies an application that a request to change the state of the media streams has been
    accepted or rejected.  The request would normally be initiated by the application calling
    ConversationManager::updateMedia(..)

    @param partHandle Handle of the participant that accepted or rejected the media change
    @param remoteAudioState The new audio direction at this end
    @param removeVideoState The new video direction at this end
   */
   virtual void onParticipantMediaChanged(ParticipantHandle partHandle, ConversationManager::MediaDirection audioState, ConversationManager::MediaDirection videoState, const resip::SipMessage* msg) = 0;

   /**
     Give the application a chance to adorn the outgoing INVITE.
     .jjg. todo - somehow consolidate this with DUM's InviteSessionHandler::onReadyToSend(..), which
     is unfortunately NOT called for outgoing INVITEs since DUM lets you muck with the SipMessage
     directly before you call send(..)
    */
   virtual void onReadyToSendInvite(ParticipantHandle partHandle, resip::SipMessage& msg) = 0;

   ///////////////////////////////////////////////////////////////////////
   // Media Related Methods - this may not be the right spot for these - move to LocalParticipant?
   ///////////////////////////////////////////////////////////////////////

   virtual void setSpeakerVolume(int volume);
   virtual void setMicrophoneGain(int gain);
   virtual void muteMicrophone(bool mute);
   virtual void enableEchoCancel(bool enable);
   virtual void enableAutoGainControl(bool enable);
   virtual void enableNoiseReduction(bool enable);


   ///////////////////////////////////////////////////////////////////////
   // Recording related methods
   ///////////////////////////////////////////////////////////////////////

   /**
     The startRecording method creates a file containing the output from the
     mixer.

     A video file will only be saved if all of width, height, and
     videoFrameRate are specified. If even one of them is not specified, video
     will not be recorded.

     @param filePath the full path and name for the file
     @param width the width of the video frame to record
     @param height the height of the video frame to record
     @param videoFrameRate the number of video frames per second to record
    */
   virtual void startRecording(
      const ConversationHandle&  convHandle,
      const std::wstring& filePath,
      unsigned long width = 0,
      unsigned long height = 0,
      int videoFrameRate = 0 );

   /**
     If the mixer recording is active, this method will stop the recording and
     close the file. If the mixer is not recording, the method will do nothing.
    */
   virtual void stopRecording(const ConversationHandle& convHandle);

   /**
     Returns a handle to the mixer object associated with a specific
     conversation.

     @param convHandle the handle to the conversation
     @return a shared pointer to the mixer object, or a null shared
             pointer if the object doesn't exist.
    */
   boost::shared_ptr<Mixer> getConversationMixer(ConversationHandle convHandle);

protected:

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

   virtual bool isOutOfDialogReferSupported() const;

   /**
     Retrieve a shared pointer to the actual conversation profile, using
     the handle as a key. This should normally not be used except for
     integration with resip (as it requires the direct profile in certain
     places).

     NB : the other xxxConversationProfile methods are asynchronous, but
          this method is not.

     @param cpHandle the "handle" of the conversation profile in question.
     @return a shared pointer to the internal conversation profile object.
    */
   resip::SharedPtr<ConversationProfile> getConversationProfile( ConversationProfileHandle cpHandle );

   /**
     Given a specific conversation handle, returns a shared pointer to the
     conversation object.

     @param convHandle the handle to the conversation
     @return a shared pointer to the conversation object, or a null shared
             pointer if the object doesn't exist.
    */
   Conversation* getConversation(ConversationHandle convHandle);

protected:
   resip::DialogUsageManager* mDum;

   MediaStack* getMediaStack() { return mMediaStack; }

private:
   friend class DefaultDialogSet;
   friend class Subscription;

   friend class UserAgentShutdownCmd;

   // Note:  In general the following fns are not thread safe and must be called from dum process
   //        loop only
   friend class Conversation;
   friend class OutputBridgeMixWeightsCmd;
   void registerConversation(Conversation*);
   void unregisterConversation(Conversation*);
   BridgeMixer& getBridgeMixer();

   void addConversationProfileImpl(ConversationProfileHandle handle, resip::SharedPtr<ConversationProfile> conversationProfile, bool defaultOutgoing=true);
   void setDefaultOutgoingConversationProfileImpl(ConversationProfileHandle handle);
   void destroyConversationProfileImpl(ConversationProfileHandle handle);
   resip::SharedPtr<ConversationProfile> getDefaultOutgoingConversationProfile();
   resip::SharedPtr<ConversationProfile> getIncomingConversationProfile(const resip::SipMessage& msg);  // returns the most appropriate conversation profile for the message

   friend class Participant;
   void registerParticipant(Participant *);
   void unregisterParticipant(Participant *);

   friend class RemoteParticipant;
   friend class UserAgent;
   friend class RegistrationManager;

   friend class MediaEvent;
   void onMediaEvent(MediaEvent::MediaEventType eventType);

   friend class RemoteParticipantDialogSet;
   friend class MediaResourceParticipant;
   friend class LocalParticipant;
   friend class BridgeMixer;

   flowmanager::FlowManager& getFlowManager() { return mFlowManager; }

   // exists here (as opposed to RemoteParticipant) - since it is required for OPTIONS responses
   virtual void buildSdpOffer(ConversationProfile* profile, resip::SdpContents& offer);

   friend class MediaResourceParticipantDeleterCmd;
   friend class AddConversationProfileCmd;
   friend class SetDefaultOutgoingConversationProfileCmd;
   friend class DestroyConversationProfileCmd;
   friend class CreateConversationCmd;
   friend class UpdateConversationProfileCmd;
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
   friend class UserAgentServerAuthManager;
   friend class CreateSubscriptionCmd;

private:
   typedef std::map<ConversationHandle, Conversation*> ConversationMap;
   ConversationMap mConversations;
   resip::Mutex mConversationHandleMutex;
   ConversationHandle mCurrentConversationHandle;
   ConversationHandle getNewConversationHandle();  // thread safe

   // Conversation Profile Storage
   typedef std::map<ConversationProfileHandle, resip::SharedPtr<ConversationProfile> > ConversationProfileMap;
   ConversationProfileMap mConversationProfiles;
   resip::Mutex mConversationProfileHandleMutex;
   ConversationProfileHandle mCurrentConversationProfileHandle;
   ConversationProfileHandle mDefaultOutgoingConversationProfileHandle;
   ConversationProfileHandle getNewConversationProfileHandle();  // thread safe

   typedef std::map<resip::Uri, ConversationProfileHandle> UriToConversationProfileMap;
   UriToConversationProfileMap mMapDefaultIncomingConvProfile;

   // Allocator for local RTP ports
   RTPPortAllocator* mRTPAllocator;

   typedef std::map<ParticipantHandle, Participant *> ParticipantMap;
   ParticipantMap mParticipants;
   resip::Mutex mParticipantHandleMutex;
   ParticipantHandle mCurrentParticipantHandle;
   ParticipantHandle getNewParticipantHandle();    // thread safe
   Participant* getParticipant(ParticipantHandle partHandle);
   RemoteParticipant* getRemoteParticipantFromMediaConnectionId(int mediaConnectionId);
   bool mLocalAudioEnabled;

   MediaResourceCache mMediaResourceCache;

   // FlowManager Instance
   flowmanager::FlowManager mFlowManager;

   // RegistrationManager Instance
   RegistrationManager* mRegManager;

   // sipX Media related members
   //virtual OsStatus post(const OsMsg& msg);

   MediaStack* mMediaStack;
   CodecFactory* mCodecFactory;
   //CpMediaInterfaceFactory* mMediaFactory;
   //CpMediaInterface* mMediaInterface;

   BridgeMixer mBridgeMixer;
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
