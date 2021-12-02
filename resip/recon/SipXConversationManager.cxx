// sipX includes
#if (_MSC_VER >= 1600)
#include <stdint.h>       // Use Visual Studio's stdint.h
#define _MSC_STDINT_H_    // This define will ensure that stdint.h in sipXport tree is not used
#endif
#include <sdp/SdpCodec.h>
#include <os/OsConfigDb.h>
#include <mp/MpCodecFactory.h>
#include <mp/MprBridge.h>
#include <mp/MpResourceFactory.h>
#include <mp/MpResourceTopology.h>
#include <mi/CpMediaInterfaceFactoryFactory.h>
#include <CpTopologyGraphFactoryImpl.h>

// resip includes
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/Lock.hxx>
#include <rutil/Random.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/ClientInviteSession.hxx>
#include <resip/dum/ServerInviteSession.hxx>
#include <resip/dum/ClientSubscription.hxx>
#include <resip/dum/ServerOutOfDialogReq.hxx>

#include "ReconSubsystem.hxx"
#include "UserAgent.hxx"
#include "SipXConversationManager.hxx"
#include "ConversationManagerCmds.hxx"
#include "SipXConversation.hxx"
#include "Participant.hxx"
#include "SipXBridgeMixer.hxx"
#include "DtmfEvent.hxx"
#include "SipXLocalParticipant.hxx"
#include "SipXMediaResourceParticipant.hxx"
#include "SipXRemoteParticipant.hxx"
#include "Conversation.hxx"
#include <rutil/WinLeakCheck.hxx>

#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4355 )
#endif

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

constexpr char SipXConversationManager::DEFAULT_FROM_FILE_2_RESOURCE_NAME[];
constexpr char SipXConversationManager::DEFAULT_RECORDER_2_RESOURCE_NAME[];

SipXConversationManager::SipXConversationManager(bool localAudioEnabled, MediaInterfaceMode mediaInterfaceMode, bool enableExtraPlayAndRecordResources)
: ConversationManager(),
  mLocalAudioEnabled(localAudioEnabled),
  mMediaInterfaceMode(mediaInterfaceMode),
  mEnableExtraPlayAndRecordResources(enableExtraPlayAndRecordResources),
  mMediaFactory(0),
  mSipXTOSValue(0)
{
   init();
}

SipXConversationManager::SipXConversationManager(bool localAudioEnabled, MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate, bool enableExtraPlayAndRecordResources)
: ConversationManager(),
  mLocalAudioEnabled(localAudioEnabled),
  mMediaInterfaceMode(mediaInterfaceMode),
  mEnableExtraPlayAndRecordResources(enableExtraPlayAndRecordResources),
  mMediaFactory(0),
  mSipXTOSValue(0)
{
   init(defaultSampleRate, maxSampleRate);
}

void
SipXConversationManager::init(int defaultSampleRate, int maxSampleRate)
{
#ifdef _DEBUG

  #if _WIN64
     UtlString codecPaths[] = {".", "../../../x64/Debug"};
  #else
    #if _WIN32
      UtlString codecPaths[] = { ".", "../../../Win32/Debug" };
    #else
      UtlString codecPaths[] = { "." };
    #endif
  #endif

#else

  #if _WIN64
    UtlString codecPaths[] = { ".", "../../../x64/Release" };
  #else
    #if _WIN32
      UtlString codecPaths[] = { ".", "../../../Win32/Release" };
    #else
      UtlString codecPaths[] = { "." };
    #endif
  #endif

#endif

   int codecPathsNum = sizeof(codecPaths)/sizeof(codecPaths[0]);
   OsStatus rc = CpTopologyGraphFactoryImpl::addCodecPaths(codecPathsNum, codecPaths);
   resip_assert(OS_SUCCESS == rc);

   if(mMediaInterfaceMode == sipXConversationMediaInterfaceMode)
   {
      OsConfigDb sipXconfig;
      sipXconfig.set("PHONESET_MAX_ACTIVE_CALLS_ALLOWED",300);  // This controls the maximum number of flowgraphs allowed - default is 16
      mMediaFactory = dynamic_cast<CpTopologyGraphFactoryImpl*>(sipXmediaFactoryFactory(&sipXconfig, 0, maxSampleRate, defaultSampleRate, mLocalAudioEnabled)->getFactoryImplementation());
   }
   else
   {
      mMediaFactory = dynamic_cast<CpTopologyGraphFactoryImpl*>(sipXmediaFactoryFactory(nullptr, 0, maxSampleRate, defaultSampleRate, mLocalAudioEnabled)->getFactoryImplementation());
   }

   if (mMediaFactory == nullptr)
   {
      CritLog(<< "Error creating media factory for sipX Topology Graph.  Cannot start.");
      exit(-1);
   }

   if (mEnableExtraPlayAndRecordResources)
   {
      if (!addExtraPlayAndRecordResourcesToTopology())
      {
         CritLog(<< "Error adjusting resource Topology.  Cannot start.");
         exit(-1);
      }
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
                                   mMediaInterface, 
                                   getBridgeMixer());
   }
}

