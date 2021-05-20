#include "SipXBridgeMixer.hxx"
#include "ReconSubsystem.hxx"
#include "SipXMediaResourceParticipant.hxx"
#include "SipXConversationManager.hxx"
#include "Conversation.hxx"
#include "UserAgent.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/stack/ExtensionParameter.hxx>
#include <rutil/WinLeakCheck.hxx>

#include "ConversationManagerCmds.hxx"
#include "SipXRemoteParticipant.hxx"

// sipX includes
#include <CpTopologyGraphInterface.h>
#include <mp/dtmflib.h>
#include <mp/MprFromFile.h>
// The ssrc_t typedef from MprEncode.h collides with one from libSrtp - we don't use it in this file, so just undefine it to avoid the compilation error
#undef ssrc_t  
#include <mp/MprEncode.h>
#include <mp/MpStreamPlayer.h>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

static const resip::ExtensionParameter p_participantonly("participant-only");
static const resip::ExtensionParameter p_append("append");
static const resip::ExtensionParameter p_silencetime("silencetime");  // in milliseconds
static const resip::ExtensionParameter p_format("format");


SipXMediaResourceParticipant::SipXMediaResourceParticipant(ParticipantHandle partHandle,
                                                   SipXConversationManager& sipXConversationManager,
                                                   const Uri& mediaUrl)
: Participant(partHandle, sipXConversationManager),
  MediaResourceParticipant(partHandle, sipXConversationManager, mediaUrl),
  SipXParticipant(partHandle, sipXConversationManager),
  mStreamPlayer(0),
  mToneGenPortOnBridge(-1),
  mFromFilePortOnBridge(-1)
#ifndef SIPX_NO_RECORD
  ,
  mRecordPortOnBridge(-1)
#endif
{
   InfoLog(<< "SipXMediaResourceParticipant created, handle=" << mHandle << " url=" << getMediaUrl());
}

SipXMediaResourceParticipant::~SipXMediaResourceParticipant()
{
   // Destroy stream player (if created)
   if(mStreamPlayer)
   {
      mStreamPlayer->removeListener(this);
      mStreamPlayer->destroy();
   }

   // Note:  Ideally this call would exist in the Participant Base class - but this call requires 
   //        dynamic_casts and virtual methods to function correctly during destruction.
   //        If the call is placed in the base Participant class then these things will not
   //        function as desired because a classes type changes as the descructors unwind.
   //        See https://stackoverflow.com/questions/10979250/usage-of-this-in-destructor.
   unregisterFromAllConversations();

   InfoLog(<< "SipXMediaResourceParticipant destroyed, handle=" << mHandle << " url=" << getMediaUrl());
}

