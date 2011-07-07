#include "ConversationManager.hxx"

#include "sdp/SdpHelperResip.hxx"
#include "sdp/Sdp.hxx"

#include "RemoteParticipant.hxx"
#include "Conversation.hxx"
#include "UserAgent.hxx"
#include "DtmfEvent.hxx"
#include "ReconSubsystem.hxx"
#include "media/Mixer.hxx"
#include "ConversationManagerCmds.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/Random.hxx>
#include <resip/stack/SipFrag.hxx>
#include <resip/stack/ExtensionHeader.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/ClientInviteSession.hxx>
#include <resip/dum/ServerInviteSession.hxx>
#include <resip/dum/ClientSubscription.hxx>
#include <resip/dum/ServerOutOfDialogReq.hxx>
#include <resip/dum/ServerSubscription.hxx>

#include <rutil/WinLeakCheck.hxx>

#include <client/IceCandidate.hxx>

using namespace recon;
using namespace sdpcontainer;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

MediaStack::MediaType
toReconMediaType(sdpcontainer::SdpMediaLine::SdpMediaType mediaType)
{
   switch (mediaType)
   {
   case SdpMediaLine::MEDIA_TYPE_AUDIO:
      return MediaStack::MediaType_Audio;
   case SdpMediaLine::MEDIA_TYPE_VIDEO:
      return MediaStack::MediaType_Video;
   case SdpMediaLine::MEDIA_TYPE_NONE:
      return MediaStack::MediaType_None;
   default:
      assert(0);
   }
   return MediaStack::MediaType_None;
}

sdpcontainer::SdpMediaLine::SdpMediaType
toSdpMediaType(MediaStack::MediaType mediaType)
{
   switch (mediaType)
   {
   case MediaStack::MediaType_Audio:
      return sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO;
   case MediaStack::MediaType_Video:
      return sdpcontainer::SdpMediaLine::MEDIA_TYPE_VIDEO;
   case MediaStack::MediaType_None:
      return sdpcontainer::SdpMediaLine::MEDIA_TYPE_NONE;
   default:
      assert(0);
   }
   return sdpcontainer::SdpMediaLine::MEDIA_TYPE_NONE;
}

// UAC
RemoteParticipant::RemoteParticipant(ParticipantHandle partHandle,
                                     ConversationManager& conversationManager,
                                     DialogUsageManager& dum,
                                     RemoteParticipantDialogSet& remoteParticipantDialogSet)
: Participant(partHandle, conversationManager),
  AppDialog(dum),
  mDum(dum),
  mDialogSet(remoteParticipantDialogSet),
  mDialogId(Data::Empty, Data::Empty, Data::Empty),
  mState(Connecting),
  mOfferRequired(false),
  mLocalSdp(0),
  mRemoteSdp(0),
  mLocalSdpSessionId(0),
  mLocalSdpVersion(1)
{
   InfoLog(<< "RemoteParticipant created (UAC), handle=" << mHandle);
}

// UAS - or forked leg
RemoteParticipant::RemoteParticipant(ConversationManager& conversationManager,
                                     DialogUsageManager& dum,
                                     RemoteParticipantDialogSet& remoteParticipantDialogSet,
                                     const RemoteParticipant* orig)
: Participant(conversationManager),
  AppDialog(dum),
  mDum(dum),
  mDialogSet(remoteParticipantDialogSet),
  mDialogId(Data::Empty, Data::Empty, Data::Empty),
  mState(Connecting),
  mOfferRequired(false),
  mLocalSdp(0),
  mRemoteSdp(0),
  mLocalSdpSessionId((orig ? orig->mLocalSdpSessionId : 0)),
  mLocalSdpVersion((orig ? orig->mLocalSdpVersion : 1))
{
   InfoLog(<< "RemoteParticipant created (UAS or forked leg), handle=" << mHandle);
}

RemoteParticipant::~RemoteParticipant()
{
   // !jjg! is DUM still alive?
   if(!mDialogId.getCallId().empty())
   {
      mDialogSet.removeDialog(mDialogId);
   }

   // unregister from Conversations
   // Note:  ideally this functionality would exist in Participant Base class - but dynamic_cast required in unregisterParticipant will not work
   ConversationMap::iterator it;
   for(it = mConversations.begin(); it != mConversations.end(); ++it)
   {
      // remove the streams for this participant from the mixer; need to handle the following cases:
      // 1) forking, where we have several RemoteParticipants sharing an RtpStream and the RtpStream has
      //    multiple sources
      // 2) a single RemoteParticipant who receives from multiple sources (Asterisk 'echo test' is an example)
      // 3) a single RemoteParticipant who receives from a single source
      if (mDialogSet.getDialogCount() == 0)
      {
         const RemoteParticipantDialogSet::RtpStreamMap& streams = mDialogSet.getRtpStreams();
         RemoteParticipantDialogSet::RtpStreamMap::const_iterator iter = streams.begin();
         while(iter != streams.end())
         {
            it->second->getMixer()->removeRtpStream(iter->second);
            ++iter;
         }
      }

      it->second->unregisterParticipant(this);
   }
   mConversations.clear();

   MapMediaTypeToConnection::iterator connIt = m_onRtpStreamClosedConns.begin();
   for (; connIt != m_onRtpStreamClosedConns.end(); ++connIt)
   {
      connIt->second.disconnect();
   }
   m_onRtpStreamClosedConns.clear();

   // Delete Sdp memory
   if(mLocalSdp) delete mLocalSdp;
   if(mRemoteSdp) delete mRemoteSdp;

   mConversationManager.mCodecFactory->releaseLicenses(mLicensedCodecs);

   InfoLog(<< "RemoteParticipant destroyed, handle=" << mHandle);
}

bool
RemoteParticipant::isHolding()
{
   if (mMediaHoldStates.size() == 0)
   {
      return false;
   }

   // if any of the media are on hold, return true
   MediaHoldStateMap::const_iterator it = mMediaHoldStates.begin();
   for (; it != mMediaHoldStates.end(); ++it)
   {
      if (it->second)
      {
         return true;
      }
   }
   return false;
}

unsigned int
RemoteParticipant::getLocalRTPPort(const sdpcontainer::SdpMediaLine::SdpMediaType& mediaType, bool v6, ConversationProfile* profile /* = NULL */)
{
   boost::shared_ptr<RtpStream> rtpStream = getRtpStream(mediaType);
   unsigned int port = mDialogSet.getLocalRTPPort(mediaType, v6, profile);
   if (!rtpStream)
   {
      // the stream may have just been created by the call to getLocalRTPPort(..)
      rtpStream = getRtpStream(mediaType);
      if (rtpStream)
      {
         mConversationManager.onMediaStreamCreated(mHandle, rtpStream);
      }
   }
   return port;
}

void
RemoteParticipant::initiateRemoteCall(resip::SharedPtr<ConversationProfile> profile, const resip::NameAddr& destination, Conversation* conversation, const ConversationManager::MediaAttributes& mediaAttributes, const ConversationManager::CallAttributes& callAttributes)
{
   SdpContents offer;

   std::set<sdpcontainer::SdpMediaLine::SdpMediaType> existingMediaTypes;
   Conversation::ParticipantMap& existingParticipants = conversation->getParticipants();
   for (Conversation::ParticipantMap::iterator it = existingParticipants.begin();
        it != existingParticipants.end();
        ++it)
   {
      RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(it->second.getParticipant());
      if (remoteParticipant)
      {
         const std::map<sdpcontainer::SdpMediaLine::SdpMediaType, boost::shared_ptr<RtpStream> >& rtpStreams = remoteParticipant->mDialogSet.getRtpStreams();
         std::map<sdpcontainer::SdpMediaLine::SdpMediaType, boost::shared_ptr<RtpStream> >::const_iterator streamIt = rtpStreams.begin();
         for (; streamIt != rtpStreams.end(); ++streamIt)
         {
            if (streamIt->second.get())
            {
               existingMediaTypes.insert(streamIt->first);
            }
         }
      }
   }

   // offer these m= lines
   mDialogSet.audioDirection() = mediaAttributes.audioDirection;
   mDialogSet.videoDirection() = mediaAttributes.videoDirection;

   // set security-related params
   if(mediaAttributes.secureMediaMode == ConversationManager::SrtpDtls &&
      profile->natTraversalMode() == flowmanager::MediaStream::TurnAllocation)
   {
      WarningLog(<< "You cannot use SrtpDtls and a Turn allocation at the same time - disabling SrtpDtls!");
      mDialogSet.setSecureMediaMode(ConversationManager::NoSecureMedia);
   }
   else
   {
      mDialogSet.setSecureMediaMode(mediaAttributes.secureMediaMode);
   }

   mDialogSet.setSecureMediaRequired(mediaAttributes.secureMediaRequired);
   mDialogSet.setSrtpCryptoSuite(mediaAttributes.secureMediaDefaultCryptoSuite);

   // !jjg! fixme - should be handling media in general, not just audio and video
   if ((existingMediaTypes.size() == 0 && mDialogSet.audioActive()) || (existingMediaTypes.size() > 0 && (existingMediaTypes.find(SdpMediaLine::MEDIA_TYPE_AUDIO) != existingMediaTypes.end())))
   {
      mMediaHoldStates[SdpMediaLine::MEDIA_TYPE_AUDIO] = false;
   }
   if ((existingMediaTypes.size() == 0 && mDialogSet.videoActive()) || (existingMediaTypes.size() > 0 && (existingMediaTypes.find(SdpMediaLine::MEDIA_TYPE_VIDEO) != existingMediaTypes.end())))
   {
      mMediaHoldStates[SdpMediaLine::MEDIA_TYPE_VIDEO] = false;
   }

   buildSdpOffer(profile.get(), mMediaHoldStates, offer, existingMediaTypes);

   SharedPtr<SipMessage> invitemsg = mDum.makeInviteSession(
      destination,
      (callAttributes.isAnonymous ? profile->getAnonymousUserProfile() : (resip::SharedPtr<UserProfile>)profile),
      &offer,
      &mDialogSet);

   // Modify the INVITE SIP message according to the privacy rules
   if (callAttributes.isAnonymous/*profile->isAnonymous()*/ &&
       (!profile->alternativePrivacyHeader().empty() || !profile->rewriteFromHeaderIfAnonymous()))
   {
      if (profile->alternativePrivacyHeader().empty())
         profile->alternativePrivacyHeader() = "id";

      if (invitemsg->exists(h_Privacies))
         invitemsg->remove(h_Privacies);

      resip::HeaderFieldValueList values;
      values.push_back(new resip::HeaderFieldValue(profile->alternativePrivacyHeader().c_str(),
         profile->alternativePrivacyHeader().size()));

      invitemsg->setRawHeader(&values, resip::Headers::Privacy);
   }

   if (callAttributes.requestAutoAnswer)
   {
      invitemsg->header(h_AnswerMode).value() = "Auto";
   }

   if (callAttributes.replacesDialogId.getCallId().size() > 0)
   {
      invitemsg->header(h_Replaces).value() = callAttributes.replacesDialogId.getCallId();
      if (callAttributes.replacesDialogId.getLocalTag().size() > 0)
      {
         invitemsg->header(h_Replaces).param(p_toTag) = callAttributes.replacesDialogId.getLocalTag();
      }
      if (callAttributes.replacesDialogId.getRemoteTag().size() > 0)
      {
         invitemsg->header(h_Replaces).param(p_fromTag) = callAttributes.replacesDialogId.getRemoteTag();
      }
   }

   if (callAttributes.joinDialogId.getCallId().size() > 0)
   {
      invitemsg->header(h_Join).value() = callAttributes.joinDialogId.getCallId();
      if (callAttributes.joinDialogId.getLocalTag().size() > 0)
      {
         invitemsg->header(h_Join).param(p_toTag) = callAttributes.joinDialogId.getLocalTag();
      }
      if (callAttributes.joinDialogId.getRemoteTag().size() > 0)
      {
         invitemsg->header(h_Join).param(p_fromTag) = callAttributes.joinDialogId.getRemoteTag();
      }
   }

   // we are ICE controlling if we sent the offer that kicks off the offer/answer exchange
   // resulting in the ICE connectivity checks
   mDialogSet.setIceRole(true);

   mDialogSet.sendInvite(invitemsg);

   // Clear any pending hold/unhold requests since our offer/answer here will handle it
   if(mPendingRequest.mType == Hold ||
      mPendingRequest.mType == Unhold)
   {
      mPendingRequest.mType = None;
   }

   // Adjust RTP streams
   adjustRTPStreams(true);

   // Special case of this call - since call in addToConversation will not work, since we didn't know our bridge port at that time
   mConversationManager.getBridgeMixer().calculateMixWeightsForParticipant(this);
}

int
RemoteParticipant::getConnectionPortOnBridge()
{
   if(mDialogSet.getActiveRemoteParticipantHandle() == mHandle)
   {
      return mDialogSet.getConnectionPortOnBridge();
   }
   else
   {
      // If this is not active fork leg, then we don't want to effect the bridge mixer.
      // Note:  All forked endpoints/participants have the same connection port on the bridge
      return -1;
   }
}

int
RemoteParticipant::getMediaConnectionId()
{
   return mDialogSet.getMediaConnectionId();
}

