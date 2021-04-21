// sipX includes
#if (_MSC_VER >= 1600)
#include <stdint.h>       // Use Visual Studio's stdint.h
#define _MSC_STDINT_H_    // This define will ensure that stdint.h in sipXport tree is not used
#endif
#include <sdp/SdpCodec.h>
#include <os/OsConfigDb.h>
#include <mp/MpCodecFactory.h>
#include <mp/MprBridge.h>
#include <mp/MpResourceTopology.h>
#include <mi/CpMediaInterfaceFactoryFactory.h>
#include <mi/CpMediaInterface.h>

// resip includes
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>

#include "ReconSubsystem.hxx"
#include "ConversationManagerCmds.hxx"
#include "RTPPortManager.hxx"
#include "SipXConversationManager.hxx"
#include "SipXBridgeMixer.hxx"
#include "SipXLocalParticipant.hxx"
#include "SipXRemoteParticipantDialogSet.hxx"


#include <rutil/WinLeakCheck.hxx>

#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4355 )
#endif

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

SipXConversationManager::SipXConversationManager(bool localAudioEnabled, MediaInterfaceMode mediaInterfaceMode)
: mLocalAudioEnabled(localAudioEnabled),
  mMediaInterfaceMode(mediaInterfaceMode),
  mMediaFactory(0),
  mBridgeMixer(0),
  mSipXTOSValue(0)
{
   init();
}

SipXConversationManager::SipXConversationManager(bool localAudioEnabled, MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate)
: ConversationManager(defaultSampleRate, maxSampleRate),
  mLocalAudioEnabled(localAudioEnabled),
  mMediaInterfaceMode(mediaInterfaceMode),
  mMediaFactory(0),
  mBridgeMixer(0),
  mSipXTOSValue(0)
{
   init(defaultSampleRate, maxSampleRate);
}

SipXConversationManager::~SipXConversationManager()
{
   delete mBridgeMixer;
   if(mMediaInterface) mMediaInterface.reset();  // Make sure inteface is destroyed before factory
   sipxDestroyMediaFactoryFactory();
}

void
SipXConversationManager::init(int defaultSampleRate, int maxSampleRate)
{
#ifdef _DEBUG
   UtlString codecPaths[] = {".", "../../../../sipXtapi/sipXmediaLib/bin"};
#else
   UtlString codecPaths[] = {"."};
#endif
   int codecPathsNum = sizeof(codecPaths)/sizeof(codecPaths[0]);
   OsStatus rc = CpMediaInterfaceFactory::addCodecPaths(codecPathsNum, codecPaths);
   resip_assert(OS_SUCCESS == rc);

   if(mMediaInterfaceMode == sipXConversationMediaInterfaceMode)
   {
      OsConfigDb sipXconfig;
      sipXconfig.set("PHONESET_MAX_ACTIVE_CALLS_ALLOWED",300);  // This controls the maximum number of flowgraphs allowed - default is 16
      mMediaFactory = sipXmediaFactoryFactory(&sipXconfig, 0, defaultSampleRate, maxSampleRate, mLocalAudioEnabled);
   }
   else
   {
      mMediaFactory = sipXmediaFactoryFactory(NULL, 0, defaultSampleRate, maxSampleRate, mLocalAudioEnabled);
   }

   // Create MediaInterface
   MpCodecFactory *pCodecFactory = MpCodecFactory::getMpCodecFactory();
   unsigned int count = 0;
   const MppCodecInfoV1_1 **codecInfoArray;
   pCodecFactory->getCodecInfoArray(count, codecInfoArray);

   if(count == 0)
   {
      // the default path and CODEC_PLUGINS_FILTER don't appear to work on Linux
      // also see the call to addCodecPaths above
      InfoLog(<<"No statically linked codecs or no codecs found with default filter, trying without a filter");
      pCodecFactory->loadAllDynCodecs(NULL, "");
      pCodecFactory->getCodecInfoArray(count, codecInfoArray);
      if(count == 0)
      {
         CritLog( << "No static codecs or dynamic codec plugins found in search paths.  Cannot start.");
         exit(-1);
      }
   }

   InfoLog( << "Loaded codecs are:");
   for(unsigned int i =0; i < count; i++)
   {
      InfoLog( << "  " << codecInfoArray[i]->codecName
               << "(" << codecInfoArray[i]->codecManufacturer << ") "
               << codecInfoArray[i]->codecVersion
               << " MimeSubtype: " << codecInfoArray[i]->mimeSubtype
               << " Rate: " << codecInfoArray[i]->sampleRate
               << " Channels: " << codecInfoArray[i]->numChannels);
   }

   if(mMediaInterfaceMode == sipXGlobalMediaInterfaceMode)
   {
      createMediaInterfaceAndMixer(mLocalAudioEnabled /* giveFocus?*/,    // This is the one and only media interface - give it focus
                                   0,
                                   mMediaInterface,
                                   &mBridgeMixer);
   }
}