SipXConversationManager::~SipXConversationManager()
{
   getBridgeMixer().reset();   // Make sure the mixer is destroyed before the media interface
   mMediaInterface.reset();    // Make sure inteface is destroyed before factory
   sipxDestroyMediaFactoryFactory();
}

void
SipXConversationManager::setUserAgent(UserAgent* userAgent)
{
   ConversationManager::setUserAgent(userAgent);

   if (mMediaInterface)
   {
      // Enable/Disable DTMF digit logging according to UserAgentMasterProfile setting
      mMediaInterface->allowLoggingDTMFDigits(userAgent->getUserAgentMasterProfile()->dtmfDigitLoggingEnabled());
   }

   // Note: This is not really required, since we are managing the port allocation - but no harm done
   // Really not needed now - since FlowManager integration
   //mMediaFactory->setRtpPortRange(getUserAgent()->getUserAgentMasterProfile()->rtpPortRangeMin(),
   //                               getUserAgent()->getUserAgentMasterProfile()->rtpPortRangeMax());

   if(!mRTPPortManager)
   {
      mRTPPortManager.reset(new RTPPortManager(getUserAgent()->getUserAgentMasterProfile()->rtpPortRangeMin(), getUserAgent()->getUserAgentMasterProfile()->rtpPortRangeMax()));
   }
}

ConversationHandle
SipXConversationManager::createSharedMediaInterfaceConversation(ConversationHandle sharedMediaInterfaceConvHandle, AutoHoldMode autoHoldMode)
{
   if (isShuttingDown()) return 0;  // Don't allow new things to be created when we are shutting down
   if (mMediaInterfaceMode == sipXGlobalMediaInterfaceMode)
   {
      assert(false);
      WarningLog(<< "Calling createSharedMediaInterfaceConversation is not appropriate when using sipXGlobalMediaInterfaceMode");
      return 0;
   }

   ConversationHandle convHandle = getNewConversationHandle();

   CreateConversationCmd* cmd = new CreateConversationCmd(this, convHandle, autoHoldMode, sharedMediaInterfaceConvHandle);
   post(cmd);
   return convHandle;
}

void 
SipXConversationManager::outputBridgeMatrix(ConversationHandle convHandle)
{
   OutputBridgeMixWeightsCmd* cmd = new OutputBridgeMixWeightsCmd(this, convHandle);
   post(cmd);
}

void
SipXConversationManager::outputBridgeMatrixImpl(ConversationHandle convHandle)
{
   // Note: convHandle of 0 only makes sense if sipXGlobalMediaInterfaceMode is enabled
   if (convHandle == 0)
   {
      if (getBridgeMixer() != 0)
      {
         getBridgeMixer()->outputBridgeMixWeights();
      }
      else
      {
         WarningLog(<< "ConversationManager::outputBridgeMatrix request with no conversation handle is not appropriate for current MediaInterfaceMode");
      }
   }
   else
   {
      Conversation* conversation = getConversation(convHandle);
      if (conversation)
      {
         if (conversation->getBridgeMixer() != 0)
         {
            conversation->getBridgeMixer()->outputBridgeMixWeights();
         }
         else
         {
            WarningLog(<< "ConversationManager::outputBridgeMatrix requested conversation wihtout a mixer/media interface, conversationHandle=" << convHandle);
         }
      }
      else
      {
         WarningLog(<< "ConversationManager::outputBridgeMatrix requested for non-existing conversationHandle=" << convHandle);
      }
   }
}