void
RemoteParticipant::destroyParticipant(const resip::Data& appDefinedReason)
{
   try
   {
      if(mState != Terminating)
      {
         stateTransition(Terminating);
         if(mInviteSessionHandle.isValid())
         {
            if (appDefinedReason.empty())
            {
               mInviteSessionHandle->end();
            }
            else
            {
               mInviteSessionHandle->end(appDefinedReason);
            }
         }
         else
         {
            mDialogSet.end();
         }

         // 3261 says we MUST consider the session terminated when we send the BYE,
         // (but of course we only do this if the rtp streams are not shared among
         // multiple dialogs (due to forking))
         if (mDialogSet.getDialogCount() == 1)
         {
            const RemoteParticipantDialogSet::RtpStreamMap& rtpStreams = mDialogSet.getRtpStreams();
            RemoteParticipantDialogSet::RtpStreamMap::const_iterator it = rtpStreams.begin();
            for (; it != rtpStreams.end(); ++it)
            {
               const boost::shared_ptr<RtpStream>& rtpStream = it->second;
               if (rtpStream.get())
               {
                  rtpStream->stopRtpSend();
                  rtpStream->stopRtpReceive();
               }
            }
         }
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteParticipant::destroyParticipant exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteParticipant::destroyParticipant unknown exception");
   }
}

void
RemoteParticipant::addToConversation(Conversation* conversation, unsigned int inputGain, unsigned int outputGain)
{
   Participant::addToConversation(conversation, inputGain, outputGain);

   // Copy over the conversation profile attributes; this is to handle the case where the user called createConversation(..)
   // and specified a ConversationProfile before adding an incoming RemoteParticipant to the Conversation.
   // Note that this will override the default incoming ConversationProfile.
   ConversationProfile* profile = dynamic_cast<ConversationProfile*>(mDialogSet.getUserProfile().get());
   if (profile) // profile will only be set if this is an incoming call
   {
      (*profile) = *(conversation->getProfile());
   }

   if(isHolding() && !conversation->shouldHold())  // If we are on hold and we now shouldn't be, then unhold
   {
      unhold();
   }

   // add streams to the mixer here to handle the case where this participant is added to a new/different
   // conversation
   const RemoteParticipantDialogSet::RtpStreamMap& streams = mDialogSet.getRtpStreams();
   RemoteParticipantDialogSet::RtpStreamMap::const_iterator streamIt = streams.begin();
   for (; streamIt != streams.end(); ++streamIt)
   {
      const boost::shared_ptr<RtpStream>& rtpStream = streamIt->second;
      if (rtpStream.get())
      {
         conversation->getMixer()->addRtpStream(rtpStream, 1); // !jjg! todo: fix gain
         if (m_onRtpStreamClosedConns.count(streamIt->first) == 0)
         {
            m_onRtpStreamClosedConns[streamIt->first] = rtpStream->onClosed().connect(boost::bind(&RemoteParticipant::onRtpStreamClosed, this, streamIt->first, _1, _2));
         }
      }
   }
}

void
RemoteParticipant::removeFromConversation(Conversation* conversation, bool bTriggerHold )
{
   const RemoteParticipantDialogSet::RtpStreamMap& streams = mDialogSet.getRtpStreams();
   RemoteParticipantDialogSet::RtpStreamMap::const_iterator iter = streams.begin();
   while( iter != streams.end() )
   {
      conversation->getMixer()->removeRtpStream( iter->second );
      ++iter;
   }

   Participant::removeFromConversation(conversation, bTriggerHold);

   if( bTriggerHold )
      checkHoldCondition();
}

void
RemoteParticipant::checkHoldCondition()
{
   // Return to Offer a hold sdp if we are not in any conversations, or all the conversations we are in have conditions such that a hold is required
   bool shouldHold = true;
   ConversationMap::iterator it;
   for(it = mConversations.begin(); it != mConversations.end(); ++it)
   {
      if(!it->second->shouldHold())
      {
         shouldHold = false;
         break;
      }
   }
   if(isHolding() != shouldHold)
   {
      if(shouldHold)
      {
         hold();
      }
      else
      {
         unhold();
      }
   }
}

void 
RemoteParticipant::updateMedia(ConversationManager::MediaAttributes mediaAttributes, bool sendOffer)
{
   // !jjg! todo -- do some validation to prevent someone setting 'none' for a media type
   // that is already enabled
   mDialogSet.audioDirection() = mediaAttributes.audioDirection;
   mDialogSet.videoDirection() = mediaAttributes.videoDirection;

   ConversationProfile* profile = dynamic_cast<ConversationProfile*>(mDialogSet.getUserProfile().get());

   if (profile)
   {
      // set security-related params
      if(mediaAttributes.secureMediaMode == ConversationManager::SrtpDtls &&
         profile->natTraversalMode() == flowmanager::MediaStream::TurnAllocation)
      {
         WarningLog(<< "You cannot use SrtpDtls and a Turn allocation at the same time - disabling SrtpDtls!");
         mDialogSet.setSecureMediaMode(ConversationManager::NoSecureMedia);
      }
      else
      {
         mDialogSet.setSecureMediaMode(mediaAttributes.secureMediaMode);
      }
   }

   mDialogSet.setSecureMediaRequired(mediaAttributes.secureMediaRequired);
   mDialogSet.setSrtpCryptoSuite(mediaAttributes.secureMediaDefaultCryptoSuite);

   // !jjg! fixme - should be handling media in general, not just audio and video
   if (mDialogSet.audioEnabled() && mMediaHoldStates.count(SdpMediaLine::MEDIA_TYPE_AUDIO) == 0)
   {
      mMediaHoldStates[SdpMediaLine::MEDIA_TYPE_AUDIO] = false;
   }
   if (mDialogSet.videoEnabled() && mMediaHoldStates.count(SdpMediaLine::MEDIA_TYPE_VIDEO) == 0)
   {
      mMediaHoldStates[SdpMediaLine::MEDIA_TYPE_VIDEO] = false;
   }

   if (sendOffer && mPendingRequest.mType == None && mPendingOffer.get() == 0)
   {
      // !jjg! should set mPendingRequest.mType = ReOffer
      provideOffer(false);
   }
}

void
RemoteParticipant::stateTransition(State state)
{
   Data stateName;

   switch(state)
   {
   case Connecting:
      stateName = "Connecting"; break;
   case Connected:
      stateName = "Connected"; break;
   case Redirecting:
      stateName = "Redirecting"; break;
   case Holding:
      stateName = "Holding"; break;
   case Unholding:
      stateName = "Unholding"; break;
   case Replacing:
      stateName = "Replacing"; break;
   case PendingOODRefer:
      stateName = "PendingOODRefer"; break;
   case Terminating:
      stateName = "Terminating"; break;
   default:
      stateName = "Unknown: " + Data(state); break;
   }
   InfoLog( << "RemoteParticipant::stateTransition of handle=" << mHandle << " to state=" << stateName );
   mState = state;

   if(mState == Connected && mPendingRequest.mType != None)
   {
      PendingRequestType type = mPendingRequest.mType;
      mPendingRequest.mType = None;
      switch(type)
      {
      case Hold:
         hold();
         break;
      case Unhold:
         unhold();
         break;
      case Redirect:
         redirect(mPendingRequest.mDestination);
         break;
      case RedirectTo:
         redirectToParticipant(mPendingRequest.mDestInviteSessionHandle);
         break;
      case None:
         break;
      }
   }
}

ConversationManager::MediaDirection
getDirectionForMediaType(const resip::Data& mediaType, const resip::SdpContents& sdp)
{
   bool oldStyleHoldRequested = isEqualNoCase(sdp.session().connection().getAddress(), "0.0.0.0");

   std::list<resip::SdpContents::Session::Medium>::const_iterator it = sdp.session().media().begin();
   for (; it != sdp.session().media().end(); ++it)
   {
      if (resip::isEqualNoCase(it->name(), mediaType))
      {
         if (it->exists("inactive"))
            return ConversationManager::MediaDirection_Inactive;
         else if (it->exists("sendonly") || oldStyleHoldRequested)
            return ConversationManager::MediaDirection_SendOnly;
         else if (it->exists("recvonly"))
            return ConversationManager::MediaDirection_ReceiveOnly;
         else
            return ConversationManager::MediaDirection_SendReceive;
      }
   }
   return ConversationManager::MediaDirection_None;
}

void
RemoteParticipant::accept(ConversationManager::MediaAttributes mediaAttributes)
{
   try
   {
      // Accept SIP call if required
      if(mState == Connecting && mInviteSessionHandle.isValid())
      {
         ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mInviteSessionHandle.get());
         if(sis && !sis->isAccepted())
         {
            // Clear any pending hold/unhold requests since our offer/answer here will handle it
            if(mPendingRequest.mType == Hold ||
               mPendingRequest.mType == Unhold)
            {
               mPendingRequest.mType = None;
            }

            if(mOfferRequired)
            {
               mDialogSet.setIceRole(true);
               mDialogSet.audioDirection() = mediaAttributes.audioDirection;
               mDialogSet.videoDirection() = mediaAttributes.videoDirection;
               provideOffer(true /* postOfferAccept */);
            }
            else if(mPendingOffer.get() != 0)
            {
               // do some validation on what the user provided in mediaAttributes, since
               // it would be illegal to add an m= line in an answer, and confusing to have 
               // it be added on subsequent offers e.g. for hold/unhold
               ConversationManager::MediaDirection remoteAudioDirection = getDirectionForMediaType("audio",*mPendingOffer.get());
               ConversationManager::MediaDirection remoteVideoDirection = getDirectionForMediaType("video",*mPendingOffer.get());

               if (remoteAudioDirection == ConversationManager::MediaDirection_None)
               {
                  mDialogSet.audioDirection() = remoteAudioDirection;
               }
               else
               {
                  mDialogSet.audioDirection() = mediaAttributes.audioDirection;
               }

               if (remoteVideoDirection == ConversationManager::MediaDirection_None)
               {
                  mDialogSet.videoDirection() = remoteVideoDirection;
               }
               else
               {
                  mDialogSet.videoDirection() = mediaAttributes.videoDirection;
               }

               ConversationProfile* profile = dynamic_cast<ConversationProfile*>(mDialogSet.getUserProfile().get());

               if (profile)
               {
                  // set security-related params
                  if(mediaAttributes.secureMediaMode == ConversationManager::SrtpDtls &&
                     profile->natTraversalMode() == flowmanager::MediaStream::TurnAllocation)
                  {
                     WarningLog(<< "You cannot use SrtpDtls and a Turn allocation at the same time - disabling SrtpDtls!");
                     mDialogSet.setSecureMediaMode(ConversationManager::NoSecureMedia);
                  }
                  else
                  {
                     mDialogSet.setSecureMediaMode(mediaAttributes.secureMediaMode);
                  }

                  mDialogSet.setSecureMediaRequired(mediaAttributes.secureMediaRequired);
                  mDialogSet.setSrtpCryptoSuite(mediaAttributes.secureMediaDefaultCryptoSuite);
               }

               mDialogSet.setIceRole(false);
               provideAnswer(*mPendingOffer.get(), true /* postAnswerAccept */, false /* postAnswerAlert */);
               mPendingOffer.release();
            }
            else
            {
               // It is possible to get here if the app calls alert with early true.  There is special logic in
               // RemoteParticipantDialogSet::accept to handle the case then an alert call followed immediately by
               // accept.  In this case the answer from the alert will be queued waiting on the flow to be ready, and
               // we need to ensure the accept call is also delayed until the answer completes.
               mDialogSet.accept(mInviteSessionHandle);
            }
         }
      }
      // Accept Pending OOD Refer if required
      else if(mState == PendingOODRefer)
      {
         acceptPendingOODRefer(mediaAttributes);
      }
      else if (mState == Connected || mState == Holding || mState == Unholding || mState == Replacing)
      {
         if (mPendingOffer.get() != 0)
         {
            // do some validation on what the user provided in mediaAttributes, since
            // it would be illegal to add an m= line in an answer, and confusing to have 
            // it be added on subsequent offers e.g. for hold/unhold
            ConversationManager::MediaDirection remoteAudioDirection = getDirectionForMediaType("audio",*mPendingOffer.get());
            ConversationManager::MediaDirection remoteVideoDirection = getDirectionForMediaType("video",*mPendingOffer.get());

            if (remoteAudioDirection == ConversationManager::MediaDirection_None)
            {
               mDialogSet.audioDirection() = remoteAudioDirection;
            }
            else
            {
               mDialogSet.audioDirection() = mediaAttributes.audioDirection;
            }

            if (remoteVideoDirection == ConversationManager::MediaDirection_None)
            {
               mDialogSet.videoDirection() = remoteVideoDirection;
            }
            else
            {
               mDialogSet.videoDirection() = mediaAttributes.videoDirection;
            }

            ConversationProfile* profile = dynamic_cast<ConversationProfile*>(mDialogSet.getUserProfile().get());

            if (profile)
            {
               // set security-related params
               if(mediaAttributes.secureMediaMode == ConversationManager::SrtpDtls &&
                  profile->natTraversalMode() == flowmanager::MediaStream::TurnAllocation)
               {
                  WarningLog(<< "You cannot use SrtpDtls and a Turn allocation at the same time - disabling SrtpDtls!");
                  mDialogSet.setSecureMediaMode(ConversationManager::NoSecureMedia);
               }
               else
               {
                  mDialogSet.setSecureMediaMode(mediaAttributes.secureMediaMode);
               }

               mDialogSet.setSecureMediaRequired(mediaAttributes.secureMediaRequired);
               mDialogSet.setSrtpCryptoSuite(mediaAttributes.secureMediaDefaultCryptoSuite);
            }

            mDialogSet.setIceRole(false);
            if (provideAnswer(*mPendingOffer.get(), mState==Replacing /* postAnswerAccept */, false /* postAnswerAlert */))
            {
               if(mState == Replacing)
               {
                  stateTransition(Connecting);
               }
               mPendingOffer.release();
            }
         }
      }
      else
      {
         WarningLog(<< "RemoteParticipant::accept called in invalid state: " << mState);
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteParticipant::accept exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteParticipant::accept unknown exception");
   }
}

void
RemoteParticipant::alert(bool earlyFlag)
{
   try
   {
      if(mState == Connecting && mInviteSessionHandle.isValid())
      {
         ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mInviteSessionHandle.get());
         if(sis && !sis->isAccepted())
         {
            if(earlyFlag && mPendingOffer.get() != 0)
            {
               provideAnswer(*mPendingOffer.get(), false /* postAnswerAccept */, true /* postAnswerAlert */);
               mPendingOffer.release();
            }
            else
            {
               sis->provisional(180, earlyFlag);
            }
         }
      }
      else
      {
         WarningLog(<< "RemoteParticipant::alert called in invalid state: " << mState);
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteParticipant::alert exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteParticipant::alert unknown exception");
   }
}

void
RemoteParticipant::reject(unsigned int rejectCode)
{
   try
   {
      // Reject UAS Invite Session if required
      if(mState == Connecting && mInviteSessionHandle.isValid())
      {
         ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mInviteSessionHandle.get());
         if(sis && !sis->isAccepted())
         {
            InfoLog( <<"Reject with code: " << rejectCode);
            sis->reject(rejectCode);
            mPendingOffer.release();
         }
      }
      // Reject Pending OOD Refer request if required
      else if(mState == PendingOODRefer)
      {
         rejectPendingOODRefer(rejectCode);
      }
      else if(mState == Connected)
      {
         // reject a re-INVITE
         InfoLog( <<"Reject with code: " << rejectCode);
         mInviteSessionHandle->reject(rejectCode);
         mPendingOffer.release();
      }
      else
      {
         WarningLog(<< "RemoteParticipant::reject called in invalid state: " << mState);
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteParticipant::reject exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteParticipant::reject unknown exception");
   }
}

void
RemoteParticipant::redirect(NameAddr& destination)
{
   try
   {
      if(mPendingRequest.mType == None)
      {
         if((mState == Connecting || mState == Connected) && mInviteSessionHandle.isValid())
         {
            ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mInviteSessionHandle.get());
            // If this is a UAS session and we haven't sent a final response yet - then redirect via 302 response
            if(sis && !sis->isAccepted())
            {
               NameAddrs destinations;
               destinations.push_back(destination);
               mConversationManager.onParticipantRedirectSuccess(mHandle);
               sis->redirect(destinations);
            }
            else if(mInviteSessionHandle->isConnected()) // redirect via blind transfer
            {
               mInviteSessionHandle->refer(destination, true /* refersub */);
               stateTransition(Redirecting);
            }
            else
            {
               mPendingRequest.mType = Redirect;
               mPendingRequest.mDestination = destination;
            }
         }
         else
         {
            mPendingRequest.mType = Redirect;
            mPendingRequest.mDestination = destination;
         }
      }
      else
      {
         WarningLog(<< "RemoteParticipant::redirect error: request pending");
         mConversationManager.onParticipantRedirectFailure(mHandle, 406 /* Not Acceptable */);
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteParticipant::redirect exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteParticipant::redirect unknown exception");
   }
}

void
RemoteParticipant::redirectToParticipant(InviteSessionHandle& destParticipantInviteSessionHandle)
{
   try
   {
      if(destParticipantInviteSessionHandle.isValid())
      {
         if(mPendingRequest.mType == None)
         {
            if((mState == Connecting || mState == Connected) && mInviteSessionHandle.isValid())
            {
               ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mInviteSessionHandle.get());
               // If this is a UAS session and we haven't sent a final response yet - then redirect via 302 response
               if(sis && !sis->isAccepted())
               {
                  NameAddrs destinations;
                  destinations.push_back(destParticipantInviteSessionHandle->peerAddr());
                  mConversationManager.onParticipantRedirectSuccess(mHandle);
                  sis->redirect(destinations);
               }
               else if(mInviteSessionHandle->isConnected()) // redirect via attended transfer (with replaces)
               {
                  NameAddr referTo(destParticipantInviteSessionHandle->peerAddr());
                  referTo.remove(p_tag);
                  mInviteSessionHandle->refer(referTo, destParticipantInviteSessionHandle /* session to replace)  */, true /* refersub */);
                  stateTransition(Redirecting);
               }
               else
               {
                  mPendingRequest.mType = RedirectTo;
                  mPendingRequest.mDestInviteSessionHandle = destParticipantInviteSessionHandle;
               }
            }
            else
            {
               mPendingRequest.mType = RedirectTo;
               mPendingRequest.mDestInviteSessionHandle = destParticipantInviteSessionHandle;
            }
         }
         else
         {
            WarningLog(<< "RemoteParticipant::redirectToParticipant error: request pending");
            mConversationManager.onParticipantRedirectFailure(mHandle, 406 /* Not Acceptable */);
         }
      }
      else
      {
         WarningLog(<< "RemoteParticipant::redirectToParticipant error: destParticipant has no valid InviteSession");
         mConversationManager.onParticipantRedirectFailure(mHandle, 406 /* Not Acceptable */);
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteParticipant::redirectToParticipant exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteParticipant::redirectToParticipant unknown exception");
   }
}

void
RemoteParticipant::hold()
{
   InfoLog(<< "RemoteParticipant::hold request: handle=" << mHandle);

   const RemoteParticipantDialogSet::RtpStreamMap& streams = mDialogSet.getRtpStreams();
   RemoteParticipantDialogSet::RtpStreamMap::const_iterator it = streams.begin();
   for (; it != streams.end(); ++it)
   {
      if (mDialogSet.isMediaActive(it->first))
      {
         // .jjg. only set the hold state to true if this media line is currently 'active',
         // since we don't want to enable or change the directionality when we unhold if it is disabled/inactive
         mMediaHoldStates[it->first] = true;
      }
   }

   try
   {
      if(mPendingRequest.mType == None)
      {
         if(mState == Connected && mInviteSessionHandle.isValid())
         {
            provideOffer(false /* postOfferAccept */);
            stateTransition(Holding);
         }
         else
         {
            mPendingRequest.mType = Hold;
         }
      }
      else if(mPendingRequest.mType == Unhold)
      {
         mPendingRequest.mType = None;  // Unhold pending, so move to do nothing
         return;
      }
      else if(mPendingRequest.mType == Hold)
      {
         return;  // Hold already pending
      }
      else
      {
         WarningLog(<< "RemoteParticipant::hold error: request already pending");
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteParticipant::hold exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteParticipant::hold unknown exception");
   }
}

void
RemoteParticipant::unhold()
{
   InfoLog(<< "RemoteParticipant::unhold request: handle=" << mHandle);

   if (mDialogSet.audioEnabled() && mMediaHoldStates[SdpMediaLine::MEDIA_TYPE_AUDIO])
   {
      mDialogSet.audioDirection() = ConversationManager::MediaDirection_SendReceive;
   }
   if (mDialogSet.videoEnabled() && mMediaHoldStates[SdpMediaLine::MEDIA_TYPE_VIDEO])
   {
      mDialogSet.videoDirection() = ConversationManager::MediaDirection_SendReceive;
   }

   MediaHoldStateMap::iterator it = mMediaHoldStates.begin();
   for (; it != mMediaHoldStates.end(); ++it)
   {
      it->second = false;
   }

   try
   {
      if(mPendingRequest.mType == None)
      {
         if(mState == Connected && mInviteSessionHandle.isValid())
         {
            provideOffer(false /* postOfferAccept */);
            stateTransition(Unholding);
         }
         else
         {
            mPendingRequest.mType = Unhold;
         }
      }
      else if(mPendingRequest.mType == Hold)
      {
         mPendingRequest.mType = None;  // Hold pending, so move do nothing
         return;
      }
      else if(mPendingRequest.mType == Unhold)
      {
         return;  // Unhold already pending
      }
      else
      {
         WarningLog(<< "RemoteParticipant::unhold error: request already pending");
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteParticipant::unhold exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteParticipant::unhold unknown exception");
   }
}

void
RemoteParticipant::setPendingOODReferInfo(ServerOutOfDialogReqHandle ood, const SipMessage& referMsg)
{
   stateTransition(PendingOODRefer);
   mPendingOODReferMsg = referMsg;
   mPendingOODReferNoSubHandle = ood;
}

void
RemoteParticipant::setPendingOODReferInfo(ServerSubscriptionHandle ss, const SipMessage& referMsg)
{
   stateTransition(PendingOODRefer);
   mPendingOODReferMsg = referMsg;
   mPendingOODReferSubHandle = ss;
}

// !jjg! is this potentially harmful?  19.1.5 in 3261 hints that you aren't supposed
// to send the request AT ALL if you don't understand some of the extensions that are used ...
// however there are platforms (sipX) that expect the client to pass along these unknown headers
void
RemoteParticipant::mergeUnknownHeaders(const resip::Uri& uriSource, resip::SharedPtr<resip::SipMessage>& msgDest)
{
   std::auto_ptr<resip::SipMessage> embeddedMsg(new resip::SipMessage(uriSource.embedded()));
   SipMessage::UnknownHeaders userHeaders = embeddedMsg->getRawUnknownHeaders();
   SipMessage::UnknownHeaders::const_iterator itUserHdrs = userHeaders.begin();
   for (; itUserHdrs != userHeaders.end(); ++itUserHdrs)
   {
      ExtensionHeader h_Ext(itUserHdrs->first);
      msgDest->header(h_Ext) = embeddedMsg->header(h_Ext);
   }
}

void
RemoteParticipant::acceptPendingOODRefer(ConversationManager::MediaAttributes mediaAttribs)
{
   if(mState == PendingOODRefer)
   {
      // Create offer
      SdpContents offer;

      resip::SharedPtr<ConversationProfile> convProfile;
      ConversationMap::const_iterator convIt = mConversations.begin();
      for (; convIt != mConversations.end(); ++convIt)
      {
         if (convIt->second)
         {
            if (convIt->second->getNumLocalParticipants() > 0)
            {
               convProfile = convIt->second->getProfile();
            }
         }
      }

      mDialogSet.audioDirection() = mediaAttribs.audioDirection;
      mDialogSet.videoDirection() = mediaAttribs.videoDirection;

      // set security-related params
      if(mediaAttribs.secureMediaMode == ConversationManager::SrtpDtls &&
         convProfile->natTraversalMode() == flowmanager::MediaStream::TurnAllocation)
      {
         WarningLog(<< "You cannot use SrtpDtls and a Turn allocation at the same time - disabling SrtpDtls!");
         mDialogSet.setSecureMediaMode(ConversationManager::NoSecureMedia);
      }
      else
      {
         mDialogSet.setSecureMediaMode(mediaAttribs.secureMediaMode);
      }

      mDialogSet.setSecureMediaRequired(mediaAttribs.secureMediaRequired);
      mDialogSet.setSrtpCryptoSuite(mediaAttribs.secureMediaDefaultCryptoSuite);

      buildSdpOffer(convProfile.get(), mMediaHoldStates, offer);

      // Build the Invite
      //SharedPtr<SipMessage> invitemsg = mDum.makeInviteSessionFromRefer(mPendingOODReferMsg,
      //                                                                  mDialogSet.getUserProfile(),
      //                                                                  &offer,
      //                                                                  &mDialogSet);
      SharedPtr<SipMessage> invitemsg = mDum.makeInviteSessionFromRefer(mPendingOODReferMsg,
                                                                        mPendingOODReferSubHandle,
                                                                        &offer,
                                                                        &mDialogSet);
      mergeUnknownHeaders(mPendingOODReferMsg.header(h_ReferTo).uri(), invitemsg);

      mDialogSet.setIceRole(true);
      mDialogSet.sendInvite(invitemsg);

      adjustRTPStreams(true);

      stateTransition(Connecting);
   }
}

void
RemoteParticipant::rejectPendingOODRefer(unsigned int statusCode)
{
   if(mState == PendingOODRefer)
   {
      mConversationManager.onParticipantTerminated(mHandle, statusCode, InviteSessionHandler::Rejected, NULL);
      delete this;
   }
}

void
RemoteParticipant::processReferNotify(const SipMessage& notify)
{
   unsigned int code = 0;  // a sipfrag might not be present if we get a neutral NOTIFY due to a requestRefresh
   resip::Data reasonPhrase(resip::Data::Empty);

   SipFrag* frag  = dynamic_cast<SipFrag*>(notify.getContents());
   if (frag)
   {
      // Get StatusCode from SipFrag
      if (frag->message().isResponse())
      {
         code = frag->message().header(h_StatusLine).statusCode();
         reasonPhrase = frag->message().header(h_StatusLine).reason();
      }
   }

   // Check if success or failure response code was in SipFrag
   if(code == 0)
   {
      // ignore
   }
   else if(code < 200)
   {
      if(mState == Redirecting)
      {
         if (mHandle) mConversationManager.onParticipantRedirectProgress(mHandle, &notify);
      }
   }
   else if(code >= 200 && code < 300)
   {
      if(mState == Redirecting)
      {
         if (mHandle) mConversationManager.onParticipantRedirectSuccess(mHandle, &notify);
         stateTransition(Connected);
      }
   }
   else if(code >= 300)
   {
      if(mState == Redirecting)
      {
         if (mHandle) mConversationManager.onParticipantRedirectFailure(mHandle, code, reasonPhrase, &notify);
         stateTransition(Connected);
      }
   }
}

void
RemoteParticipant::provideOffer(bool postOfferAccept)
{
   std::auto_ptr<SdpContents> offer(new SdpContents);
   assert(mInviteSessionHandle.isValid());

   ConversationProfile* convProfile = dynamic_cast<ConversationProfile*>(mDialogSet.getUserProfile().get());
   buildSdpOffer(convProfile, mMediaHoldStates, *offer);

   // we are ICE controlling if we sent the offer that kicks off the offer/answer exchange
   // resulting in the ICE connectivity checks
   mDialogSet.setIceRole(true);

   adjustRTPStreams(true);
   mDialogSet.provideOffer(offer, mInviteSessionHandle, postOfferAccept);
   mOfferRequired = false;
}

bool
RemoteParticipant::provideAnswer(const SdpContents& offer, bool postAnswerAccept, bool postAnswerAlert)
{
   auto_ptr<SdpContents> answer(new SdpContents);
   assert(mInviteSessionHandle.isValid());
   bool answerOk = buildSdpAnswer(offer, *answer);

   if(answerOk)
   {
      adjustRTPStreams();
      mDialogSet.provideAnswer(answer, mInviteSessionHandle, postAnswerAccept, postAnswerAlert);
   }
   else
   {
      std::auto_ptr<resip::WarningCategory> pWarning(new resip::WarningCategory());
      pWarning->code() = 305;
      pWarning->hostname() = "devnull";
      pWarning->text() = "SDP: Incompatible media format: no common codec";
      mInviteSessionHandle->reject(488, pWarning.get());
   }

   return answerOk;
}

sdpcontainer::SdpMediaLine::SdpMediaType
mediaTypeFromName(const resip::Data& name)
{
   if (resip::isEqualNoCase(name, "audio"))
      return sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO;
   if (resip::isEqualNoCase(name, "video"))
      return sdpcontainer::SdpMediaLine::MEDIA_TYPE_VIDEO;
   return sdpcontainer::SdpMediaLine::MEDIA_TYPE_UNKNOWN;
}

void
RemoteParticipant::buildSdpOffer(ConversationProfile* profile, MediaHoldStateMap holdStates, SdpContents& offer, std::set<sdpcontainer::SdpMediaLine::SdpMediaType> existingMediaTypes)
{
   SdpContents::Session::Medium *offerMedium = 0;

   if(!profile) // This can happen for UAC calls
   {
      profile = mConversationManager.getDefaultOutgoingConversationProfile().get();
   }

   // If we already have a local sdp for this session, then use this to form
   // the next offer - doing so will ensure that we do not switch codecs or
   // payload id's mid session.
   if(mInviteSessionHandle.isValid() && mInviteSessionHandle->getLocalSdp().session().media().size() != 0)
   {
      offer = mInviteSessionHandle->getLocalSdp();

      // Trim anything out of the offer that shouldn't be there (specifically, m= lines)
      std::list<SdpContents::Session::Medium>::iterator offerIt = offer.session().media().begin();
      for (; offerIt !=  offer.session().media().end() ; ++offerIt )
      {
         SdpMediaLine::SdpMediaType mt_itm = SdpMediaLine::getMediaTypeFromString(offerIt->name().c_str());
         if (mDialogSet.isMediaEnabled(mt_itm))
         {
            // always re-fetch the port, since it's possible that this media stream
            // was disabled (closing the port) but will now be re-enabled in our offer
            offerIt->port() = getLocalRTPPort(mt_itm, (offerIt->getConnections().front().getAddressType() == SdpContents::IP6), profile);
         }
         else
         {
            offerIt->port() = 0;
         }
      }

      // Set sessionid and version for this sdp
      if (offer.session().origin().getSessionId() == 0)
      {
         mLocalSdpSessionId = Timer::getTimeMicroSec(); //UInt64 currentTime 
         offer.session().origin().getSessionId() = mLocalSdpSessionId;
      }
      offer.session().origin().getVersion() = mLocalSdpVersion++;

      if (profile->useRfc2543Hold() && resip::isEqualNoCase(offer.session().connection().getAddress(),"0.0.0.0"))
      {
         // restore the actual IP address
         offer.session().connection().setAddress(mDialogSet.connectionAddress());
      }

      // Add any codecs from our capabilities that may not be in current
      // local sdp - since endpoint may have changed and may now be capable
      // of handling codecs that it previously could not (common when
      // endpoint is a B2BUA).

      SdpContents& sessionCaps = profile->sessionCaps();
      int highPayloadId = 96;  // Note:  static payload id's are in range of 0-96

      for ( std::list<SdpContents::Session::Medium>::iterator mediaIt2 = sessionCaps.session().media().begin() ;
            mediaIt2 != sessionCaps.session().media().end() ; ++mediaIt2 )
      {
         SdpContents::Session::Medium *scapsMedium = &(*mediaIt2);

         if (isEqualNoCase("audio", scapsMedium->name()) && !mDialogSet.audioActive())
         {
            continue;
         }
         else if (isEqualNoCase("video", scapsMedium->name()) && !mDialogSet.videoActive())
         {
            continue;
         }

         bool foundMediumInPreviousOffer = false;

         // Iterate over the media lines in the offer. Compare these with
         // lines from the session capabilities.
         for (std::list<SdpContents::Session::Medium>::iterator mediaIt = offer.session().media().begin();
              mediaIt != offer.session().media().end(); ++mediaIt)
         {
            offerMedium = &(*mediaIt);

            // Compare only equivalent media
            if ( isEqualNoCase( offerMedium->name(), scapsMedium->name() ))
            {
               foundMediumInPreviousOffer = true;

               // Iterate through codecs in session caps and check if
               // already in offer
               for (std::list<SdpContents::Session::Codec>::iterator codecsIt = scapsMedium->codecs().begin();
                    codecsIt != scapsMedium->codecs().end() ; ++codecsIt)
               {
                  bool found=false;
                  bool payloadIdCollision=false;
                  for (std::list<SdpContents::Session::Codec>::iterator codecsIt2 = offerMedium->codecs().begin();
                       codecsIt2 != offerMedium->codecs().end() ; ++codecsIt2)
                  {
                     if(isEqualNoCase(codecsIt->getName(), codecsIt2->getName()) &&
                        codecsIt->getRate() == codecsIt2->getRate())
                     {
                        found = true;
                     }
                     else if(codecsIt->payloadType() == codecsIt2->payloadType())
                     {
                        payloadIdCollision = true;
                     }

                     // Keep track of highest payload id in offer - used if we need
                     // to resolve a payload id conflict
                     if(codecsIt2->payloadType() > highPayloadId)
                     {
                        highPayloadId = codecsIt2->payloadType();
                     }
                  }
                  if(!found)
                  {
                     if(payloadIdCollision)
                     {
                        highPayloadId++;
                        codecsIt->payloadType() = highPayloadId;
                     }
                     else if(codecsIt->payloadType() > highPayloadId)
                     {
                        highPayloadId = codecsIt->payloadType();
                     }
                     offerMedium->addCodec(*codecsIt);
                  }
               }
            }
         }

         if (!foundMediumInPreviousOffer)
         {
            // add the medium
            SdpContents defaultOffer;

            // Build base offer
            mConversationManager.buildSdpOffer(profile, defaultOffer);

            offerMedium = NULL;
            std::auto_ptr<SdpContents::Session::Medium> addedMedium;

            // Make sure the local port is set properly for all media types
            // in the base offer.
            for( std::list<SdpContents::Session::Medium>::iterator mediaIt = defaultOffer.session().media().begin();
                 mediaIt != defaultOffer.session().media().end(); ++mediaIt )
            {
               if (isEqualNoCase(mediaIt->name(), scapsMedium->name()))
               {
                  bool mediaConnIsV6 = (mediaIt->getConnections().size() > 0 ? mediaIt->getConnections().front().getAddressType() == SdpContents::IP6 : false);
                  addedMedium.reset(new SdpContents::Session::Medium(*mediaIt));
                  addedMedium->port() = getLocalRTPPort(SdpMediaLine::getMediaTypeFromString(addedMedium->name().c_str()), mediaConnIsV6, profile);
               }
            }

            if (addedMedium.get())
            {
               offer.session().addMedium(*addedMedium);
            }
         }
      }
   }
   else
   {
      // Build base offer
      mConversationManager.buildSdpOffer(profile, offer);
      mLocalSdpSessionId = offer.session().origin().getSessionId();
      mLocalSdpVersion = offer.session().origin().getVersion() + 1;

      // Make sure the local port is set properly for all media types
      // in the base offer.
      std::list<SdpContents::Session::Medium>& offerMedia = offer.session().media();
      std::list<SdpContents::Session::Medium>::iterator mediaIt = offerMedia.begin();
      //for( ; mediaIt != offerMedia.end(); mediaIt++ )
      while (mediaIt != offerMedia.end())
      {
         offerMedium = &(*mediaIt);
         if (existingMediaTypes.size() > 0)
         {
            // there are other RemoteParticipants in this Conversation,
            // and we need to ensure that we are offering the same media types
            if (resip::isEqualNoCase(offerMedium->name(), "audio") && (existingMediaTypes.find(SdpMediaLine::MEDIA_TYPE_AUDIO) == existingMediaTypes.end()))
            {
               mediaIt = offer.session().media().erase(mediaIt);
               continue;
            }
            else if (resip::isEqualNoCase(offerMedium->name(), "video") && (existingMediaTypes.find(SdpMediaLine::MEDIA_TYPE_VIDEO) == existingMediaTypes.end()))
            {
               mediaIt = offer.session().media().erase(mediaIt);
               continue;
            }
         }
         if (resip::isEqualNoCase(offerMedium->name(), "audio") && !mDialogSet.audioActive())
         {
            mediaIt = offer.session().media().erase(mediaIt);
            continue;
         }
         else if (resip::isEqualNoCase(offerMedium->name(), "video") && !mDialogSet.videoActive())
         {
            mediaIt = offer.session().media().erase(mediaIt);
            continue;
         }
         offerMedium = &(*mediaIt);
         offerMedium->port() = getLocalRTPPort(SdpMediaLine::getMediaTypeFromString(offerMedium->name().c_str()), offerMedium->getConnections().front().getAddressType() == SdpContents::IP6, profile);
         mediaIt++;
      }
   }

   // only used if useRfc2543Hold is true
   mDialogSet.connectionAddress() = offer.session().connection().getAddress();

   // Perform SRTP and DTLS madness
   for( std::list<SdpContents::Session::Medium>::iterator mediaIt = offer.session().media().begin();
      mediaIt != offer.session().media().end(); ++mediaIt )
   {
      offerMedium = &(*mediaIt);

      // Add Crypto attributes (if required)
      offerMedium->clearAttribute("crypto");
      offerMedium->clearAttribute("encryption");
      offerMedium->clearAttribute("tcap");
      offerMedium->clearAttribute("pcfg");
      offer.session().clearAttribute("fingerprint");
      offer.session().clearAttribute("setup");
      if(mDialogSet.getSecureMediaMode() == ConversationManager::Srtp)
      {
         // Note:  We could add the crypto attribute to the "SDP Capabilties Negotiation"
         //        potential configuration if secure media is not required - but other implementations
         //        should ignore them any way if just plain RTP is used.  It is thought the
         //        current implementation will increase interopability. (ie. SNOM Phones)

         Data crypto;

         switch(mDialogSet.getSrtpCryptoSuite())
         {
         case flowmanager::MediaStream::SRTP_AES_CM_128_HMAC_SHA1_32:
            crypto = "1 AES_CM_128_HMAC_SHA1_32 inline:" + mDialogSet.getLocalSrtpSessionKey().base64encode();
            offerMedium->addAttribute("crypto", crypto);
            crypto = "2 AES_CM_128_HMAC_SHA1_80 inline:" + mDialogSet.getLocalSrtpSessionKey().base64encode();
            offerMedium->addAttribute("crypto", crypto);
            break;
         default:
            crypto = "1 AES_CM_128_HMAC_SHA1_80 inline:" + mDialogSet.getLocalSrtpSessionKey().base64encode();
            offerMedium->addAttribute("crypto", crypto);
            crypto = "2 AES_CM_128_HMAC_SHA1_32 inline:" + mDialogSet.getLocalSrtpSessionKey().base64encode();
            offerMedium->addAttribute("crypto", crypto);
            break;
         }
         if(mDialogSet.getSecureMediaRequired())
         {
            offerMedium->protocol() = Symbols::RTP_SAVP;
            mDialogSet.setSrtpEnabled(mediaTypeFromName(offerMedium->name()), true);
         }
         else
         {
            offerMedium->protocol() = Symbols::RTP_AVP;
            offerMedium->addAttribute("encryption", "optional");  // Used by SNOM phones?
            offerMedium->addAttribute("tcap", "1 RTP/SAVP");      // draft-ietf-mmusic-sdp-capability-negotiation-08
            offerMedium->addAttribute("pcfg", "1 t=1");
         }
      }
      else if(mDialogSet.getSecureMediaMode() == ConversationManager::SrtpDtls)
      {
#ifdef USE_DTLS
         if(mConversationManager.getFlowManager().getDtlsFactory())
         {
            // Note:  We could add the fingerprint and setup attributes to the "SDP Capabilties Negotiation"
            //        potential configuration if secure media is not required - but other implementations
            //        should ignore them any way if just plain RTP is used.  It is thought the
            //        current implementation will increase interopability.

            // Add fingerprint attribute
            char fingerprint[100];
            mConversationManager.getFlowManager().getDtlsFactory()->getMyCertFingerprint(fingerprint);
            offer.session().addAttribute("fingerprint", "SHA-1 " + Data(fingerprint));
            //offer.session().addAttribute("acap", "1 fingerprint:SHA-1 " + Data(fingerprint));

            // Add setup attribute
            offer.session().addAttribute("setup", "actpass");

            if(mDialogSet.getSecureMediaRequired())
            {
               offerMedium->protocol() = Symbols::UDP_TLS_RTP_SAVP;
            }
            else
            {
               offerMedium->protocol() = Symbols::RTP_AVP;
               offerMedium->addAttribute("tcap", "1 UDP/TLS/RTP/SAVP");      // draft-ietf-mmusic-sdp-capability-negotiation-08
               offerMedium->addAttribute("pcfg", "1 t=1");
               //offerMedium->addAttribute("pcfg", "1 t=1 a=1");
            }
         }
#endif // USE_DTLS
      }

      bool mediumActive = false;
      if (resip::isEqualNoCase(offerMedium->name(),"audio"))
      {
         mediumActive = mDialogSet.audioActive();
      }
      else if (resip::isEqualNoCase(offerMedium->name(),"video"))
      {
         mediumActive = mDialogSet.videoActive();
      }

      bool holdSdp = false;
      MediaHoldStateMap::const_iterator holdstateIt = holdStates.find(SdpMediaLine::getMediaTypeFromString(offerMedium->name().c_str()));
      if (holdstateIt != holdStates.end())
      {
         holdSdp = holdstateIt->second;
      }

      offerMedium->clearAttribute("sendrecv");
      offerMedium->clearAttribute("sendonly");
      offerMedium->clearAttribute("recvonly");
      offerMedium->clearAttribute("inactive");

      if( mediumActive && holdSdp )
      {
         // The medium is being held
         offerMedium->addAttribute( "sendonly" );
         if( profile->useRfc2543Hold() )
            offer.session().connection().setAddress("0.0.0.0");
      }
      else
      {
         // The medium is either not being held, or it is inactive
         ConversationManager::MediaDirection mediaDir = ConversationManager::MediaDirection_None;
         if( resip::isEqualNoCase( offerMedium->name(), "audio" ))
            mediaDir = mDialogSet.audioDirection();
         else if( resip::isEqualNoCase( offerMedium->name(), "video"))
            mediaDir = mDialogSet.videoDirection();

         switch( mediaDir )
      {
         case ConversationManager::MediaDirection_SendReceive:
            offerMedium->addAttribute( "sendrecv" );
            break;
         case ConversationManager::MediaDirection_SendOnly:
            offerMedium->addAttribute( "sendonly" );
            break;
         case ConversationManager::MediaDirection_ReceiveOnly:
            offerMedium->addAttribute( "recvonly" );
            break;
         case ConversationManager::MediaDirection_Inactive:
            offerMedium->addAttribute( "inactive" );
            break;
         case ConversationManager::MediaDirection_None:
         default:
            break;
         }
      }
   }

   mConversationManager.mCodecFactory->releaseLicenses(mLicensedCodecs);

   std::list<SdpContents::Session::Medium>::iterator mediaIt = offer.session().media().begin();
   for (; mediaIt != offer.session().media().end(); ++mediaIt)
   {
      SdpContents::Session::Medium& m = *mediaIt;

      if (m.port() != 0) // only need to acquire licenses if this medium is enabled
      {
         // increments usage count, and filters out codecs for which the usage count exceeds the max avail
         mConversationManager.mCodecFactory->acquireLicenses(m.codecs());
         mLicensedCodecs.insert(mLicensedCodecs.begin(), m.codecs().begin(), m.codecs().end());
      }
   }

   setProposedSdp(offer);
}

bool
RemoteParticipant::answerMediaLine(SdpContents::Session::Medium& mediaSessionCaps, const SdpMediaLine& sdpMediaLine, SdpContents& answer, bool potential, ConversationProfile* profile )
{
   resip::Data protocol = sdpMediaLine.getTransportProtocolTypeString();
   protocol = protocol.uppercase();
   bool valid = false;

   // If this is a valid medium then process it
   if( protocol.find( "RTP", 0 ) == 0 &&
      sdpMediaLine.getConnections().size() != 0 &&
      sdpMediaLine.getConnections().front().getPort() != 0)
   {
      SdpContents::Session::Medium medium(sdpMediaLine.getMediaTypeString(), getLocalRTPPort(sdpMediaLine.getMediaType(), sdpMediaLine.getConnections().front().getAddressType() == Sdp::ADDRESS_TYPE_IP6, profile), 1, protocol);
      medium.encodeAttribsForStaticPLs() = false;

      // Check secure media properties and requirements
      bool secureMediaRequired = mDialogSet.getSecureMediaRequired() || protocol == resip::Data("RTP/SAVP");

      if(mDialogSet.getSecureMediaMode() == ConversationManager::Srtp ||
         isEqualNoCase( protocol, "RTP/SAVP"))  // allow accepting of SAVP profiles, even if SRTP is not enabled as a SecureMedia mode
      {
         mDialogSet.setSrtpEnabled(sdpMediaLine.getMediaType(), true);
         bool supportedCryptoSuite = false;
         SdpMediaLine::CryptoList::const_iterator itCrypto = sdpMediaLine.getCryptos().begin();
         for(; !supportedCryptoSuite && itCrypto!=sdpMediaLine.getCryptos().end(); ++itCrypto)
         {
            Data cryptoKeyB64(itCrypto->getCryptoKeyParams().front().getKeyValue());
            Data cryptoKey = cryptoKeyB64.base64decode();

            if(cryptoKey.size() == SRTP_MASTER_KEY_LEN)
            {
               switch(itCrypto->getSuite())
               {
               case SdpMediaLine::CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_80:
                  medium.addAttribute("crypto", Data(itCrypto->getTag()) + " AES_CM_128_HMAC_SHA1_80 inline:" + mDialogSet.getLocalSrtpSessionKey().base64encode());
                  supportedCryptoSuite = true;
                  mDialogSet.setSecureMediaMode(ConversationManager::Srtp);
                  mDialogSet.setSrtpCryptoSuite(flowmanager::MediaStream::SRTP_AES_CM_128_HMAC_SHA1_80);
                  break;
               case SdpMediaLine::CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_32:
                  medium.addAttribute("crypto", Data(itCrypto->getTag()) + " AES_CM_128_HMAC_SHA1_32 inline:" + mDialogSet.getLocalSrtpSessionKey().base64encode());
                  supportedCryptoSuite = true;
                  mDialogSet.setSecureMediaMode(ConversationManager::Srtp);
                  mDialogSet.setSrtpCryptoSuite(flowmanager::MediaStream::SRTP_AES_CM_128_HMAC_SHA1_32);
                  break;
               default:
                  break;
               }
            }
            else
            {
               InfoLog(<< "SDES crypto key found in SDP, but is not of correct length after base 64 decode: " << cryptoKey.size());
            }
         }
         if(!supportedCryptoSuite && secureMediaRequired)
         {
            InfoLog(<< "Secure media stream is required, but there is no supported crypto attributes in the offer - skipping this stream...");
            return false;
         }
      }
#ifdef USE_DTLS
      else if(mConversationManager.getFlowManager().getDtlsFactory() &&
              (mDialogSet.getSecureMediaMode() == ConversationManager::SrtpDtls ||
               protocolType == SdpMediaLine::PROTOCOL_TYPE_UDP_TLS_RTP_SAVP))  // allow accepting of DTLS SAVP profiles, even if DTLS-SRTP is not enabled as a SecureMedia mode
      {
         bool supportedFingerprint = false;

         // We will only process Dtls-Srtp if fingerprint is in SHA-1 format
         if(sdpMediaLine.getFingerPrintHashFunction() == SdpMediaLine::FINGERPRINT_HASH_FUNC_SHA_1)
         {
            answer.session().clearAttribute("fingerprint");  // ensure we don't add these twice
            answer.session().clearAttribute("setup");  // ensure we don't add these twice

            // Add fingerprint attribute to answer
            char fingerprint[100];
            mConversationManager.getFlowManager().getDtlsFactory()->getMyCertFingerprint(fingerprint);
            answer.session().addAttribute("fingerprint", "SHA-1 " + Data(fingerprint));

            // Add setup attribute
            if(sdpMediaLine.getTcpSetupAttribute() == SdpMediaLine::TCP_SETUP_ATTRIBUTE_ACTIVE)
            {
               answer.session().addAttribute("setup", "passive");
            }
            else
            {
               answer.session().addAttribute("setup", "active");
            }

            supportedFingerprint = true;
         }
         if(!supportedFingerprint && secureMediaRequired)
         {
            InfoLog(<< "Secure media stream is required, but there is no supported fingerprint attributes in the offer - skipping this stream...");
            return false;
         }
      }
#endif // USE_DTLS

      if(potential && !sdpMediaLine.getPotentialMediaViewString().empty())
      {
         medium.addAttribute("acfg", sdpMediaLine.getPotentialMediaViewString());
      }

      // Iterate through codecs and look for supported codecs - tag found ones by storing their payload id
      SdpMediaLine::CodecList::const_iterator itCodec = sdpMediaLine.getCodecs().begin();
      SdpMediaLine::CodecList mediaSessionCapsCodecs;
      CodecConverter::toSdpCodec(mediaSessionCaps.codecs(), mediaSessionCaps.name(), mediaSessionCapsCodecs);
      for(; itCodec != sdpMediaLine.getCodecs().end(); ++itCodec)
      {
         std::auto_ptr<SdpCodec> pMatch(mConversationManager.mCodecFactory->getBestMatchingCodec(mediaSessionCapsCodecs, *itCodec));

         //const resip::Data& offerFMTP = itCodec->getFormatParameters();
         //bool modeInOffer = offerFMTP.prefix("mode=");

         //// Loop through allowed codec list and see if codec is supported locally
         //for (std::list<SdpContents::Session::Codec>::iterator capsCodecsIt = mediaSessionCaps.codecs().begin();
         //     capsCodecsIt != mediaSessionCaps.codecs().end(); capsCodecsIt++)
         //{
         //   // If the mime sub-types do not match, skip it
         //   if (!isEqualNoCase(capsCodecsIt->getName(), itCodec->getMimeSubtype()))
         //      continue;

         //   // If the rates do not match, skip it
         //   if ((unsigned int)capsCodecsIt->getRate() != itCodec->getRate())
         //      continue;

         //   // Check the Format parameters
         //   const resip::Data& capsFMTP = capsCodecsIt->parameters();

         //   // If one of them has FMTP, and the other does not, this is
         //   // considered to NOT be a match
         //   if (( capsFMTP.size() == 0 && offerFMTP.size() != 0 ) ||
         //       ( capsFMTP.size() != 0 && offerFMTP.size() == 0 ))
         //       continue;

         //   // If the offer contains an FMTP, check that it is valid.
         //   if ( offerFMTP.size() != 0 )
         //   {
         //      if ( !mConversationManager.mCodecFactory->isFMTPValid(
         //               itCodec->getMimeType(), capsCodecsIt->payloadType(),
         //               offerFMTP, itCodec->getRate() ))
         //         continue;
         //   }

         //   // Proceed with checking the "mode"
         //   bool modeInCaps = capsFMTP.prefix("mode=");

         //   if(!modeInOffer && !modeInCaps)
         //   {
         //      // If mode is not specified in either - then we have a match
         //      bestCapsCodecMatchIt = capsCodecsIt;
         //      break;
         //   }
         //   else if(modeInOffer && modeInCaps)
         //   {
         //      if(isEqualNoCase(capsCodecsIt->parameters(), itCodec->getFormatParameters()))
         //      {
         //         bestCapsCodecMatchIt = capsCodecsIt;
         //         break;
         //      }
         //      // If mode is specified in both, and doesn't match - then we have no match
         //   }
         //   else
         //   {
         //      // Mode is specified on either offer or caps - this match is a potential candidate
         //      // As a rule - use first match of this kind only
         //      if(bestCapsCodecMatchIt == mediaSessionCaps.codecs().end())
         //      {
         //         bestCapsCodecMatchIt = capsCodecsIt;
         //      }
         //   }
         //}

         //if(bestCapsCodecMatchIt != mediaSessionCaps.codecs().end())
         if (pMatch.get())
         {
            std::list<SdpContents::Session::Codec>::iterator capsCodecsIt = mediaSessionCaps.codecs().begin();
            for (;capsCodecsIt != mediaSessionCaps.codecs().end(); ++capsCodecsIt)
            {
               if (pMatch->getPayloadType() == capsCodecsIt->payloadType())
               {
                  break;
               }
            }

            SdpContents::Session::Codec codec(*capsCodecsIt);
            codec.payloadType() = itCodec->getPayloadType();  // honour offered payload id - just to be nice  :)
            medium.addCodec(codec);
            if(!valid && !isEqualNoCase(capsCodecsIt->getName(), "telephone-event"))
            {
               // Consider offer valid if we see any matching codec other than telephone-event
               valid = true;
            }
         }
      }

      if(valid)
      {
         // copy ptime attribute from session caps (if exists)
         if(mediaSessionCaps.exists("ptime"))
         {
            medium.addAttribute("ptime", mediaSessionCaps.getValues("ptime").front());
         }

         // Check requested direction
         unsigned int remoteRtpPort = sdpMediaLine.getConnections().front().getPort();
         bool oldStyleHoldRequested = resip::isEqualNoCase(sdpMediaLine.getConnections().front().getAddress(), "0.0.0.0");
         bool mediaHoldState = mMediaHoldStates[sdpMediaLine.getMediaType()];
         if(sdpMediaLine.getMediaType() == SdpMediaLine::MEDIA_TYPE_AUDIO && !mDialogSet.audioActive() ||
            sdpMediaLine.getMediaType() == SdpMediaLine::MEDIA_TYPE_VIDEO && !mDialogSet.videoActive() ||
            sdpMediaLine.getDirection() == SdpMediaLine::DIRECTION_TYPE_INACTIVE ||
            (oldStyleHoldRequested && mediaHoldState) ||
            (mediaHoldState && (sdpMediaLine.getDirection() == SdpMediaLine::DIRECTION_TYPE_SENDONLY || remoteRtpPort == 0)))  // If remote inactive or both sides are holding
         {
            medium.addAttribute("inactive");
         }
         else if(sdpMediaLine.getDirection() == SdpMediaLine::DIRECTION_TYPE_SENDONLY || (oldStyleHoldRequested && !mediaHoldState) || remoteRtpPort == 0 /* old RFC 2543 hold */)
         {
            medium.addAttribute("recvonly");
         }
         else if(sdpMediaLine.getDirection() == SdpMediaLine::DIRECTION_TYPE_RECVONLY || mediaHoldState)
         {
            medium.addAttribute("sendonly");
         }
         else
         {
            // Note:  sendrecv is the default in SDP
            medium.addAttribute("sendrecv");
         }
         answer.session().addMedium(medium);
      }
   }
   return valid;
}

bool
RemoteParticipant::buildSdpAnswer(const SdpContents& offer, SdpContents& answer)
{
   // Note: this implementation has minimal support for draft-ietf-mmusic-sdp-capabilities-negotiation
   //       for responding "best-effort" / optional SRTP (Dtls-SRTP) offers

   bool valid = false;
   Sdp* remoteSdp = SdpHelperResip::createSdpFromResipSdp(offer);

   try
   {
      // Get the conversation profile
      ConversationProfile *profile = dynamic_cast<ConversationProfile*>(mDialogSet.getUserProfile().get());

      // get session capabilities
      SdpContents sessionCaps = profile->sessionCaps();

      // Initialize answer from session caps
      answer = sessionCaps;

      // Set sessionid and version for this answer
      //UInt64 currentTime = Timer::getTimeMicroSec();
      //answer.session().origin().getSessionId() = currentTime;
      if (mLocalSdpSessionId == 0)
      {
         mLocalSdpSessionId = Timer::getTimeMicroSec(); //UInt64 currentTime 
      }
      answer.session().origin().getSessionId() = mLocalSdpSessionId;
      answer.session().origin().getVersion() = mLocalSdpVersion++;

      // Copy t= field from sdp (RFC3264)
      if(offer.session().getTimes().size() >= 1)
      {
         answer.session().getTimes().clear();
         answer.session().addTime(offer.session().getTimes().front());
      }

      // Clear out m= lines in answer then populate below
      answer.session().media().clear();

      // .jjg. for now, assume one stream of each media type
      std::set<SdpMediaLine::SdpMediaType> processedMediaTypes;

      // Loop through each offered m= line and provide a response
      Sdp::MediaLineList::const_iterator itMediaLine;
      std::list<SdpContents::Session::Medium>::const_iterator itOldMediaLine;

      if (offer.session().media().size() == 0)
      {
         // RFC 3725 flow IV
         valid = true;
      }

      for( itMediaLine = remoteSdp->getMediaLines().begin(), itOldMediaLine = offer.session().media().begin() ;
           itMediaLine != remoteSdp->getMediaLines().end() ;
           ++itMediaLine, ++itOldMediaLine )
      {
         bool matchFound = false;

         if (processedMediaTypes.find((*itMediaLine)->getMediaType()) == processedMediaTypes.end())
         {
            processedMediaTypes.insert((*itMediaLine)->getMediaType());

            if ((*itMediaLine)->getMediaType() == SdpMediaLine::MEDIA_TYPE_AUDIO && !profile->audioSupported())
            {
               InfoLog(<< "buildSdpAnswer - audio not supported");
               continue;
            }
            if ((*itMediaLine)->getMediaType() == SdpMediaLine::MEDIA_TYPE_VIDEO && !profile->videoSupported())
            {
               InfoLog(<< "buildSdpAnswer - video not supported");
               continue;
            }

            // Check the capabilities for matches
            for( std::list<SdpContents::Session::Medium>::iterator iter = sessionCaps.session().media().begin() ;
                 !matchFound && ( iter != sessionCaps.session().media().end()) ; ++iter )
            {
               // The configuration may override the remote m line, check it first
               SdpMediaLine::SdpMediaLineList::const_iterator itPotentialMediaLine = (*itMediaLine)->getPotentialMediaViews().begin();
               for(; !matchFound && ( itPotentialMediaLine != (*itMediaLine)->getPotentialMediaViews().end() ) ; ++itPotentialMediaLine)
               {
                  matchFound = answerMediaLine(*iter, *itPotentialMediaLine, answer, true, profile);
                  if ( matchFound )
                  {
                     // We have a valid potential media - line - copy over
                     // normal media line to make further processing easier
                     *(*itMediaLine) = *itPotentialMediaLine;
                     break;
                  }
               }

               // If nothing was found, proceed as normal
               if ( !matchFound )
                  matchFound = answerMediaLine(*iter, *(*itMediaLine), answer, false, profile);
            }
         }

         if ( !matchFound )
         {
            // If no matching media was found, reject this offer
            SdpContents::Session::Medium rejmedium(itOldMediaLine->name(), 0, 1, (*itMediaLine)->getTransportProtocolTypeString());

            std::list<Data>::const_iterator oldFormats = itOldMediaLine->getFormats().begin();
            for( ; oldFormats != itOldMediaLine->getFormats().end() ; ++oldFormats )
               rejmedium.addFormat(*oldFormats);

            rejmedium.mRtpMapDone = true;
            answer.session().addMedium(rejmedium);
            InfoLog(<< "rejecting medium " << rejmedium.name());
         }
         else
         {
            // If any media was matched, the entire sdp is valid
            valid = true;
         }
      }
   }
   catch(BaseException &e)
   {
      WarningLog( << "buildSdpAnswer: exception parsing SDP offer: " << e.getMessage());
      valid = false;
   }
   catch(...)
   {
      WarningLog( << "buildSdpAnswer: unknown exception parsing SDP offer");
      valid = false;
   }

   //InfoLog( << "SDPOffer: " << offer);
   //InfoLog( << "SDPAnswer: " << answer);
   if(valid)
   {
      mConversationManager.mCodecFactory->releaseLicenses(mLicensedCodecs);
      std::list<SdpContents::Session::Medium>::iterator mediaIt = answer.session().media().begin();
      for (; mediaIt != answer.session().media().end(); ++mediaIt)
      {
         if (mediaIt->port() != 0) // only need to acquire licenses if this medium is enabled
         {
            // increments usage count, and filters out codecs for which the usage count exceeds the max avail
            mConversationManager.mCodecFactory->acquireLicenses(mediaIt->codecs());
            mLicensedCodecs.insert(mLicensedCodecs.begin(), mediaIt->codecs().begin(), mediaIt->codecs().end());

            if (mLicensedCodecs.size() == 1)
            {
               if (resip::isEqualNoCase(mLicensedCodecs.front().getName(), "telephone-event"))
               {
                  valid = false;
                  delete remoteSdp;
                  return valid;
               }
            }
         }
      }

      setLocalSdp(answer);
      setRemoteSdp(offer, remoteSdp);
   }
   else
   {
      delete remoteSdp;
   }
   return valid;
}

#ifdef OLD_CODE
// Note:  This old code used to serve 2 purposes -
// 1 - that we do not change payload id's mid session
// 2 - that we do not add codecs or media streams that have previously rejected
// Purpose 2 is not correct.  RFC3264 states we need purpose 1, but you are allowed to add new codecs mid-session
//
// Decision to comment out this code and implement purpose 1 elsewhere - leaving this code here for reference (for now)
// as it may be useful for something in the future.
bool
RemoteParticipant::formMidDialogSdpOfferOrAnswer(const SdpContents& localSdp, const SdpContents& remoteSdp, SdpContents& newSdp, bool offer)
{
   bool valid = false;

   try
   {
      // start with current localSdp
      newSdp = localSdp;

      // Clear all m= lines are rebuild
      newSdp.session().media().clear();

      // Set sessionid and version for this sdp
      UInt64 currentTime = Timer::getTimeMicroSec();
      newSdp.session().origin().getSessionId() = currentTime;
      newSdp.session().origin().getVersion() = mLocalSdpVersion++;

      // Loop through each m= line in local Sdp and remove or disable if not in remote
      for (std::list<SdpContents::Session::Medium>::const_iterator localMediaIt = localSdp.session().media().begin();
           localMediaIt != localSdp.session().media().end(); localMediaIt++)
      {
         for (std::list<SdpContents::Session::Medium>::const_iterator remoteMediaIt = remoteSdp.session().media().begin();
              remoteMediaIt != remoteSdp.session().media().end(); remoteMediaIt++)
         {
            if(localMediaIt->name() == remoteMediaIt->name() && localMediaIt->protocol() == remoteMediaIt->protocol())
            {
               // Found an m= line match, proceed to process codecs
               SdpContents::Session::Medium medium(localMediaIt->name(), localMediaIt->port(), localMediaIt->multicast(), localMediaIt->protocol());

               // Iterate through local codecs and look for remote supported codecs
               for (std::list<SdpContents::Session::Codec>::const_iterator localCodecsIt = localMediaIt->codecs().begin();
                    localCodecsIt != localMediaIt->codecs().end(); localCodecsIt++)
               {
                  // Loop through remote supported codec list and see if codec is supported
                  for (std::list<SdpContents::Session::Codec>::const_iterator remoteCodecsIt = remoteMediaIt->codecs().begin();
                       remoteCodecsIt != remoteMediaIt->codecs().end(); remoteCodecsIt++)
                  {
                     if(isEqualNoCase(localCodecsIt->getName(), remoteCodecsIt->getName()) &&
                        localCodecsIt->getRate() == remoteCodecsIt->getRate())
                     {
                        // matching supported codec found - add to newSdp
                        medium.addCodec(*localCodecsIt);
                        if(!valid && !isEqualNoCase(localCodecsIt->getName(), "telephone-event"))
                        {
                           // Consider valid if we see any matching codec other than telephone-event
                           valid = true;
                        }
                        break;
                     }
                  }
               }

               // copy ptime attribute from session caps (if exists)
               if(localMediaIt->exists("ptime"))
               {
                  medium.addAttribute("ptime", localMediaIt->getValues("ptime").front());
               }

               if(offer)
               {
                  if(mLocalHold)
                  {
                     if(remoteMediaIt->exists("inactive") ||
                        remoteMediaIt->exists("sendonly") ||
                        remoteMediaIt->port() == 0)  // If remote inactive or both sides are holding
                     {
                        medium.addAttribute("inactive");
                     }
                     else
                     {
                        medium.addAttribute("sendonly");
                     }
                  }
                  else
                  {
                     if(remoteMediaIt->exists("inactive") || remoteMediaIt->exists("sendonly") || remoteMediaIt->port() == 0 /* old RFC 2543 hold */)
                     {
                        medium.addAttribute("recvonly");
                     }
                     else
                     {
                        medium.addAttribute("sendrecv");
                     }
                  }
               }
               else  // This is an sdp answer
               {
                  // Check requested direction
                  if(remoteMediaIt->exists("inactive") ||
                     (mLocalHold && (remoteMediaIt->exists("sendonly") || remoteMediaIt->port() == 0)))  // If remote inactive or both sides are holding
                  {
                     medium.addAttribute("inactive");
                  }
                  else if(remoteMediaIt->exists("sendonly") || remoteMediaIt->port() == 0 /* old RFC 2543 hold */)
                  {
                     medium.addAttribute("recvonly");
                  }
                  else if(remoteMediaIt->exists("recvonly") || mLocalHold)
                  {
                     medium.addAttribute("sendonly");
                  }
                  else
                  {
                     // Note:  sendrecv is the default in SDP
                     medium.addAttribute("sendrecv");
                  }
               }

               newSdp.session().addMedium(medium);
               break;
            }
         }
      }
   }
   catch(BaseException &e)
   {
      WarningLog( << "formMidDialogSdpOfferOrAnswer: exception: " << e.getMessage());
      valid = false;
   }
   catch(...)
   {
      WarningLog( << "formMidDialogSdpOfferOrAnswer: unknown exception");
      valid = false;
   }

   return valid;
}
#endif

void
RemoteParticipant::destroyConversations()
{
   ConversationMap temp = mConversations;  // Copy since we may end up being destroyed
   ConversationMap::iterator it;
   for(it = temp.begin(); it != temp.end(); it++)
   {
      it->second->destroy();
   }
}

void
RemoteParticipant::setProposedSdp(const resip::SdpContents& sdp)
{
   mDialogSet.setProposedSdp(mHandle, sdp);
}

void
RemoteParticipant::setLocalSdp(const resip::SdpContents& sdp)
{
   if(mLocalSdp) delete mLocalSdp;
   mLocalSdp = 0;
   InfoLog(<< "setLocalSdp: handle=" << mHandle << ", localSdp=" << sdp);
   mLocalSdp = SdpHelperResip::createSdpFromResipSdp(sdp);
}

void
RemoteParticipant::setRemoteSdp(const resip::SdpContents& sdp, bool answer)
{
   if(mRemoteSdp) delete mRemoteSdp;
   mRemoteSdp = 0;
   InfoLog(<< "setRemoteSdp: handle=" << mHandle << ", remoteSdp=" << sdp);
   mRemoteSdp = SdpHelperResip::createSdpFromResipSdp(sdp);
   if(answer && mDialogSet.getProposedSdp())
   {
      if(mLocalSdp) delete mLocalSdp;
      mLocalSdp = new sdpcontainer::Sdp(*mDialogSet.getProposedSdp());  // copied
   }
}

void
RemoteParticipant::setRemoteSdp(const resip::SdpContents& sdp, Sdp* remoteSdp) // Note: sdp only passed for logging purposes
{
   if(mRemoteSdp) delete mRemoteSdp;
   InfoLog(<< "setRemoteSdp: handle=" << mHandle << ", remoteSdp=" << sdp);
   mRemoteSdp = remoteSdp;
}

ConversationManager::MediaDirection
toMediaState(sdpcontainer::SdpMediaLine::SdpDirectionType direction)
{
   switch (direction)
   {
   case SdpMediaLine::DIRECTION_TYPE_INACTIVE:
      return ConversationManager::MediaDirection_Inactive;
   case SdpMediaLine::DIRECTION_TYPE_NONE:
      return ConversationManager::MediaDirection_None;
   case SdpMediaLine::DIRECTION_TYPE_RECVONLY:
      return ConversationManager::MediaDirection_ReceiveOnly;
   case SdpMediaLine::DIRECTION_TYPE_SENDONLY:
      return ConversationManager::MediaDirection_SendOnly;
   case SdpMediaLine::DIRECTION_TYPE_SENDRECV:
      return ConversationManager::MediaDirection_SendReceive;
   }
   return ConversationManager::MediaDirection_None;
}

void 
RemoteParticipant::checkRemoteIceAttribs(const resip::SdpContents& remoteSdp)
{
   if (remoteSdp.session().exists("ice-ufrag"))
   {
      const resip::Data& latestUFrag = remoteSdp.session().getValues("ice-ufrag").front();
      if (latestUFrag != mRemoteIceUFrag)
      {
         if (!mRemoteIceUFrag.empty())
         {
            mDialogSet.resetIceAttribs();
         }
         mRemoteIceUFrag = latestUFrag;
      }
   }
}

void
RemoteParticipant::adjustRTPStreams(bool sendingOffer, const resip::SipMessage* msg)
{
   // these are the 'base' used in the c= and m= lines
   Data remoteIPAddress;

   Sdp *localSdp = sendingOffer ? mDialogSet.getProposedSdp() : mLocalSdp;
   Sdp *remoteSdp = sendingOffer ? 0 : mRemoteSdp;
   const SdpMediaLine::CodecList* localCodecs = NULL;
   const SdpMediaLine::CodecList* remoteCodecs = NULL;
   assert(localSdp);

   // Iterate over all the supported medias. For now, voice and video.
   SdpMediaLine::SdpMediaType supportedMedias[] = { SdpMediaLine::MEDIA_TYPE_AUDIO, SdpMediaLine::MEDIA_TYPE_VIDEO };

   int mediaDirection[sizeof( supportedMedias ) / sizeof( SdpMediaLine::SdpMediaType )];
   mediaDirection[0] = SdpMediaLine::DIRECTION_TYPE_NONE;
   mediaDirection[1] = SdpMediaLine::DIRECTION_TYPE_NONE;

   int remoteMediaDirection[sizeof( supportedMedias ) / sizeof( SdpMediaLine::SdpMediaType )];
   remoteMediaDirection[0] = SdpMediaLine::DIRECTION_TYPE_NONE;
   remoteMediaDirection[1] = SdpMediaLine::DIRECTION_TYPE_NONE;

   int localMediaDirection[sizeof( supportedMedias ) / sizeof( SdpMediaLine::SdpMediaType )];
   localMediaDirection[0] = SdpMediaLine::DIRECTION_TYPE_NONE;
   localMediaDirection[1] = SdpMediaLine::DIRECTION_TYPE_NONE;

   for( int i = 0 ; i < ( sizeof( supportedMedias ) / sizeof( SdpMediaLine::SdpMediaType )) ; ++i )
   {
      bool bOldStyleHoldRequested = false;
      bool supportedCryptoSuite = false;
      bool supportedFingerprint = false;

      bool mlineSupported = false;

      unsigned int remoteRtpPort=0;
      unsigned int remoteRtcpPort=0;

      // these are ICE candidates, in order by priority
      SdpMediaLine::SdpCandidateList candidates;
      resip::Data iceUsername;
      resip::Data icePassword;

      Sdp::MediaLineList::const_iterator itMediaLine = localSdp->getMediaLines().begin();
      for(; itMediaLine != localSdp->getMediaLines().end(); ++itMediaLine)
      {
         resip::Data proto = (*itMediaLine)->getTransportProtocolTypeString();
         proto = proto.uppercase();

         DebugLog(<< "adjustRTPStreams: handle=" << mHandle << ", found media line in local sdp, mediaType=" << (*itMediaLine)->getMediaType() <<
            ", transportType=" << (*itMediaLine)->getTransportProtocolTypeString() << ", numConnections=" << (*itMediaLine)->getConnections().size() <<
            ", port=" << ((*itMediaLine)->getConnections().size() > 0 ? (*itMediaLine)->getConnections().front().getPort() : 0));
         if((*itMediaLine)->getMediaType() == supportedMedias[ i ] &&
            ( isEqualNoCase( proto, "RTP/AVP")  ||
              isEqualNoCase( proto, "RTP/SAVP") ||
              isEqualNoCase( proto, "UDP/TLS/RTP/SAVP")) &&
            (*itMediaLine)->getConnections().size() != 0 &&
            (*itMediaLine)->getConnections().front().getPort() != 0)
         {
            //InfoLog(<< "adjustRTPStreams: handle=" << mHandle << ", found media line in local sdp");
            localMediaDirection[i] = (*itMediaLine)->getDirection();
            localCodecs = &(*itMediaLine)->getCodecs();
            break;
         }
      }

      if(remoteSdp)
      {
         remoteMediaDirection[i] = SdpMediaLine::DIRECTION_TYPE_NONE;
         Sdp::MediaLineList::const_iterator itRemMediaLine = remoteSdp->getMediaLines().begin();
         for(; itRemMediaLine != remoteSdp->getMediaLines().end(); ++itRemMediaLine)
         {
            resip::Data proto = (*itRemMediaLine)->getTransportProtocolTypeString();
            proto = proto.uppercase();

            //InfoLog(<< "adjustRTPStreams: handle=" << mHandle << ", found media line in remote sdp");
            if((*itRemMediaLine)->getMediaType() == supportedMedias[ i ] &&
               ( isEqualNoCase( proto, "RTP/AVP")  ||
                 isEqualNoCase( proto, "RTP/SAVP") ||
                 isEqualNoCase( proto, "UDP/TLS/RTP/SAVP")) &&
               (*itRemMediaLine)->getConnections().size() != 0 &&
               (*itRemMediaLine)->getConnections().front().getPort() != 0)
            {
               //InfoLog(<< "adjustRTPStreams: handle=" << mHandle << ", found media line in remote sdp");
               mlineSupported = true;
               remoteMediaDirection[i] = (*itRemMediaLine)->getDirection();
               remoteRtpPort = (*itRemMediaLine)->getConnections().front().getPort();
               if ((*itRemMediaLine)->getRtcpConnections().size() > 0)
               {
                  remoteRtcpPort = (*itRemMediaLine)->getRtcpConnections().front().getPort();
               }
               remoteIPAddress = (*itRemMediaLine)->getConnections().front().getAddress();
               remoteCodecs = &(*itRemMediaLine)->getCodecs();
               candidates = (*itRemMediaLine)->getCandidates();
               {
                  resip::DataStream dsIceUsername(iceUsername);
                  dsIceUsername << (*itRemMediaLine)->getIceUserFrag();
                  dsIceUsername << ":";
                  dsIceUsername << mDialogSet.getIceUFrag();
               }
               icePassword = (*itRemMediaLine)->getIcePassword();

               // Check if they are using "old-style" hold.
               if( !remoteIPAddress.empty() && isEqualNoCase( remoteIPAddress, "0.0.0.0" ))
               {
                  bOldStyleHoldRequested = true;
                  remoteMediaDirection[i] = SdpMediaLine::DIRECTION_TYPE_SENDONLY;
               }
               
               // If so, we need to try to fetch a better remote IP address.
               // Be careful, as it still might not exist if the first INVITE
               // contained the 0.0.0.0 address.
               if( bOldStyleHoldRequested && !mLastRemoteIPAddr.empty())
                  remoteIPAddress = mLastRemoteIPAddr;

               // Process Crypto settings (if required) - createSRTPSession using remote key
               // Note:  Top crypto in remote sdp will always be the correct suite/key
               if(mDialogSet.getSecureMediaMode() == ConversationManager::Srtp ||
                  isEqualNoCase( proto, "RTP/SAVP"))
               {
                  SdpMediaLine::CryptoList::const_iterator itCrypto = (*itRemMediaLine)->getCryptos().begin();
                  for(; itCrypto != (*itRemMediaLine)->getCryptos().end(); itCrypto++)
                  {
                     Data cryptoKeyB64(itCrypto->getCryptoKeyParams().front().getKeyValue());
                     Data cryptoKey = cryptoKeyB64.base64decode();

                     if(cryptoKey.size() == SRTP_MASTER_KEY_LEN)
                     {
                        switch(itCrypto->getSuite())
                        {
                        case SdpMediaLine::CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_80:
                           mDialogSet.createSRTPSession(supportedMedias[i], flowmanager::MediaStream::SRTP_AES_CM_128_HMAC_SHA1_80, cryptoKey.data(), cryptoKey.size());
                           supportedCryptoSuite = true;
                           break;
                        case SdpMediaLine::CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_32:
                           mDialogSet.createSRTPSession(supportedMedias[i], flowmanager::MediaStream::SRTP_AES_CM_128_HMAC_SHA1_32, cryptoKey.data(), cryptoKey.size());
                           supportedCryptoSuite = true;
                           break;
                        default:
                           break;
                        }
                     }
                     else
                     {
                        InfoLog(<< "SDES crypto key found in SDP, but is not of correct length after base 64 decode: " << cryptoKey.size());
                     }
                     if(supportedCryptoSuite)
                     {
                        break;
                     }
                  }
               }
               // Process Fingerprint and setup settings (if required)
               else if( isEqualNoCase( proto, "UDP/TLS/RTP/SAVP"))
               {
                  // We will only process Dtls-Srtp if fingerprint is in SHA-1 format
                  if((*itRemMediaLine)->getFingerPrintHashFunction() == SdpMediaLine::FINGERPRINT_HASH_FUNC_SHA_1)
                  {
                     if(!(*itRemMediaLine)->getFingerPrint().empty())
                     {
                        InfoLog(<< "Fingerprint retrieved from remote SDP: " << (*itRemMediaLine)->getFingerPrint());
#ifdef USE_DTLS
                        // ensure we only accept media streams with this fingerprint
                        mDialogSet.setRemoteSDPFingerprint((*itRemMediaLine)->getFingerPrint());

                        // If remote setup value is not active then we must be the Dtls client  - ensure client DtlsSocket is create
                        if((*itRemMediaLine)->getTcpSetupAttribute() != SdpMediaLine::TCP_SETUP_ATTRIBUTE_ACTIVE)
                        {
                           // If we are the active end, then kick start the DTLS handshake
                           mDialogSet.startDtlsClient(remoteIPAddress.c_str(), remoteRtpPort, remoteRtcpPort);
                        }

                        supportedFingerprint = true;
#else
                     ErrLog(<< "DTLS support not compiled in; define USE_DTLS");
#endif // USE_DTLS
                     }
                  }
                  else if((*itRemMediaLine)->getFingerPrintHashFunction() != SdpMediaLine::FINGERPRINT_HASH_FUNC_NONE)
                  {
                     InfoLog(<< "Fingerprint found, but is not using SHA-1 hash.");
                  }
               }

               break;
            }
         }

         // Aggregate local and remote direction attributes to determine overall media direction
         if(localMediaDirection[i] == SdpMediaLine::DIRECTION_TYPE_NONE ||
            remoteMediaDirection[i] == SdpMediaLine::DIRECTION_TYPE_NONE)
         {
            mediaDirection[i] = SdpMediaLine::DIRECTION_TYPE_NONE;
         }
         else if(localMediaDirection[i] == SdpMediaLine::DIRECTION_TYPE_INACTIVE ||
                 remoteMediaDirection[i] == SdpMediaLine::DIRECTION_TYPE_INACTIVE)
         {
            mediaDirection[i] = SdpMediaLine::DIRECTION_TYPE_INACTIVE;
         }
         else if(localMediaDirection[i] == SdpMediaLine::DIRECTION_TYPE_SENDONLY)
         {
            mediaDirection[i] = SdpMediaLine::DIRECTION_TYPE_SENDONLY;
         }
         else if(localMediaDirection[i] == SdpMediaLine::DIRECTION_TYPE_RECVONLY)
         {
            mediaDirection[i] = SdpMediaLine::DIRECTION_TYPE_RECVONLY;
         }
         else if(remoteMediaDirection[i] == SdpMediaLine::DIRECTION_TYPE_SENDONLY)
         {
            mediaDirection[i] = SdpMediaLine::DIRECTION_TYPE_RECVONLY;
         }
         else
         {
            mediaDirection[i] = SdpMediaLine::DIRECTION_TYPE_SENDRECV;
         }
      }
      else
      {
         // No remote SDP info - so use local media direction
         mediaDirection[i] = localMediaDirection[i];
      }

      if(remoteSdp && mDialogSet.getSecureMediaRequired() && mlineSupported && !supportedCryptoSuite && !supportedFingerprint)
      {
         InfoLog(<< "Secure media is required and no valid support found in remote sdp - ending call.");
         destroyParticipant("secure media required");
         return;
      }

      InfoLog(<< "adjustRTPStreams: handle=" << mHandle << ", mediaDirection=" << SdpMediaLine::SdpDirectionTypeString[mediaDirection[i]] << ", remoteIp=" << remoteIPAddress << ", remotePort=" << remoteRtpPort);

      if(!remoteIPAddress.empty() && remoteRtpPort != 0)
      {
         mDialogSet.setShortTermCredentials(supportedMedias[i], iceUsername, icePassword);
         mDialogSet.setActiveDestination(supportedMedias[i], remoteIPAddress.c_str(), remoteRtpPort, remoteRtcpPort, candidates);
      }

      // Fetch the rtp stream
      boost::shared_ptr<RtpStream> rtpStream = mDialogSet.getRtpStream(supportedMedias[i]);

      if (rtpStream.get())
      {
         // the RTP streams may have just been created, so add them to the mixer now
         Participant::ConversationMap::iterator convIter = mConversations.begin();
         for (; convIter != mConversations.end(); ++convIter)
         {
            convIter->second->getMixer()->addRtpStream(rtpStream, 1);
         }
         if (m_onRtpStreamClosedConns.count(supportedMedias[i]) == 0)
         {
            m_onRtpStreamClosedConns[supportedMedias[i]] = rtpStream->onClosed().connect(boost::bind(&RemoteParticipant::onRtpStreamClosed, this, supportedMedias[i], _1, _2));
         }
      }

      // .jjg. in the future, this will need to reflect whether or not we are
      // doing music on hold; for now, just assume that pausing the RTP stream is OK
      if(!remoteIPAddress.empty() && remoteRtpPort != 0 &&
         remoteCodecs && localCodecs && rtpStream.get())
      {
         // Calculate intersection of local and remote codecs, and pass remote codecs that exist locally to RTP send fn
         int numCodecs=0;
         SdpMediaLine::CodecList matchedRemoteCodecs;
         SdpMediaLine::CodecList matchedLocalCodecs;
         SdpMediaLine::CodecList::const_iterator itRemoteCodec = remoteCodecs->begin();
         for(; itRemoteCodec != remoteCodecs->end(); ++itRemoteCodec)
         {
            std::auto_ptr<SdpCodec> pMatch(mConversationManager.mCodecFactory->getBestMatchingCodec(*localCodecs, *itRemoteCodec));

            if (pMatch.get())
            {
               numCodecs++;
               sdpcontainer::SdpCodec remoteCodec(itRemoteCodec->getPayloadType(),
                                                    itRemoteCodec->getMimeType().c_str(),
                                                    itRemoteCodec->getMimeSubtype().c_str(),
                                                    itRemoteCodec->getRate(),
                                                    itRemoteCodec->getPacketTime(),
                                                    itRemoteCodec->getNumChannels(),
                                                    itRemoteCodec->getFormatParameters().c_str());
               matchedRemoteCodecs.push_back(remoteCodec);

               sdpcontainer::SdpCodec localCodec(pMatch->getPayloadType(),
                                                   pMatch->getMimeType().c_str(),
                                                   pMatch->getMimeSubtype().c_str(),
                                                   pMatch->getRate(),
                                                   pMatch->getPacketTime(),
                                                   pMatch->getNumChannels(),
                                                   pMatch->getFormatParameters().c_str());
               matchedLocalCodecs.push_back(localCodec);

               Data codecString;
               remoteCodec.toString(codecString);

               InfoLog(<< "adjustRTPStreams: handle=" << mHandle << ", sending to destination address " << remoteIPAddress << ":" <<
                  remoteRtpPort << " (RTCP on " << remoteRtcpPort << "): " << codecString.data());
            }
         }

         if(numCodecs > 0)
         {
            if (mediaDirection[i] == SdpMediaLine::DIRECTION_TYPE_RECVONLY ||
                mediaDirection[i] == SdpMediaLine::DIRECTION_TYPE_INACTIVE ||
                (mediaDirection[i] == SdpMediaLine::DIRECTION_TYPE_SENDONLY && mConversations.size() == 0))
            {
               // .jjg. NOTE that the media stack is required to keep track of the fact that pause
               //       has been called, and do the right thing when startRtpSend(..) gets called below
               //       (this typically means sending keepalive packets)
               rtpStream->pauseRtpSend( bOldStyleHoldRequested );
            }
            else
            {
               if (rtpStream->isPausing())
               {
                  rtpStream->resumeRtpSend();
               }
            }
            // .jjg. allow calling startRtpSend(..) even while sending so that we can update the codecs
            //if (!rtpStream->isSendingRtp())
            {
               // .jjg. ALWAYS start sending, because we need firewall keepalives (be they RTP packets with an
               //       unused pltype, or STUN packets) need to flow no matter what media direction gets negotiated
               rtpStream->startRtpSend(remoteIPAddress, remoteRtpPort, remoteIPAddress, remoteRtcpPort, matchedLocalCodecs, matchedRemoteCodecs);
            }
         }
         else
         {
            WarningLog(<< "adjustRTPStreams: handle=" << mHandle << ", something went wrong during SDP negotiations, no common codec found.");
         }
      }
      else if (remoteSdp && remoteRtpPort == 0 && rtpStream.get())
      {
         InviteSessionHandle hInvSess = getInviteSession();
         bool isEarly = false;
         if (hInvSess.isValid())
         {
            isEarly = hInvSess->isEarly();
         }
         if (!isEarly)
         {
            rtpStream->stopRtpSend();
            rtpStream->stopRtpReceive();
            mDialogSet.freeLocalRTPPort(supportedMedias[i]);
            m_onRtpStreamClosedConns[supportedMedias[i]].disconnect();
            m_onRtpStreamClosedConns.erase(supportedMedias[i]);
         }
      }

      // .jjg. note that all media directions are specified here
      // because the port is allocated, and we need keep-alives
      // to flow, so we'd better get our RTP stack listening
      if(rtpStream.get())
      {
         // .jjg. we now require calling startRtpReceive() even if we are already receiving
         // because we might need to update the list of codecs that were negotiated (this set can
         // change if, e.g., we get an INVITE with no body during a call)
         if (/*!rtpStream->isReceivingRtp() &&*/ localCodecs)
         {
            // !SLG! - we could make this better, no need to recalculate this every time
            // We are always willing to receive any of our locally supported codecs
            int numCodecs=0;
            SdpMediaLine::CodecList offeredCodecs;
            SdpMediaLine::CodecList::const_iterator itLocalCodec = localCodecs->begin();
            for(; itLocalCodec != localCodecs->end(); ++itLocalCodec)
            {
               numCodecs++;
               sdpcontainer::SdpCodec c(itLocalCodec->getPayloadType(),
                                                    itLocalCodec->getMimeType().c_str(),
                                                    itLocalCodec->getMimeSubtype().c_str(),
                                                    itLocalCodec->getRate(),
                                                    itLocalCodec->getPacketTime(),
                                                    itLocalCodec->getNumChannels(),
                                                    itLocalCodec->getFormatParameters().c_str());
               offeredCodecs.push_back(c);

               Data codecString;
               c.toString(codecString);
               InfoLog(<< "adjustRTPStreams: handle=" << mHandle << ", receving: " << codecString.data());
            }

            rtpStream->startRtpReceive(offeredCodecs);
         }
         InfoLog(<< "adjustRTPStreams: handle=" << mHandle << ", receiving...");
      }
   }

   if (remoteSdp)
   {
      ConversationManager::MediaDirection audioState = toMediaState((SdpMediaLine::SdpDirectionType)mediaDirection[0]);
      ConversationManager::MediaDirection videoState = toMediaState((SdpMediaLine::SdpDirectionType)mediaDirection[1]);

      if (mHandle) mConversationManager.onParticipantMediaChanged(mHandle, audioState, videoState, msg);
   }
}

void
RemoteParticipant::replaceWithParticipant(RemoteParticipant* replacingParticipant)
{
    // Copy our local hold setting to the replacing participant to replace us
    replacingParticipant->mMediaHoldStates = mMediaHoldStates;         

    // We are about to adjust the participant handle of the replacing participant to ours
    // ensure that the mapping is also adjusted in the replacing participants dialog set
    if(replacingParticipant->mHandle == replacingParticipant->mDialogSet.getActiveRemoteParticipantHandle())
    {
        replacingParticipant->mDialogSet.setActiveRemoteParticipantHandle(mHandle);
    }

    Participant::replaceWithParticipant(replacingParticipant);
}

void
RemoteParticipant::onDtmfEvent(int dtmf, int duration, bool up)
{
   if(mHandle) mConversationManager.onDtmfEvent(mHandle, dtmf, duration, up);
}

void
RemoteParticipant::onNewSession(ClientInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   InfoLog(<< "onNewSession(Client): handle=" << mHandle << ", " << msg.brief());
   mInviteSessionHandle = h->getSessionHandle();
   mDialogId = getDialogId();
   mDialogSet.setUACRedirected(resip::DialogId(Data::Empty, Data::Empty, Data::Empty));

   if (mHandle) mConversationManager.onNewOutgoingParticipant(mHandle, msg);
}

void
RemoteParticipant::onNewSession(ServerInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   InfoLog(<< "onNewSession(Server): handle=" << mHandle << ", " << msg.brief());

   // !jjg! removed, because we don't know how many/what types of media streams we will have yet
   //getLocalRTPPort(sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO);  // Allocate a port now - since it will be required as soon as we add this participant to a conversation

   mInviteSessionHandle = h->getSessionHandle();
   mDialogId = getDialogId();

   // First check if this INVITE is to replace an existing session
   if(msg.exists(h_Replaces))
   {
      pair<InviteSessionHandle, int> presult;
      presult = mDum.findInviteSession(msg.header(h_Replaces));
      if(!(presult.first == InviteSessionHandle::NotValid()))
      {
         RemoteParticipant* participantToReplace = dynamic_cast<RemoteParticipant *>(presult.first->getAppDialog().get());
         InfoLog(<< "onNewSession(Server): handle=" << mHandle << ", to replace handle=" << participantToReplace->getParticipantHandle() << ", " << msg.brief());

         if(mHandle) mConversationManager.onIncomingTransferRequest(mHandle, participantToReplace->mHandle, msg);
         return;
      }
   }
   else if (msg.exists(h_Join))
   {
      pair<InviteSessionHandle, int> presult;
      presult = mDum.findInviteSession(msg.header(h_Join));
      if(!(presult.first == InviteSessionHandle::NotValid()))
      {
         RemoteParticipant* participantToJoin = dynamic_cast<RemoteParticipant *>(presult.first->getAppDialog().get());
         InfoLog(<< "onNewSession(Server): handle=" << mHandle << ", to join handle=" << participantToJoin->getParticipantHandle() << ", " << msg.brief());

         if(mHandle) mConversationManager.onIncomingJoinRequest(mHandle, participantToJoin->mHandle, msg);
         return;
      }
   }

   // Check for Auto-Answer indication - support draft-ietf-answer-mode-01
   // and Answer-After parameter of Call-Info header
   ConversationProfile* profile = dynamic_cast<ConversationProfile*>(h->getUserProfile().get());
   assert(profile);
   bool autoAnswerRequired;
   bool autoAnswer = profile->shouldAutoAnswer(msg, &autoAnswerRequired);
   if(!autoAnswer && autoAnswerRequired)  // If we can't autoAnswer but it was required, we must reject the call
   {
      WarningCategory warning;
      warning.hostname() = DnsUtil::getLocalHostName();
      warning.code() = 399; /* Misc. */
      warning.text() = "automatic answer forbidden";
      setHandle(0); // Don't generate any callbacks for this rejected invite
      h->reject(403 /* Forbidden */, &warning);
      return;
   }

   // notify of new participant
   if(mHandle) mConversationManager.onIncomingParticipant(mHandle, msg, autoAnswer);
}

void
RemoteParticipant::onFailure(ClientInviteSessionHandle h, const SipMessage& msg)
{
   stateTransition(Terminating);
   InfoLog(<< "onFailure: handle=" << mHandle << ", " << msg.brief());
   // If ForkSelectMode is automatic, then ensure we destory any conversations, except the original
   if(mDialogSet.getForkSelectMode() == ConversationManager::ForkSelectAutomatic &&
      mHandle != mDialogSet.getActiveRemoteParticipantHandle())
   {
      destroyConversations();
   }
}

void
RemoteParticipant::onEarlyMedia(ClientInviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   InfoLog(<< "onEarlyMedia: handle=" << mHandle << ", " << msg.brief());
   if(!mDialogSet.isStaleFork(getDialogId()))
   {
      setRemoteSdp(sdp, true);

      // Check if there is a valid connection IP address for this session.
      // If there is, remember it for later (to support RTCP timers during an
      // "old-style" hold
      if (!isEqualNoCase(sdp.session().connection().getAddress(), "0.0.0.0"))
      {
         mLastRemoteIPAddr = sdp.session().connection().getAddress();
      }

      checkRemoteIceAttribs(sdp);

      adjustRTPStreams(false, &msg);

      if(mHandle) mConversationManager.onParticipantEarlyMedia(mHandle, msg);
   }
}

void 
RemoteParticipant::handleNonDialogCreatingProvisionalWithEarlyMedia(const resip::SipMessage& msg, const resip::SdpContents& sdp)
{
   InfoLog(<< "handleNonDialogCreatingProvisionalWithEarlyMedia: handle=" << mHandle << ", " << msg.brief());

   setRemoteSdp(sdp, true);

   // Check if there is a valid connection IP address for this session.
   // If there is, remember it for later (to support RTCP timers during an
   // "old-style" hold
   if (!isEqualNoCase(sdp.session().connection().getAddress(), "0.0.0.0"))
   {
      mLastRemoteIPAddr = sdp.session().connection().getAddress();
   }

   checkRemoteIceAttribs(sdp);

   adjustRTPStreams(false, &msg);

   if(mHandle) mConversationManager.onParticipantEarlyMedia(mHandle, msg);
}

void
RemoteParticipant::onProvisional(ClientInviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onProvisional: handle=" << mHandle << ", " << msg.brief());
   assert(msg.header(h_StatusLine).responseCode() != 100);

   if(!mDialogSet.isStaleFork(getDialogId()))
   {
      if(mHandle) mConversationManager.onParticipantAlerting(mHandle, msg);
   }
}

void
RemoteParticipant::onConnected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onConnected(Client): handle=" << mHandle << ", " << msg.brief());

   // Check if this is the first leg in a potentially forked call to send a 200
   if(!mDialogSet.isUACConnected())
   {
      if(mHandle) mConversationManager.onParticipantConnected(mHandle, msg);

      mDialogSet.setUACConnected(getDialogId(), mHandle);
      stateTransition(Connected);
   }
   else
   {
      // We already have a 200 response - send a BYE to this leg
      h->end();
   }
}

void
RemoteParticipant::onConnected(InviteSessionHandle, const SipMessage& msg)
{
   InfoLog(<< "onConnected: handle=" << mHandle << ", " << msg.brief());
   stateTransition(Connected);
}

void
RemoteParticipant::onStaleCallTimeout(ClientInviteSessionHandle)
{
   WarningLog(<< "onStaleCallTimeout: handle=" << mHandle);
}

void
RemoteParticipant::onTerminated(InviteSessionHandle h, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg)
{
   stateTransition(Terminating);
   switch(reason)
   {
   case InviteSessionHandler::RemoteBye:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", received a BYE from peer");
      break;
   case InviteSessionHandler::RemoteCancel:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", received a CANCEL from peer");
      break;
   case InviteSessionHandler::Rejected:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", received a rejection from peer");
      break;
   case InviteSessionHandler::LocalBye:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", ended locally via BYE");
      break;
   case InviteSessionHandler::LocalCancel:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", ended locally via CANCEL");
      break;
   case InviteSessionHandler::Replaced:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", ended due to being replaced");
      break;
   case InviteSessionHandler::Referred:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", ended due to being reffered");
      break;
   case InviteSessionHandler::Error:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", ended due to an error");
      break;
   case InviteSessionHandler::Timeout:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", ended due to a timeout");
      break;
   default:
      assert(false);
      break;
   }
   unsigned int statusCode = 0;
   if(msg)
   {
      if(msg->isResponse())
      {
         statusCode = msg->header(h_StatusLine).responseCode();
      }
   }

   // If this is a referred call and the refer is still around - then switch back to referrer (ie. failed transfer recovery)
   if(mHandle && mReferringAppDialog.isValid())
   {
      RemoteParticipant* participant = (RemoteParticipant*)mReferringAppDialog.get();
      replaceWithParticipant(participant);      // adjust conversation mappings
      if(participant->getParticipantHandle())
      {
         participant->adjustRTPStreams(false, msg);
         return;
      }
   }

   // Ensure terminating party is from answered fork before generating event
   //if(!mDialogSet.isStaleFork(getDialogId()))
   // !jjg! since we generate new ParticipantHandles and notify the app about them
   // when forking occurs, don't we need to call onParticipantTerminated for each one as well?
   {
      if(mHandle) mConversationManager.onParticipantTerminated(mHandle, statusCode, reason, msg);
   }
}