void
SipXConversationManager::setUserAgent(UserAgent* userAgent)
{
   ConversationManager::setUserAgent(userAgent);

   // Note: This is not really required, since we are managing the port allocation - but no harm done
   // Really not needed now - since FlowManager integration
   //mMediaFactory->getFactoryImplementation()->setRtpPortRange(mUserAgent->getUserAgentMasterProfile()->rtpPortRangeMin(),
   //                                                           mUserAgent->getUserAgentMasterProfile()->rtpPortRangeMax());

   if(!mRTPPortManager)
   {
      mRTPPortManager.reset(new RTPPortManager(userAgent->getUserAgentMasterProfile()->rtpPortRangeMin(), userAgent->getUserAgentMasterProfile()->rtpPortRangeMax()));
   }
}

ParticipantHandle
SipXConversationManager::createLocalParticipant()
{
   ParticipantHandle partHandle = 0;
   if(mLocalAudioEnabled)
   {
      partHandle = getNewParticipantHandle();

      CreateLocalParticipantCmd* cmd = new CreateLocalParticipantCmd(this, partHandle);
      post(cmd);
   }
   else
   {
      WarningLog(<< "createLocalParticipant called when local audio support is disabled.");
   }

   return partHandle;
}

void
SipXConversationManager::outputBridgeMatrix()
{
   if(mMediaInterfaceMode == sipXGlobalMediaInterfaceMode)
   {
      OutputBridgeMixWeightsCmd* cmd = new OutputBridgeMixWeightsCmd(this);
      post(cmd);
   }
   else
   {
      WarningLog(<< "ConversationManager::outputBridgeMatrix not supported in current Media Interface Mode");
   }
}

void
SipXConversationManager::setSpeakerVolume(int volume)
{
   OsStatus status =  mMediaFactory->getFactoryImplementation()->setSpeakerVolume(volume);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "setSpeakerVolume failed: status=" << status);
   }
}

void
SipXConversationManager::setMicrophoneGain(int gain)
{
   OsStatus status =  mMediaFactory->getFactoryImplementation()->setMicrophoneGain(gain);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "setMicrophoneGain failed: status=" << status);
   }
}

void
SipXConversationManager::muteMicrophone(bool mute)
{
   OsStatus status =  mMediaFactory->getFactoryImplementation()->muteMicrophone(mute? TRUE : FALSE);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "muteMicrophone failed: status=" << status);
   }
}

void
SipXConversationManager::enableEchoCancel(bool enable)
{
   OsStatus status =  mMediaFactory->getFactoryImplementation()->setAudioAECMode(enable ? MEDIA_AEC_CANCEL : MEDIA_AEC_DISABLED);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "enableEchoCancel failed: status=" << status);
   }
   if(mMediaInterfaceMode == sipXGlobalMediaInterfaceMode)  // Note for sipXConversationMediaInterfaceMode - setting will apply on next conversation given focus
   {
      mMediaInterface->getInterface()->defocus();   // required to apply changes
      mMediaInterface->getInterface()->giveFocus();
   }
}