void 
SipXMediaResourceParticipant::startResourceImpl()
{
   switch(getResourceType())
   {
   case Tone:
   {
      int toneid;
      bool isDtmf = false;
      if(getMediaUrl().host().size() == 1)
      {
         toneid = getMediaUrl().host().at(0);
         isDtmf = true;
      }
      else
      {
         if(isEqualNoCase(getMediaUrl().host(), dialtoneTone)) toneid = DTMF_TONE_DIALTONE;
         else if(isEqualNoCase(getMediaUrl().host(), busyTone)) toneid = DTMF_TONE_BUSY;
         else if(isEqualNoCase(getMediaUrl().host(), ringbackTone)) toneid = DTMF_TONE_RINGBACK;
         else if(isEqualNoCase(getMediaUrl().host(), ringTone)) toneid = DTMF_TONE_RINGTONE;
         else if(isEqualNoCase(getMediaUrl().host(), fastbusyTone)) toneid = DTMF_TONE_CALLFAILED;
         else if(isEqualNoCase(getMediaUrl().host(), backspaceTone)) toneid = DTMF_TONE_BACKSPACE;
         else if(isEqualNoCase(getMediaUrl().host(), callwaitingTone)) toneid = DTMF_TONE_CALLWAITING;
         else if(isEqualNoCase(getMediaUrl().host(), holdingTone)) toneid = DTMF_TONE_CALLHELD;
         else if(isEqualNoCase(getMediaUrl().host(), loudfastbusyTone)) toneid = DTMF_TONE_LOUD_FAST_BUSY;
         else
         {
            WarningLog(<< "SipXMediaResourceParticipant::startResource invalid tone identifier: " << getMediaUrl().host());
            return;
         }
      }

      OsStatus status = OS_FAILED;
      if(isDtmf && getMediaUrl().exists(p_participantonly))
      {
         ParticipantHandle partHandle = getMediaUrl().param(p_participantonly).convertUnsignedLong();
         SipXRemoteParticipant* participant = dynamic_cast<SipXRemoteParticipant*>(getConversationManager().getParticipant(partHandle));
         if(participant)
         {
            StackLog(<<"SipXMediaResourceParticipant::startResource: sending tone to sipX connection: " << participant->getMediaConnectionId());
            // Start RFC4733 out-of-band tone
            UtlString encodeName(DEFAULT_ENCODE_RESOURCE_NAME);
            MpResourceTopology::replaceNumInName(encodeName, participant->getMediaConnectionId());
            status = MprEncode::startTone(encodeName, *getMediaInterface()->getInterface()->getMsgQ(), toneid);
         }
         else
         {
            WarningLog(<<"SipXMediaResourceParticipant::startResource Participant " << partHandle << " no longer exists or invalid");
         }
      }
      else
      {
         status = getMediaInterface()->getInterface()->startTone(toneid, TRUE /* local - unused */, TRUE /* remote - unused */);
      }
      if(status == OS_SUCCESS)
      {
         setRunning(true);
      }
      else
      {
         WarningLog(<< "SipXMediaResourceParticipant::startResource error calling startTone: " << status);
      }
   }
   break;

   case File:
   {
      Data filepath = getMediaUrl().host().urlDecoded();
      if(filepath.size() > 3 && filepath.substr(0, 3) == Data("///")) filepath = filepath.substr(2);
      else if(filepath.size() > 2 && filepath.substr(0, 2) == Data("//")) filepath = filepath.substr(1);

      filepath.replace("|", ":");  // For Windows filepath processing - convert | to :

      InfoLog(<< "SipXMediaResourceParticipant playing, handle=" << mHandle << " filepath=" << filepath);

      SipXMediaInterface* mediaInterface = getMediaInterface().get();
      OsStatus status = mediaInterface->getInterface()->playAudio(filepath.c_str(),
                                                                  isRepeat() ? TRUE : FALSE /* repeast? */,
                                                                  TRUE /* local - unused */, TRUE /* remote - unused */);

      if(status == OS_SUCCESS)
      {
         // Playing an audio file, generates a finished event on the MediaInterface, set our participant handle
         // as the one that performed the last play media operation, so that MediaInterface can generate the event 
         // to the conversation manage with the correct participant handle.  Note:  this works because sipX
         // only allows a single play from file or cache at a time per media interface.
         mediaInterface->setPlayMediaOperationPartipantHandle(mHandle);
         setRunning(true);
      }
      else
      {
         WarningLog(<< "SipXMediaResourceParticipant::startResource error calling playAudio: " << status);
      }
   }
   break;

   case Cache:
   {
      InfoLog(<< "SipXMediaResourceParticipant playing, handle=" << mHandle << " cacheKey=" << getMediaUrl().host());

      Data *buffer;
      int type;
      if(getConversationManager().getBufferFromMediaResourceCache(getMediaUrl().host(), &buffer, &type))
      {
         SipXMediaInterface* mediaInterface = getMediaInterface().get();
         OsStatus status = mediaInterface->getInterface()->playBuffer((char*)buffer->data(),
                                                                      buffer->size(),
                                                                      8000, /* rate */
                                                                      type,
                                                                      isRepeat() ? TRUE : FALSE /* repeat? */,
                                                                      TRUE /* local - unused */, TRUE /* remote - unused */);
         if(status == OS_SUCCESS)
         {
            // Playing an audio file, generates a finished event on the MediaInterface, set our participant handle
            // as the one that performed the last play media operation, so that MediaInterface can generate the event 
            // to the conversation manage with the correct participant handle.  Note:  this works because sipX
            // only allows a single play from file or cache at a time per media interface.
            mediaInterface->setPlayMediaOperationPartipantHandle(mHandle);
            setRunning(true);
         }
         else
         {
            WarningLog(<< "SipXMediaResourceParticipant::startResource error calling playAudio: " << status);
         }
      }
      else
      {
         WarningLog(<< "SipXMediaResourceParticipant::startResource media not found in cache, key: " << getMediaUrl().host());
      }
   }
   break;
 
   // Warning: The stream player has been deprecated from the SipX CpTopologyGraphInterface - leaving code in place in case it even get's
   //          implemented.  If someone tries to play from Http or Https with sipX it will fail at the createPlayer call below, and
   //          the MediaResourceParticipant will self destruct.
   case Http:
   case Https:
   {
      int flags = STREAM_SOUND_LOCAL | STREAM_SOUND_REMOTE;

      OsStatus status = getMediaInterface()->getInterface()->createPlayer(&mStreamPlayer, Data::from(getMediaUrl()).c_str(), flags);
      if(status == OS_SUCCESS)
      {
         mStreamPlayer->addListener(this);
         status = mStreamPlayer->realize(FALSE /* block? */);
         if(status != OS_SUCCESS)
         {
            WarningLog(<< "SipXMediaResourceParticipant::startResource error calling StreamPlayer::realize: " << status);
         }
         else
         {
            // Playing an audio file, generates a finished event on the MediaInterface, set our participant handle
            // as the one that performed the last play media operation, so that MediaInterface can generate the event 
            // to the conversation manage with the correct participant handle.  Note:  this works because sipX
            // only allows a single play from file or cache at a time per media interface.
            getMediaInterface()->setPlayMediaOperationPartipantHandle(mHandle);
            setRunning(true);
         }
      }
      else
      {
         WarningLog(<< "SipXMediaResourceParticipant::startResource error calling createPlayer: " << status);
      }
   }
   break;

   case Record:
   {
#ifdef SIPX_NO_RECORD
      ErrLog(<< "support for Record was not enabled at compile time");
#else
      Data filepath = getMediaUrl().host().urlDecoded();
      if (filepath.size() > 3 && filepath.substr(0, 3) == Data("///")) filepath = filepath.substr(2);
      else if (filepath.size() > 2 && filepath.substr(0, 2) == Data("//")) filepath = filepath.substr(1);

      filepath.replace("|", ":");  // For Windows filepath processing - convert | to :

      bool append = getMediaUrl().exists(p_append);
      int silenceTimeMs = -1;  // disabled
      if (getMediaUrl().exists(p_silencetime))
      {
         silenceTimeMs = getMediaUrl().param(p_silencetime).convertInt();
      }

      CpMediaInterface::CpAudioFileFormat format = CpMediaInterface::CP_WAVE_PCM_16;  // Default recording format
      Data formatString(recordingFormatWAVPCM16);
      if (getMediaUrl().exists(p_format))
      {
         Data& urlFormatString = getMediaUrl().param(p_format);
         if (isEqualNoCase(urlFormatString, recordingFormatWAVMULAW))
         {
            format = CpMediaInterface::CP_WAVE_MULAW;
            formatString = recordingFormatWAVMULAW;
         }
         else if (isEqualNoCase(urlFormatString, recordingFormatWAVALAW))
         {
            format = CpMediaInterface::CP_WAVE_ALAW;
            formatString = recordingFormatWAVALAW;
         }
         else if (isEqualNoCase(urlFormatString, recordingFormatWAVGSM))
         {
            format = CpMediaInterface::CP_WAVE_GSM;
            formatString = recordingFormatWAVGSM;
         }
         else if (isEqualNoCase(urlFormatString, recordingFormatOGGOPUS))
         {
            format = CpMediaInterface::CP_OGG_OPUS;
            formatString = recordingFormatOGGOPUS;
         }
      }

      InfoLog(<< "SipXMediaResourceParticipant recording, handle=" << mHandle << " filepath=" << filepath << ", format=" << formatString << ", append=" << (append ? "YES" : "NO") << ", maxDurationMs=" << getDurationMs() << ", silenceTimeMs=" << silenceTimeMs);

      SipXMediaInterface* mediaInterface = getMediaInterface().get();

      // TODO - add URL parameter support for selecting between the following file formats: CP_WAVE_PCM_16, CP_WAVE_GSM, CP_OGG_OPUS
      // Note:  locking to single channel recording for now.  Will record all participants in a conversation in a mixed single
      //        channel file.  In order to support multi-channel recording a few things need to happen:
      //        1.  Fix bugs in sipX with multi-channel GSM WAV recording, or disallow.
      //        2.  Fix bugs in sipX with multi-channel OPUS OGG recording, or disallow.
      //        3.  Control mixes with SipXBridgeMixer for multiple recording outputs.  sipX sets up mixes when you start recording
      //            but they are not alterened when additional RTP streams (remote Participants) come and go.
      //        4.  The MAXIMUM_RECORDER_CHANNELS=1 define needs to change in sipXmedaLib and sipXmediaAdpaterLib project files
      // Note: Automatic trimming of silence is not supported for OPUS recordings.
      OsStatus status = mediaInterface->getInterface()->recordChannelAudio(
         0 /* connectionId - not used by sipX */,
         filepath.c_str(), 
         format, 
         append /* append? */,
         1 /* numChannels */, 
         getDurationMs() /* maxTime Ms */,
         silenceTimeMs /* silenceLength Ms, -1 to disable */,
         FALSE /* setupMixesAutomatically? */);
      if (status == OS_SUCCESS)
      {
         // Recoding an audio file, can generate a finished event on the MediaInterface, set our participant handle
         // as the one that performed the last record media operation, so that MediaInterface can generate the event 
         // to the conversation manage with the correct participant handle.  Note:  this works because sipX
         // only allows a single record at a time per media interface.
         mediaInterface->setRecordMediaOperationPartipantHandle(mHandle);
         setRunning(true);
      }
      else
      {
         WarningLog(<< "SipXMediaResourceParticipant::startResource error calling recordChannelAudio: " << status);
      }
#endif
   }
   break;

   case Invalid:
      WarningLog(<< "SipXMediaResourceParticipant::startResource invalid resource type: " << getMediaUrl().scheme());
      break;
   }
}