void
RemoteParticipant::onRedirected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onRedirected: handle=" << mHandle << ", " << msg.brief());
   // Check if this is the first leg in a potentially forked call to send a 200
   if(!mDialogSet.isUACConnected() && !mDialogSet.isUACRedirected())
   {
      mDialogSet.setUACRedirected(getDialogId());
      mDialogSet.destroyStaleDialogs(getDialogId());
      mConversationManager.onLocalParticipantRedirected(mHandle, msg);
   }
   else
   {
      // We already have a 200 response - send a BYE to this leg
      h->end();
   }
}

void
RemoteParticipant::onAnswer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   InfoLog(<< "onAnswer: handle=" << mHandle << ", " << msg.brief());

   // Ensure answering party is from answered fork before generating event
   if(!mDialogSet.isStaleFork(getDialogId()))
   {
      setRemoteSdp(sdp, true);

      // Check if there is a valid connection IP address for this session.
      // If there is, remember it for later (to support RTCP timers during an
      // "old-style" hold
      if (!isEqualNoCase(sdp.session().connection().getAddress(), "0.0.0.0"))
      {
         mLastRemoteIPAddr = sdp.session().connection().getAddress();
      }

      checkRemoteIceAttribs(sdp);

      adjustRTPStreams(false, &msg);
   }
   stateTransition(Connected);  // This is valid until PRACK is implemented
}