void
SipXConversationManager::enableAutoGainControl(bool enable)
{
   OsStatus status =  mMediaFactory->getFactoryImplementation()->enableAGC(enable ? TRUE : FALSE);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "enableAutoGainControl failed: status=" << status);
   }
   if(mMediaInterfaceMode == sipXGlobalMediaInterfaceMode)  // Note for sipXConversationMediaInterfaceMode - setting will apply on next conversation given focus
   {
      mMediaInterface->getInterface()->defocus();   // required to apply changes
      mMediaInterface->getInterface()->giveFocus();
   }
}

void
SipXConversationManager::enableNoiseReduction(bool enable)
{
   OsStatus status =  mMediaFactory->getFactoryImplementation()->setAudioNoiseReductionMode(enable ? MEDIA_NOISE_REDUCTION_MEDIUM /* arbitrary */ : MEDIA_NOISE_REDUCTION_DISABLED);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "enableNoiseReduction failed: status=" << status);
   }
   if(mMediaInterfaceMode == sipXGlobalMediaInterfaceMode)  // Note for sipXConversationMediaInterfaceMode - setting will apply on next conversation given focus
   {
      mMediaInterface->getInterface()->defocus();   // required to apply changes
      mMediaInterface->getInterface()->giveFocus();
   }
}

void
SipXConversationManager::buildSessionCapabilities(const resip::Data& ipaddress, unsigned int numCodecIds,
                                              unsigned int codecIds[], resip::SdpContents& sessionCaps)
{
   sessionCaps = SdpContents::Empty;  // clear out passed in SdpContents

   // Check if ipaddress is V4 or V6
   bool v6 = false;
   if(!ipaddress.empty())
   {
      Tuple testTuple(ipaddress, 0, UDP);
      if(testTuple.ipVersion() == V6)
      {
         v6 = true;
      }
   }

   // Create Session Capabilities
   // Note:  port, sessionId and version will be replaced in actual offer/answer
   // Build s=, o=, t=, and c= lines
   SdpContents::Session::Origin origin("-", 0 /* sessionId */, 0 /* version */, v6 ? SdpContents::IP6 : SdpContents::IP4, ipaddress.empty() ? "0.0.0.0" : ipaddress);   // o=
   SdpContents::Session session(0, origin, "-" /* s= */);
   session.connection() = SdpContents::Session::Connection(v6 ? SdpContents::IP6 : SdpContents::IP4, ipaddress.empty() ? "0.0.0.0" : ipaddress);  // c=
   session.addTime(SdpContents::Session::Time(0, 0));

   MpCodecFactory *pCodecFactory = MpCodecFactory::getMpCodecFactory();
   SdpCodecList codecList;
   pCodecFactory->addCodecsToList(codecList);
   codecList.bindPayloadTypes();

   //UtlString output;
   //codecList.toString(output);
   //InfoLog( << "Codec List: " << output.data());

   // Auto-Create Session Codec Capabilities
   // Note:  port, and potentially payloadid will be replaced in actual offer/answer

   // Build Codecs and media offering
   SdpContents::Session::Medium medium("audio", 0, 1, "RTP/AVP");

   bool firstCodecAdded = false;
   for(unsigned int idIter = 0; idIter < numCodecIds; idIter++)
   {
      const SdpCodec* sdpcodec = codecList.getCodec((SdpCodec::SdpCodecTypes)codecIds[idIter]);
      if(sdpcodec)
      {
         UtlString mediaType;
         sdpcodec->getMediaType(mediaType);
         // Ensure this codec is an audio codec
         if(mediaType.compareTo("audio", UtlString::ignoreCase) == 0)
         {
            UtlString mimeSubType;
            sdpcodec->getEncodingName(mimeSubType);
            //mimeSubType.toUpper();

            int capabilityRate = sdpcodec->getSampleRate();
            if(mimeSubType == "G722")
            {
               capabilityRate = 8000;
            }

            Data codecName(mimeSubType.data());
#ifdef RECON_SDP_ENCODING_NAMES_CASE_HACK
            // The encoding names are not supposed to be case sensitive.
            // However, some phones, including Polycom, don't recognize
            // telephone-event if it is not lowercase.
            // sipXtapi is writing all codec names in uppercase.
            // (see sipXtapi macro SDP_MIME_SUBTYPE_TO_CASE) and this
            // hack works around that.
            if(mimeSubType.compareTo("telephone-event", UtlString::ignoreCase) == 0)
            {
               codecName = "telephone-event";
            }
#endif
            SdpContents::Session::Codec codec(codecName, sdpcodec->getCodecPayloadFormat(), capabilityRate);
            if(sdpcodec->getNumChannels() > 1)
            {
               codec.encodingParameters() = Data(sdpcodec->getNumChannels());
            }

            // Check for telephone-event and add fmtp manually
            if(mimeSubType.compareTo("telephone-event", UtlString::ignoreCase) == 0)
            {
               codec.parameters() = Data("0-15");
            }
            else
            {
               UtlString fmtpField;
               sdpcodec->getSdpFmtpField(fmtpField);
               if(fmtpField.length() != 0)
               {
                  codec.parameters() = Data(fmtpField.data());
               }
            }

            DebugLog(<< "Added codec to session capabilites: id=" << codecIds[idIter]
                    << " type=" << mimeSubType.data()
                    << " rate=" << sdpcodec->getSampleRate()
                    << " plen=" << sdpcodec->getPacketLength()
                    << " payloadid=" << sdpcodec->getCodecPayloadFormat()
                    << " fmtp=" << codec.parameters());

            medium.addCodec(codec);
            if(!firstCodecAdded)
            {
               firstCodecAdded = true;

               // 20 ms of speech per frame (note G711 has 10ms samples, so this is 2 samples per frame)
               // Note:  There are known problems with SDP and the ptime attribute.  For now we will choose an
               // appropriate ptime from the first codec
               medium.addAttribute("ptime", Data(sdpcodec->getPacketLength() / 1000));
            }
         }
      }
   }

   session.addMedium(medium);
   sessionCaps.session() = session;
}