bool
SipXMediaResourceParticipant::stopResource()
{
   bool okToDeleteNow = true;

   if (isRunning())
   {
      switch (getResourceType())
      {
      case Tone:
      {
         OsStatus status = OS_FAILED;
         if (getMediaUrl().exists(p_participantonly))
         {
            bool isDtmf = (getMediaUrl().host().size() == 1);
            int partHandle = getMediaUrl().param(p_participantonly).convertInt();
            SipXRemoteParticipant* participant = dynamic_cast<SipXRemoteParticipant*>(getConversationManager().getParticipant(partHandle));
            if (isDtmf && participant)
            {
               // Stop RFC4733 out-of-band tone
               UtlString encodeName(DEFAULT_ENCODE_RESOURCE_NAME);
               MpResourceTopology::replaceNumInName(encodeName, participant->getMediaConnectionId());
               status = MprEncode::stopTone(encodeName, *getMediaInterface()->getInterface()->getMsgQ());
            }
            else
            {
               WarningLog(<< "SipXMediaResourceParticipant::stopResource participant " << partHandle << " no longer exists or invalid");
            }
         }
         else
         {
            status = getMediaInterface()->getInterface()->stopTone();
         }
         if (status != OS_SUCCESS)
         {
            WarningLog(<< "SipXMediaResourceParticipant::stopResource error calling stopTone: " << status);
         }
      }
      break;
      case File:
      case Cache:
      {
         OsStatus status = getMediaInterface()->getInterface()->stopAudio();
         if (status != OS_SUCCESS)
         {
            WarningLog(<< "SipXMediaResourceParticipant::stopResource error calling stopAudio: " << status);
         }
      }
      break;
      case Http:
      case Https:
      {
         setRepeat(false);  // Required so that player will not just repeat on stopped event
         OsStatus status = mStreamPlayer->stop();
         if (status != OS_SUCCESS)
         {
            WarningLog(<< "SipXMediaResourceParticipant::stopResource error calling StreamPlayer::stop: " << status);
         }
         else
         {
            okToDeleteNow = false;  // Wait for play finished event to come in
         }
      }
      break;
      case Record:
      {
         OsStatus status = getMediaInterface()->getInterface()->stopRecordChannelAudio(0 /* connectionId - not used by sipX */);
         if (status != OS_SUCCESS)
         {
            WarningLog(<< "SipXMediaResourceParticipant::stopResource error calling stopRecordChannelAudio: " << status);
         }
      }
      break;
      case Invalid:
         WarningLog(<< "SipXMediaResourceParticipant::stopResource invalid resource type: " << getResourceType());
         break;
      }
   }
   return okToDeleteNow;
}