void
RemoteParticipant::onOffer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& offer)
{
   InfoLog(<< "onOffer: handle=" << mHandle << ", " << msg.brief());

   // Check to make sure we have *at least* one valid port open for accepting
   // media. If not, we must reject the entire request.
   unsigned int nBadPorts = 0;
   for( std::list<SdpContents::Session::Medium>::const_iterator iter = offer.session().media().begin() ;
        iter != offer.session().media().end() ; ++iter )
   {
      SdpMediaLine::SdpMediaType mt = SdpMediaLine::getMediaTypeFromString(iter->name().c_str());
      if (mt == SdpMediaLine::MEDIA_TYPE_UNKNOWN)
      {
         ++nBadPorts;
      }
      else if (getLocalRTPPort(mt, (iter->getConnections().front().getAddressType() == SdpContents::IP6)) == 0)
      {
         ++nBadPorts;
      }
   }

   if ((offer.session().media().size() > 0) && (nBadPorts >= offer.session().media().size()))
   {
      // "Not Acceptable Here", we can't match anything
      h->reject(488);
   }
   else
   {
      // We can accept at least one media. Proceed.
      if(mInviteSessionHandle.isValid())
      {
         // Don't set answer now - store offer and set when needed - so that sendHoldSdp() can be calculated at the right instant
         // we need to allow time for app to add to a conversation before alerting(with early flag) or answering
         mPendingOffer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(offer.clone()));

         // Check if there is a valid connection IP address for this session.
         bool oldStyleHoldRequested = false;
         if (isEqualNoCase(offer.session().connection().getAddress(), "0.0.0.0"))
         {
            oldStyleHoldRequested = true;
         }
         else
         {
            // If there is, remember it for later (to support RTCP timers during an
            // "old-style" hold
            mLastRemoteIPAddr = offer.session().connection().getAddress();
         }

         checkRemoteIceAttribs(offer);

         ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mInviteSessionHandle.get());
         if(mState == Connecting && sis && !sis->isAccepted())
         {
            // the user already knows that there is an incoming call that they have to accept/reject
            return;
         }

         ConversationManager::MediaDirection requestedAudioState = ConversationManager::MediaDirection_None;
         ConversationManager::MediaDirection requestedVideoState = ConversationManager::MediaDirection_None;
         for (std::list<SdpContents::Session::Medium>::const_iterator mediaIt = offer.session().media().begin();
              mediaIt != offer.session().media().end(); ++mediaIt)
         {
            const SdpContents::Session::Medium& m = *mediaIt;
            if (resip::isEqualNoCase("audio", m.name()) && m.port() != 0)
            {
               if (m.exists("sendonly") || oldStyleHoldRequested)
               {
                  requestedAudioState = ConversationManager::MediaDirection_SendOnly;
               }
               else if (m.exists("recvonly"))
               {
                  requestedAudioState = ConversationManager::MediaDirection_ReceiveOnly;
               }
               else if (m.exists("inactive"))
               {
                  requestedAudioState = ConversationManager::MediaDirection_Inactive;
               }
               else
               {
                  requestedAudioState = ConversationManager::MediaDirection_SendReceive;
               }
            }
            else if (resip::isEqualNoCase("video", m.name()) && m.port() != 0)
            {
               if (m.exists("sendonly") || oldStyleHoldRequested)
               {
                  requestedVideoState = ConversationManager::MediaDirection_SendOnly;
               }
               else if (m.exists("recvonly"))
               {
                  requestedVideoState = ConversationManager::MediaDirection_ReceiveOnly;
               }
               else if (m.exists("inactive"))
               {
                  requestedVideoState = ConversationManager::MediaDirection_Inactive;
               }
               else
               {
                  requestedVideoState = ConversationManager::MediaDirection_SendReceive;
               }
            }
         }

         // this is a re-INVITE
         // !jjg! todo: add an auto-accept option to the ConversationProfile so that existing recon apps don't break
         mConversationManager.onParticipantMediaChangeRequested(mHandle, requestedAudioState, requestedVideoState, &msg);
      }
   }
}