void
SipXConversationManager::createMediaInterfaceAndMixer(bool giveFocus,
                                                  ConversationHandle ownerConversationHandle,
                                                  std::shared_ptr<MediaInterface>& mediaInterface,
                                                  SipXBridgeMixer** bridgeMixer)
{
   UtlString localRtpInterfaceAddress("127.0.0.1");  // Will be overridden in RemoteParticipantDialogSet, when connection is created anyway

   // Note:  STUN and TURN capabilities of the sipX media stack are not used - the FlowManager is responsible for STUN/TURN
   mediaInterface = std::make_shared<MediaInterface>(*this, ownerConversationHandle, mMediaFactory->createMediaInterface(NULL,
            localRtpInterfaceAddress,
            0,     /* numCodecs - not required at this point */
            0,     /* codecArray - not required at this point */
            NULL,  /* local */
            mSipXTOSValue,  /* TOS Options */
            NULL,  /* STUN Server Address */
            0,     /* STUN Options */
            25,    /* STUN Keepalive period (seconds) */
            NULL,  /* TURN Server Address */
            0,     /* TURN Port */
            NULL,  /* TURN User */
            NULL,  /* TURN Password */
            25,    /* TURN Keepalive period (seconds) */
            false)); /* enable ICE? */

   // Register the NotificationDispatcher class (derived from OsMsgDispatcher)
   // as the sipX notification dispatcher
   mediaInterface->getInterface()->setNotificationDispatcher(mediaInterface.get());

   // Turn on notifications for all resources...
   mediaInterface->getInterface()->setNotificationsEnabled(true);

   if(giveFocus)
   {
      mediaInterface->getInterface()->giveFocus();
   }

   *bridgeMixer = new SipXBridgeMixer(*(mediaInterface->getInterface()));
}

bool
SipXConversationManager::supportsJoin()
{
   return (getMediaInterfaceMode() == SipXConversationManager::sipXGlobalMediaInterfaceMode);
}

LocalParticipant *
createLocalParticipantImpl(ParticipantHandle partHandle)
{
   new SipXLocalParticipant(partHandle, *this);
}

RemoteParticipantDialogSet *
SipXConversationManager::createRemoteParticipantDialogSet(
      ConversationManager::ParticipantForkSelectMode forkSelectMode,
      std::shared_ptr<ConversationProfile> conversationProfile)
{
   return new SipXRemoteParticipantDialogSet(*this, forkSelectMode, conversationProfile);
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