int 
SipXMediaResourceParticipant::getConnectionPortOnBridge()
{
   int connectionPort = -1;
   switch(getResourceType())
   {
   case Tone:     
      if(mToneGenPortOnBridge == -1)
      {
         resip_assert(getMediaInterface() != 0);     
         ((CpTopologyGraphInterface*)getMediaInterface()->getInterface())->getResourceInputPortOnBridge(DEFAULT_TONE_GEN_RESOURCE_NAME,0,mToneGenPortOnBridge);
         InfoLog(<< "SipXMediaResourceParticipant getConnectionPortOnBridge, handle=" << mHandle << ", mToneGenPortOnBridge=" << mToneGenPortOnBridge);
      }
      connectionPort = mToneGenPortOnBridge;
      break;
   case File:
   case Cache:
   case Http:
   case Https:
      if(mFromFilePortOnBridge == -1)
      {
         resip_assert(getMediaInterface() != 0);     
         ((CpTopologyGraphInterface*)getMediaInterface()->getInterface())->getResourceInputPortOnBridge(DEFAULT_FROM_FILE_RESOURCE_NAME,0,mFromFilePortOnBridge);
         InfoLog(<< "SipXMediaResourceParticipant getConnectionPortOnBridge, handle=" << mHandle << ", mFromFilePortOnBridge=" << mFromFilePortOnBridge);
      }
      connectionPort = mFromFilePortOnBridge;
      break;
   case Record:
#ifdef SIPX_NO_RECORD
      ErrLog(<< "support for Record was not enabled at compile time");
#else
      if (mRecordPortOnBridge == -1)
      {
         resip_assert(getMediaInterface() != 0);
         ((CpTopologyGraphInterface*)getMediaInterface()->getInterface())->getResourceOutputPortOnBridge(DEFAULT_RECORDER_RESOURCE_NAME, 0, mRecordPortOnBridge);
         InfoLog(<< "SipXMediaResourceParticipant getConnectionPortOnBridge, handle=" << mHandle << ", mRecordPortOnBridge=" << mRecordPortOnBridge);
      }
      connectionPort = mRecordPortOnBridge;
#endif
      break;
   case Invalid:
      WarningLog(<< "SipXMediaResourceParticipant::getConnectionPortOnBridge invalid resource type: " << getResourceType());
      break;
   }
   return connectionPort;
}