void
SipXConversationManager::setSpeakerVolume(int volume)
{
   OsStatus status =  mMediaFactory->setSpeakerVolume(volume);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "setSpeakerVolume failed: status=" << status);
   }
}

void 
SipXConversationManager::setMicrophoneGain(int gain)
{
   OsStatus status =  mMediaFactory->setMicrophoneGain(gain);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "setMicrophoneGain failed: status=" << status);
   }
}

void 
SipXConversationManager::muteMicrophone(bool mute)
{
   OsStatus status =  mMediaFactory->muteMicrophone(mute? TRUE : FALSE);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "muteMicrophone failed: status=" << status);
   }
}
 
void 
SipXConversationManager::enableEchoCancel(bool enable)
{
   OsStatus status =  mMediaFactory->setAudioAECMode(enable ? MEDIA_AEC_CANCEL : MEDIA_AEC_DISABLED);
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
   OsStatus status =  mMediaFactory->enableAGC(enable ? TRUE : FALSE);
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
   OsStatus status =  mMediaFactory->setAudioNoiseReductionMode(enable ? MEDIA_NOISE_REDUCTION_MEDIUM /* arbitrary */ : MEDIA_NOISE_REDUCTION_DISABLED);
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
SipXConversationManager::buildSessionCapabilities(const resip::Data& ipaddress,
                                              const std::vector<unsigned int>& codecIds, resip::SdpContents& sessionCaps)
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
   for(std::vector<unsigned int>::const_iterator idIter = codecIds.begin();
      idIter != codecIds.end(); idIter++)
   {
      const unsigned int& codecId = *idIter;
      const SdpCodec* sdpcodec = codecList.getCodec((SdpCodec::SdpCodecTypes)codecId);
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
            // The encoding names are not supposed to be case sensitive.
            // However, some phones, including Polycom, don't recognize
            // telephone-event if it is not lowercase.
            // sipXtapi is writing all codec names in uppercase.
            // (see sipXtapi macro SDP_MIME_SUBTYPE_TO_CASE) and this
            // hack works around that.
            // It is more common to use lowercase, so we just lowercase everything.
            codecName.lowercase();

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

            DebugLog(<< "Added codec to session capabilites: id=" << codecId
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
                                                  std::shared_ptr<SipXMediaInterface>& mediaInterface,
                                                  std::shared_ptr<BridgeMixer>& bridgeMixer)
{
   UtlString localRtpInterfaceAddress("127.0.0.1");  // Will be overridden in RemoteParticipantDialogSet, when connection is created anyway

   // Note:  STUN and TURN capabilities of the sipX media stack are not used - the FlowManager is responsible for STUN/TURN
   // TODO SLG - if DISABLE_FLOWMANAGER define is on we should consider enabling stun or turn options
   mediaInterface = std::make_shared<SipXMediaInterface>(*this, (CpTopologyGraphInterface*)mMediaFactory->createMediaInterface(NULL,
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
            false, /* enable ICE? */
            0,     /* samplesPerSec, 0 = default */
            nullptr)); /* pDispatcher - set below in setNotificationDispatcher call */

   // Register the NotificationDispatcher class (derived from OsMsgDispatcher)
   // as the sipX notification dispatcher
   mediaInterface->getInterface()->setNotificationDispatcher(mediaInterface.get());

   // Turn on notifications for all resources...
   mediaInterface->getInterface()->setNotificationsEnabled(true);

   // If we have a user agent set DTMF logging now, otherwise wait until setUserAgent is called
   if (getUserAgent())
   {
      // Enable/Disable DTMF digit logging according to UserAgentMasterProfile setting
      mediaInterface->allowLoggingDTMFDigits(getUserAgent()->getUserAgentMasterProfile()->dtmfDigitLoggingEnabled());
   }

   if(giveFocus)
   {
      mediaInterface->getInterface()->giveFocus();
   }

   bridgeMixer = std::make_shared<SipXBridgeMixer>(*(mediaInterface->getInterface()));
}

bool
SipXConversationManager::supportsMultipleMediaInterfaces()
{
   return (getMediaInterfaceMode() == SipXConversationManager::sipXConversationMediaInterfaceMode);
}

bool
SipXConversationManager::canConversationsShareParticipants(Conversation* conversation1, Conversation* conversation2)
{
   assert(conversation1);
   assert(conversation2);

   // You can only add non-local participants to 2 different conversations if they share the same MediaInterface
   return (((SipXConversation*)conversation1)->getMediaInterface().get() == ((SipXConversation*)conversation2)->getMediaInterface().get());
}

Conversation *
SipXConversationManager::createConversationInstance(ConversationHandle handle,
      RelatedConversationSet* relatedConversationSet,  // Pass NULL to create new RelatedConversationSet
      ConversationHandle sharedMediaInterfaceConvHandle,
      ConversationManager::AutoHoldMode autoHoldMode)
{
   return new SipXConversation(handle, *this, relatedConversationSet, sharedMediaInterfaceConvHandle, autoHoldMode);
}

LocalParticipant *
SipXConversationManager::createLocalParticipantInstance(ParticipantHandle partHandle)
{
   return new SipXLocalParticipant(partHandle, *this);
}

MediaResourceParticipant *
SipXConversationManager::createMediaResourceParticipantInstance(ParticipantHandle partHandle, resip::Uri mediaUrl)
{
   return new SipXMediaResourceParticipant(partHandle, *this, mediaUrl);
}

RemoteParticipant *
SipXConversationManager::createRemoteParticipantInstance(DialogUsageManager& dum, RemoteParticipantDialogSet& rpds)
{
   return new SipXRemoteParticipant(*this, dum, rpds);
}

RemoteParticipant *
SipXConversationManager::createRemoteParticipantInstance(ParticipantHandle partHandle, DialogUsageManager& dum, RemoteParticipantDialogSet& rpds)
{
   return new SipXRemoteParticipant(partHandle, *this, dum, rpds);
}

RemoteParticipantDialogSet *
SipXConversationManager::createRemoteParticipantDialogSetInstance(
      ConversationManager::ParticipantForkSelectMode forkSelectMode,
      std::shared_ptr<ConversationProfile> conversationProfile)
{
   return new SipXRemoteParticipantDialogSet(*this, forkSelectMode, conversationProfile);
}

bool
SipXConversationManager::addExtraPlayAndRecordResourcesToTopology()
{
   MpResourceTopology* resourceTopology = mMediaFactory->getInitialResourceTopology();

   // Add in an extra player and recorder
   OsStatus result = resourceTopology->addResource(DEFAULT_FROM_FILE_RESOURCE_TYPE, DEFAULT_FROM_FILE_2_RESOURCE_NAME, MP_INVALID_CONNECTION_ID, -1);
   resip_assert(result == OS_SUCCESS);
   if (result == OS_SUCCESS)
   {
      result = resourceTopology->addResource(DEFAULT_RECORDER_RESOURCE_TYPE, DEFAULT_RECORDER_2_RESOURCE_NAME, MP_INVALID_CONNECTION_ID, -1);
   }
   resip_assert(result == OS_SUCCESS);
   if (result == OS_SUCCESS)
   {
      result = resourceTopology->addConnection(DEFAULT_FROM_FILE_2_RESOURCE_NAME, 0, DEFAULT_BRIDGE_RESOURCE_NAME, MpResourceTopology::MP_TOPOLOGY_NEXT_AVAILABLE_PORT);
   }
   resip_assert(result == OS_SUCCESS);
   if (result == OS_SUCCESS)
   {
      result = resourceTopology->addConnection(DEFAULT_BRIDGE_RESOURCE_NAME, MpResourceTopology::MP_TOPOLOGY_NEXT_AVAILABLE_PORT, DEFAULT_RECORDER_2_RESOURCE_NAME, 0);
   }
   resip_assert(result == OS_SUCCESS);

   //UtlString resourcesDump;
   //resourceTopology->dumpResources(resourcesDump);
   //UtlString connectionsDump;
   //resourceTopology->dumpConnections(connectionsDump);
   //cout << resourcesDump.data();
   //cout << connectionsDump.data();

   return result == OS_SUCCESS;
}

void
SipXConversationManager::initializeDtlsFactory(const resip::Data& defaultAoR)
{
   getFlowManager().initializeDtlsFactory(defaultAoR.c_str());
}


/* ====================================================================

 Copyright (c) 2021, SIP Spectrum, Inc. www.sipspectrum.com
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