void
RemoteParticipant::onOfferRequired(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onOfferRequired: handle=" << mHandle << ", " << msg.brief());

   if(mState == Connecting && !h->isAccepted())
   {
      // If we haven't accepted yet - delay providing the offer until accept is called (this allows time
      // for a localParticipant to be added before generating the offer)
      mOfferRequired = true;
   }
   else
   {
      provideOffer(mState == Replacing /* postOfferAccept */);
      if(mState == Replacing)
      {
         stateTransition(Connecting);
      }
   }
}

void
RemoteParticipant::onOfferRejected(InviteSessionHandle, const SipMessage* msg)
{
   if(msg)
   {
      InfoLog(<< "onOfferRejected: handle=" << mHandle << ", " << msg->brief());

      // Fire an event if need be
      if (mLocalSdp)
      {
         // at this point, mLocalSdp represents the last "accepted" SDP;
         // i.e. the last SDP offer we sent that was accepted with a valid answer,
         // or the last SDP answer we sent
         ConversationManager::MediaDirection audioState = ConversationManager::MediaDirection_None;
         ConversationManager::MediaDirection videoState = ConversationManager::MediaDirection_None;
         for ( Sdp::MediaLineList::const_iterator mediaIt = mLocalSdp->getMediaLines().begin();
               mediaIt != mLocalSdp->getMediaLines().end(); ++mediaIt)
         {
            SdpMediaLine::SdpDirectionType mediaDirection = (*mediaIt)->getDirection();
            if ((*mediaIt)->getMediaType() == SdpMediaLine::MEDIA_TYPE_AUDIO)
            {
               if (mediaDirection == SdpMediaLine::DIRECTION_TYPE_SENDONLY)
                  audioState = ConversationManager::MediaDirection_SendOnly;
               else if (mediaDirection == SdpMediaLine::DIRECTION_TYPE_RECVONLY)
                  audioState = ConversationManager::MediaDirection_ReceiveOnly;
               else if (mediaDirection == SdpMediaLine::DIRECTION_TYPE_INACTIVE)
                  audioState = ConversationManager::MediaDirection_Inactive;
               else
                  audioState = ConversationManager::MediaDirection_SendReceive;
            }
            if ((*mediaIt)->getMediaType() == SdpMediaLine::MEDIA_TYPE_VIDEO)
            {
               if (mediaDirection == SdpMediaLine::DIRECTION_TYPE_SENDONLY)
                  videoState = ConversationManager::MediaDirection_SendOnly;
               else if (mediaDirection == SdpMediaLine::DIRECTION_TYPE_RECVONLY)
                  videoState = ConversationManager::MediaDirection_ReceiveOnly;
               else if (mediaDirection == SdpMediaLine::DIRECTION_TYPE_INACTIVE)
                  videoState = ConversationManager::MediaDirection_Inactive;
               else
                  videoState = ConversationManager::MediaDirection_SendReceive;
            }
         }

         stateTransition(Connected); // this is valid until PRACK is implemented

         mDialogSet.audioDirection() = audioState;
         mDialogSet.videoDirection() = videoState;
         if (mHandle) mConversationManager.onParticipantMediaChanged(mHandle, audioState, videoState, msg);
      }
   }
   else
   {
      InfoLog(<< "onOfferRejected: handle=" << mHandle);
   }
}