void 
SipXMediaResourceParticipant::playerRealized(MpPlayerEvent& event)
{
   InfoLog(<< "SipXMediaResourceParticipant::playerRealized: handle=" << mHandle);
   if(isPrefetch())
   {
      OsStatus status = mStreamPlayer->prefetch(FALSE);
      if(status != OS_SUCCESS)
      {
         WarningLog(<< "SipXMediaResourceParticipant::playerRealized error calling StreamPlayer::prefetch: " << status);
         MediaResourceParticipantDeleterCmd* cmd = new MediaResourceParticipantDeleterCmd(getConversationManager(), mHandle);
         getConversationManager().post(cmd);
      }
   }
   else
   {
      OsStatus status = mStreamPlayer->play(FALSE /*block?*/);
      if(status != OS_SUCCESS)
      {
         WarningLog(<< "SipXMediaResourceParticipant::playerRealized error calling StreamPlayer::play: " << status);
         MediaResourceParticipantDeleterCmd* cmd = new MediaResourceParticipantDeleterCmd(getConversationManager(), mHandle);
         getConversationManager().post(cmd);
      }
   }
}

void 
SipXMediaResourceParticipant::playerPrefetched(MpPlayerEvent& event)
{
   InfoLog(<< "SipXMediaResourceParticipant::playerPrefetched: handle=" << mHandle);
   OsStatus status = mStreamPlayer->play(FALSE/*block?*/);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "SipXMediaResourceParticipant::playerPrefetched error calling StreamPlayer::play: " << status);
       MediaResourceParticipantDeleterCmd* cmd = new MediaResourceParticipantDeleterCmd(getConversationManager(), mHandle);
       getConversationManager().post(cmd);
   }
}

void 
SipXMediaResourceParticipant::playerPlaying(MpPlayerEvent& event)
{
   InfoLog(<< "SipXMediaResourceParticipant::playerPlaying: handle=" << mHandle);
}

void 
SipXMediaResourceParticipant::playerPaused(MpPlayerEvent& event)
{
   InfoLog(<< "SipXMediaResourceParticipant::playerPaused: handle=" << mHandle);
}

void 
SipXMediaResourceParticipant::playerStopped(MpPlayerEvent& event)
{
   InfoLog(<< "SipXMediaResourceParticipant::playerStopped: handle=" << mHandle);
   // We get this event when playing is completed
   if(isRepeat())
   {
      OsStatus status = mStreamPlayer->rewind(FALSE/*block?*/);   // Generate playerPrefetched event
      if(status != OS_SUCCESS)
      {
         WarningLog(<< "SipXMediaResourceParticipant::playerStopped error calling StreamPlayer::rewind: " << status);
         MediaResourceParticipantDeleterCmd* cmd = new MediaResourceParticipantDeleterCmd(getConversationManager(), mHandle);
         getConversationManager().post(cmd);
      }
   }
   else
   {
      MediaResourceParticipantDeleterCmd* cmd = new MediaResourceParticipantDeleterCmd(getConversationManager(), mHandle);
      getConversationManager().post(cmd);
   }
}
 
void 
SipXMediaResourceParticipant::playerFailed(MpPlayerEvent& event)
{
   InfoLog(<< "SipXMediaResourceParticipant::playerFailed: handle=" << mHandle);
   MediaResourceParticipantDeleterCmd* cmd = new MediaResourceParticipantDeleterCmd(getConversationManager(), mHandle);
   getConversationManager().post(cmd);
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