void
RemoteParticipant::onOfferRequestRejected(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onOfferRequestRejected: handle=" << mHandle << ", " << msg.brief());
   assert(0);  // We never send a request for an offer (ie. Invite with no SDP)
}

void
RemoteParticipant::onRemoteSdpChanged(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   InfoLog(<< "onRemoteSdpChanged: handle=" << mHandle << ", " << msg.brief());
   setRemoteSdp(sdp);
   adjustRTPStreams(false, &msg);
}

void
RemoteParticipant::onInfo(InviteSessionHandle, const SipMessage& msg)
{
   InfoLog(<< "onInfo: handle=" << mHandle << ", " << msg.brief());
   mConversationManager.onInfo(mHandle, msg);
}

void
RemoteParticipant::onInfoSuccess(InviteSessionHandle, const SipMessage& msg)
{
   InfoLog(<< "onInfoSuccess: handle=" << mHandle << ", " << msg.brief());
   assert(0);  // We never send an info request
}

void
RemoteParticipant::onInfoFailure(InviteSessionHandle, const SipMessage& msg)
{
   InfoLog(<< "onInfoFailure: handle=" << mHandle << ", " << msg.brief());
   assert(0);  // We never send an info request
}

void
RemoteParticipant::onRefer(InviteSessionHandle is, ServerSubscriptionHandle ss, const SipMessage& msg)
{
   InfoLog(<< "onRefer: handle=" << mHandle << ", " << msg.brief());

   try
   {
      // Create new Participant
      RemoteParticipantDialogSet *participantDialogSet = new RemoteParticipantDialogSet(mConversationManager);
      RemoteParticipant *participant = participantDialogSet->createUACOriginalRemoteParticipant(mConversationManager.getNewParticipantHandle());

      // Set pending OOD info in Participant - causes accept or reject to be called later
      participant->setPendingOODReferInfo(ss, msg);

      ss->send(ss->accept(202));

      // Notify application
      mConversationManager.onRequestOutgoingParticipant(participant->getParticipantHandle(), mHandle, msg);

      //// Accept the Refer
      //ss->send(ss->accept(202 /* Refer Accepted */));

      //// Figure out hold SDP before removing ourselves from the conversation
      //bool holdSdp = mLocalHold;

      //// Create new Participant - but use same participant handle
      //RemoteParticipantDialogSet* participantDialogSet = new RemoteParticipantDialogSet(mConversationManager, mDialogSet.getForkSelectMode());
      //RemoteParticipant *participant = participantDialogSet->createUACOriginalRemoteParticipant(mHandle); // This will replace old participant in ConversationManager map
      //participant->mReferringAppDialog = getHandle();

      //// Create offer
      //SdpContents offer;
      //participant->buildSdpOffer(holdSdp, offer);

      //replaceWithParticipant(participant);      // adjust conversation mappings - do this after buildSdpOffer, so that we have a bridge port

      //// Build the Invite
      //SharedPtr<SipMessage> NewInviteMsg = mDum.makeInviteSessionFromRefer(msg, ss->getHandle(), &offer, participantDialogSet);
      //mDialogSet.sendInvite(NewInviteMsg);

      //// Set RTP stack to listen
      //participant->adjustRTPStreams(true);
   }
   catch(BaseException &e)
   {
      WarningLog(<< "onRefer exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "onRefer unknown exception");
   }
}

void
RemoteParticipant::doReferNoSub(const SipMessage& msg)
{
   // Figure out hold SDP before removing ourselves from the conversation
   MediaHoldStateMap holdStates = mMediaHoldStates;

   // Create new Participant - but use same participant handle
   RemoteParticipantDialogSet* participantDialogSet = new RemoteParticipantDialogSet(mConversationManager, mDialogSet.getForkSelectMode());
   RemoteParticipant *participant = participantDialogSet->createUACOriginalRemoteParticipant(mHandle); // This will replace old participant in ConversationManager map
   participant->mReferringAppDialog = getHandle();

   // Create offer
   SdpContents offer;
   ConversationProfile* convProfile = dynamic_cast<ConversationProfile*>(mDialogSet.getUserProfile().get());
   participant->buildSdpOffer(convProfile, holdStates, offer);

   replaceWithParticipant(participant);      // adjust conversation mappings - do this after buildSdpOffer, so that we have a bridge port

   // Build the Invite
   SharedPtr<SipMessage> NewInviteMsg = mDum.makeInviteSessionFromRefer(msg, mDialogSet.getUserProfile(), &offer, participantDialogSet);
   mergeUnknownHeaders(msg.header(h_ReferTo).uri(), NewInviteMsg);
   participantDialogSet->sendInvite(NewInviteMsg); 

   // Set RTP stack to listen
   participant->adjustRTPStreams(true, &msg);
}

void
RemoteParticipant::onReferNoSub(InviteSessionHandle is, const SipMessage& msg)
{
   InfoLog(<< "onReferNoSub: handle=" << mHandle << ", " << msg.brief());

   try
   {
      // Accept the Refer
		is->acceptReferNoSub(202 /* Refer Accepted */);

      doReferNoSub(msg);
   }
   catch(BaseException &e)
   {
      WarningLog(<< "onReferNoSub exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "onReferNoSub unknown exception");
   }
}

void
RemoteParticipant::onReferAccepted(InviteSessionHandle, ClientSubscriptionHandle, const SipMessage& msg)
{
   InfoLog(<< "onReferAccepted: handle=" << mHandle << ", " << msg.brief());
}

void
RemoteParticipant::onReferRejected(InviteSessionHandle, const SipMessage& msg)
{
   InfoLog(<< "onReferRejected: handle=" << mHandle << ", " << msg.brief());
   if(msg.isResponse() && mState == Redirecting)
   {
      if(mHandle) mConversationManager.onParticipantRedirectFailure(mHandle, msg.header(h_StatusLine).responseCode(), msg.header(h_StatusLine).reason(), &msg);
      stateTransition(Connected);
   }
}

void
RemoteParticipant::onMessage(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onMessage: handle=" << mHandle << ", " << msg.brief());
   h->acceptNIT();
}

void
RemoteParticipant::onMessageSuccess(InviteSessionHandle, const SipMessage& msg)
{
   InfoLog(<< "onMessageSuccess: handle=" << mHandle << ", " << msg.brief());
}

void
RemoteParticipant::onMessageFailure(InviteSessionHandle, const SipMessage& msg)
{
   InfoLog(<< "onMessageFailure: handle=" << mHandle << ", " << msg.brief());
}

void
RemoteParticipant::onForkDestroyed(ClientInviteSessionHandle)
{
   InfoLog(<< "onForkDestroyed: handle=" << mHandle);
}


////////////////////////////////////////////////////////////////////////////////
// ClientSubscriptionHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
RemoteParticipant::onUpdatePending(ClientSubscriptionHandle h, const SipMessage& notify, bool outOfOrder)
{
   InfoLog(<< "onUpdatePending(ClientSub): handle=" << mHandle << ", " << notify.brief());
   if (notify.exists(h_Event) && notify.header(h_Event).value() == "refer")
   {
      h->acceptUpdate();
      processReferNotify(notify);
   }
   else
   {
      h->rejectUpdate(400, Data("Only notifies for refers are allowed."));
   }
}

void
RemoteParticipant::onUpdateActive(ClientSubscriptionHandle h, const SipMessage& notify, bool outOfOrder)
{
   InfoLog(<< "onUpdateActive(ClientSub): handle=" << mHandle << ", " << notify.brief());
   if (notify.exists(h_Event) && notify.header(h_Event).value() == "refer")
   {
      h->acceptUpdate();
      processReferNotify(notify);
   }
   else
   {
      h->rejectUpdate(400, Data("Only notifies for refers are allowed."));
   }
}

void
RemoteParticipant::onUpdateExtension(ClientSubscriptionHandle h, const SipMessage& notify, bool outOfOrder)
{
   InfoLog(<< "onUpdateExtension(ClientSub): handle=" << mHandle << ", " << notify.brief());
   if (notify.exists(h_Event) && notify.header(h_Event).value() == "refer")
   {
      h->acceptUpdate();
      processReferNotify(notify);
   }
   else
   {
      h->rejectUpdate(400, Data("Only notifies for refers are allowed."));
   }
}

void
RemoteParticipant::onTerminated(ClientSubscriptionHandle h, const SipMessage* notify)
{
   if(notify)
   {
      InfoLog(<< "onTerminated(ClientSub): handle=" << mHandle << ", " << notify->brief());
      if (notify->isRequest() && notify->exists(h_Event) && notify->header(h_Event).value() == "refer")
      {
         // Note:  Final notify is sometimes only passed in the onTerminated callback
         processReferNotify(*notify);
      }
      else if(notify->isResponse() && mState == Redirecting)
      {
         if(mHandle) mConversationManager.onParticipantRedirectFailure(mHandle, notify->header(h_StatusLine).responseCode(), notify->header(h_StatusLine).reason(), notify);
         stateTransition(Connected);
      }
   }
   else
   {
      // Timed out waiting for notify
      InfoLog(<< "onTerminated(ClientSub): handle=" << mHandle);
      if(mState == Redirecting)
      {
         if(mHandle) mConversationManager.onParticipantRedirectFailure(mHandle, 408, resip::Data::Empty, notify);
         stateTransition(Connected);
      }
   }
}

void
RemoteParticipant::onNewSubscription(ClientSubscriptionHandle h, const SipMessage& notify)
{
   InfoLog(<< "onNewSubscription(ClientSub): handle=" << mHandle << ", " << notify.brief());
}

int
RemoteParticipant::onRequestRetry(ClientSubscriptionHandle h, int retryMinimum, const SipMessage& notify)
{
   InfoLog(<< "onRequestRetry(ClientSub): handle=" << mHandle << ", " << notify.brief());
   return -1;
}

void
RemoteParticipant::onRtpStreamClosed(sdpcontainer::SdpMediaLine::SdpMediaType mediaType, RtpStream::ClosedReason reason, const asio::error_code& ec)
{
   class HandleRtpStreamClosedCmd : public DumCommandStub
   {
   public:
      HandleRtpStreamClosedCmd(RemoteParticipant* rp, sdpcontainer::SdpMediaLine::SdpMediaType mediaType, RtpStream::ClosedReason reason, const asio::error_code& ec)
         : mRemoteParticipant(rp), mMediaType(mediaType), mReason(reason), mEc(ec) {}
      virtual void executeCommand()
      {
         mRemoteParticipant->handleRtpStreamClosed(mMediaType, mReason, mEc);
      }
   private:
      RemoteParticipant* mRemoteParticipant;
      sdpcontainer::SdpMediaLine::SdpMediaType mMediaType;
      RtpStream::ClosedReason mReason;
      asio::error_code mEc;
   };
   mDum.post(new HandleRtpStreamClosedCmd(this, mediaType, reason, ec));   
}

void
RemoteParticipant::handleRtpStreamClosed(sdpcontainer::SdpMediaLine::SdpMediaType mediaType, RtpStream::ClosedReason reason, const asio::error_code& ec)
{
   Data reasonStr;
   {
      resip::DataStream ds(reasonStr);
      ds << "RTP stream closed; mediaType=" << sdpcontainer::SdpMediaLine::SdpMediaTypeString[mediaType] << "; reason=" << reason;
      if (ec)
      {
         ds << "; error_code=" << ec.value();
      }
   }
   if (reason == RtpStream::ClosedReason_UserRequest)
   {
      InfoLog(<< "RTP stream closed by request (likely m= line with port==0): " << reasonStr);
   }
   else
   {
      mConversationManager.destroyParticipant(mHandle, reasonStr);
   }
}

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
