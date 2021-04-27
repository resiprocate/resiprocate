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
#include <mp/MpStreamPlayer.h>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

static const resip::ExtensionParameter p_localonly("local-only");
static const resip::ExtensionParameter p_remoteonly("remote-only");
static const resip::ExtensionParameter p_participantonly("participant-only");
static const resip::ExtensionParameter p_repeat("repeat");
static const resip::ExtensionParameter p_prefetch("prefetch");

// Url schemes
static const Data toneScheme("tone");
static const Data fileScheme("file");
static const Data cacheScheme("cache");
static const Data httpScheme("http");
static const Data httpsScheme("https");

// Special Tones
static const Data dialtoneTone("dialtone");
static const Data busyTone("busy");
static const Data ringbackTone("ringback");
static const Data ringTone("ring");
static const Data fastbusyTone("fastbusy");
static const Data backspaceTone("backspace");
static const Data callwaitingTone("callwaiting");
static const Data holdingTone("holding");
static const Data loudfastbusyTone("loudfastbusy");

SipXMediaResourceParticipant::SipXMediaResourceParticipant(ParticipantHandle partHandle,
                                                   ConversationManager& conversationManager,
                                                   const Uri& mediaUrl)
: MediaResourceParticipant(partHandle, conversationManager, mediaUrl),
  mStreamPlayer(0),
  mToneGenPortOnBridge(-1),
  mFromFilePortOnBridge(-1)
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

   // unregister from Conversations
   // Note:  ideally this functionality would exist in Participant Base class - but dynamic_cast required in unregisterParticipant will not work
   ConversationMap::iterator it;
   for(it = mConversations.begin(); it != mConversations.end(); it++)
   {
      it->second->unregisterParticipant(this);
   }
   mConversations.clear();

   InfoLog(<< "SipXMediaResourceParticipant destroyed, handle=" << mHandle << " url=" << getMediaUrl());
}

void 
SipXMediaResourceParticipant::startPlayImpl()
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
            WarningLog(<< "MediaResourceParticipant::startPlay invalid tone identifier: " << getMediaUrl().host());
            return;
         }
      }

      OsStatus status = OS_FAILED;
      if(getMediaUrl().exists(p_participantonly))
      {
         int partHandle = getMediaUrl().param(p_participantonly).convertInt();
         SipXRemoteParticipant* participant = dynamic_cast<SipXRemoteParticipant*>(getConversationManager().getParticipant(partHandle));
         if(participant)
         {
            StackLog(<<"sending tone to sipX connection: " << participant->getMediaConnectionId());
#ifdef SIPX_TONES_INBAND
            // this uses the original API, where both inband and RFC2833 tones are always sent simultaneously:
            status = getMediaInterface()->getInterface()->startChannelTone(participant->getMediaConnectionId(), toneid, isRemoteOnly() ? FALSE : TRUE /* local */, isLocalOnly() ? FALSE : TRUE /* remote */);
#else
            // this is for newer sipXtapi API, option to suppress inband tones:
            status = getMediaInterface()->getInterface()->startChannelTone(participant->getMediaConnectionId(), toneid, isRemoteOnly() ? FALSE : TRUE /* local */, isLocalOnly() ? FALSE : TRUE /* remote */, !isDtmf /* inband */, true /* RFC 4733 */);
#endif
         }
         else
         {
            WarningLog(<<"Participant " << partHandle << " no longer exists or invalid");
         }
      }
      else
      {
         status = getMediaInterface()->getInterface()->startTone(toneid, isRemoteOnly() ? FALSE : TRUE /* local */, isLocalOnly() ? FALSE : TRUE /* remote */);
      }
      if(status == OS_SUCCESS)
      {
         setPlaying(true);
      }
      else
      {
         WarningLog(<< "MediaResourceParticipant::startPlay error calling startTone: " << status);
      }
   }
   break;
   case File:
   {
      Data filepath = getMediaUrl().host().urlDecoded();
      if(filepath.size() > 3 && filepath.substr(0, 3) == Data("///")) filepath = filepath.substr(2);
      else if(filepath.size() > 2 && filepath.substr(0, 2) == Data("//")) filepath = filepath.substr(1);

      filepath.replace("|", ":");  // For Windows filepath processing - convert | to :

      InfoLog(<< "MediaResourceParticipant playing, handle=" << mHandle << " filepath=" << filepath);

      SipXMediaInterface* mediaInterface = getMediaInterface().get();
      OsStatus status = mediaInterface->getInterface()->playAudio(filepath.c_str(),
                                                       isRepeat() ? TRUE: FALSE /* repeast? */,
                                                       isRemoteOnly() ? FALSE : TRUE /* local */,
                                                       isLocalOnly() ? FALSE : TRUE /* remote */,
                                                       FALSE /* mixWithMic */,
                                                       100 /* downScaling */);
      if(status == OS_SUCCESS)
      {
         // Playing an audio file, generates a finished event on the MediaInterface, set our participant handle
         // as the one that performed the last media operation, so that MediaInterface can generate the event
         // to the conversation manage with the correct participant handle.  Note:  this works because sipX
         // only allows a single play from file or cache at a time per media interface.
         mediaInterface->setMediaOperationPartipantHandle(mHandle);
         setPlaying(true);
      }
      else
      {
         WarningLog(<< "MediaResourceParticipant::startPlay error calling playAudio: " << status);
      }
   }
   break;
   case Cache:
   {
      InfoLog(<< "MediaResourceParticipant playing, handle=" << mHandle << " cacheKey=" << getMediaUrl().host());

      Data *buffer;
      int type;
      if(getConversationManager().mMediaResourceCache.getFromCache(getMediaUrl().host(), &buffer, &type))
      {
         SipXMediaInterface* mediaInterface = getMediaInterface().get();
         OsStatus status = mediaInterface->getInterface()->playBuffer((char*)buffer->data(),
                                                           buffer->size(),
                                                           8000, /* rate */
                                                           type,
                                                           isRepeat() ? TRUE: FALSE /* repeast? */,
                                                           isRemoteOnly() ? FALSE : TRUE /* local */,
                                                           isLocalOnly() ? FALSE : TRUE /* remote */,
                                                           NULL /* OsProtectedEvent */,
                                                           FALSE /* mixWithMic */,
                                                           100 /* downScaling */);
         if(status == OS_SUCCESS)
         {
            // Playing an audio file, generates a finished event on the MediaInterface, set our participant handle
            // as the one that performed the last media operation, so that MediaInterface can generate the event 
            // to the conversation manage with the correct participant handle.  Note:  this works because sipX
            // only allows a single play from file or cache at a time per media interface.
            mediaInterface->setMediaOperationPartipantHandle(mHandle);
            setPlaying(true);
         }
         else
         {
            WarningLog(<< "MediaResourceParticipant::startPlay error calling playAudio: " << status);
         }
      }
      else
      {
         WarningLog(<< "MediaResourceParticipant::startPlay media not found in cache, key: " << getMediaUrl().host());
      }
   }
   break;
   case Http:
   case Https:
   {
      int flags;

      if(isLocalOnly())
      {
         flags = STREAM_SOUND_LOCAL;
      }
      else if(isRemoteOnly())
      {
         flags = STREAM_SOUND_REMOTE;
      }
      else
      {
         flags = STREAM_SOUND_LOCAL | STREAM_SOUND_REMOTE;
      }
      OsStatus status = getMediaInterface()->getInterface()->createPlayer(&mStreamPlayer, Data::from(getMediaUrl()).c_str(), flags);
      if(status == OS_SUCCESS)
      {
         mStreamPlayer->addListener(this);
         status = mStreamPlayer->realize(FALSE /* block? */);
         if(status != OS_SUCCESS)
         {
            WarningLog(<< "MediaResourceParticipant::startPlay error calling StreamPlayer::realize: " << status);
         }
         else
         {
            setPlaying(true);
         }
      }
      else
      {
         WarningLog(<< "MediaResourceParticipant::startPlay error calling createPlayer: " << status);
      }
   }
   break;
   case Invalid:
      WarningLog(<< "MediaResourceParticipant::startPlay invalid resource type: " << getMediaUrl().scheme());
      break;
   }
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
         InfoLog(<< "MediaResourceParticipant getConnectionPortOnBridge, handle=" << mHandle << ", mToneGenPortOnBridge=" << mToneGenPortOnBridge);
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
         InfoLog(<< "MediaResourceParticipant getConnectionPortOnBridge, handle=" << mHandle << ", mFromFilePortOnBridge=" << mFromFilePortOnBridge);
      }
      connectionPort = mFromFilePortOnBridge;
      break;
   case Invalid:
      WarningLog(<< "MediaResourceParticipant::getConnectionPortOnBridge invalid resource type: " << getResourceType());
      break;
   }
   return connectionPort;
}

void
SipXMediaResourceParticipant::destroyParticipant()
{
   bool deleteNow = true;

   if(isDestroying()) return;
   setDestroying(true);

   if(isPlaying())
   {
      switch(getResourceType())
      {
      case Tone:
         {
            OsStatus status = OS_FAILED;
            if(getMediaUrl().exists(p_participantonly))
            {
               bool isDtmf = (getMediaUrl().host().size() == 1);
               int partHandle = getMediaUrl().param(p_participantonly).convertInt();
               SipXRemoteParticipant* participant = dynamic_cast<SipXRemoteParticipant*>(getConversationManager().getParticipant(partHandle));
               if(participant)
               {
#ifdef SIPX_TONES_INBAND
                  // this uses the original API, where both inband and RFC2833 tones are always sent simultaneously:
                  status = getMediaInterface()->getInterface()->stopChannelTone(participant->getMediaConnectionId());
#else
                  // this is for newer sipXtapi API, option to suppress inband tones:
                  status = getMediaInterface()->getInterface()->stopChannelTone(participant->getMediaConnectionId(), !isDtmf, true);
#endif
               }
               else
               {
                  WarningLog(<<"Participant " << partHandle << " no longer exists or invalid");
               }
            }
            else
            {
               status = getMediaInterface()->getInterface()->stopTone();
            }
            if(status != OS_SUCCESS)
            {
               WarningLog(<< "MediaResourceParticipant::destroyParticipant error calling stopTone: " << status);
            }
         }
         break;
      case File:
      case Cache:
         {
            OsStatus status = getMediaInterface()->getInterface()->stopAudio();
            if(status != OS_SUCCESS)
            {
               WarningLog(<< "MediaResourceParticipant::destroyParticipant error calling stopAudio: " << status);
            }
         }
         break;
      case Http:
      case Https:
         {
            setRepeat(false);  // Required so that player will not just repeat on stopped event
            OsStatus status = mStreamPlayer->stop();
            if(status != OS_SUCCESS)
            {
               WarningLog(<< "MediaResourceParticipant::destroyParticipant error calling StreamPlayer::stop: " << status);
            }
            else
            {
               deleteNow = false;  // Wait for play finished event to come in
            }
         }
         break;
      case Invalid:
         WarningLog(<< "MediaResourceParticipant::destroyParticipant invalid resource type: " << getResourceType());
         break;
      }
   }
   if(deleteNow) delete this;
}

void 
SipXMediaResourceParticipant::playerRealized(MpPlayerEvent& event)
{
   InfoLog(<< "MediaResourceParticipant::playerRealized: handle=" << mHandle);
   if(isPrefetch())
   {
      OsStatus status = mStreamPlayer->prefetch(FALSE);
      if(status != OS_SUCCESS)
      {
         WarningLog(<< "MediaResourceParticipant::playerRealized error calling StreamPlayer::prefetch: " << status);
         MediaResourceParticipantDeleterCmd* cmd = new MediaResourceParticipantDeleterCmd(getConversationManager(), mHandle);
         getConversationManager().post(cmd);
      }
   }
   else
   {
      OsStatus status = mStreamPlayer->play(FALSE /*block?*/);
      if(status != OS_SUCCESS)
      {
         WarningLog(<< "MediaResourceParticipant::playerRealized error calling StreamPlayer::play: " << status);
         MediaResourceParticipantDeleterCmd* cmd = new MediaResourceParticipantDeleterCmd(getConversationManager(), mHandle);
         getConversationManager().post(cmd);
      }
   }
}

void 
SipXMediaResourceParticipant::playerPrefetched(MpPlayerEvent& event)
{
   InfoLog(<< "MediaResourceParticipant::playerPrefetched: handle=" << mHandle);
   OsStatus status = mStreamPlayer->play(FALSE/*block?*/);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "MediaResourceParticipant::playerPrefetched error calling StreamPlayer::play: " << status);
       MediaResourceParticipantDeleterCmd* cmd = new MediaResourceParticipantDeleterCmd(getConversationManager(), mHandle);
       getConversationManager().post(cmd);
   }
}

void 
SipXMediaResourceParticipant::playerPlaying(MpPlayerEvent& event)
{
   InfoLog(<< "MediaResourceParticipant::playerPlaying: handle=" << mHandle);
}

void 
SipXMediaResourceParticipant::playerPaused(MpPlayerEvent& event)
{
   InfoLog(<< "MediaResourceParticipant::playerPaused: handle=" << mHandle);
}

void 
SipXMediaResourceParticipant::playerStopped(MpPlayerEvent& event)
{
   InfoLog(<< "MediaResourceParticipant::playerStopped: handle=" << mHandle);
   // We get this event when playing is completed
   if(isRepeat())
   {
      OsStatus status = mStreamPlayer->rewind(FALSE/*block?*/);   // Generate playerPrefetched event
      if(status != OS_SUCCESS)
      {
         WarningLog(<< "MediaResourceParticipant::playerStopped error calling StreamPlayer::rewind: " << status);
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
   InfoLog(<< "MediaResourceParticipant::playerFailed: handle=" << mHandle);
   MediaResourceParticipantDeleterCmd* cmd = new MediaResourceParticipantDeleterCmd(getConversationManager(), mHandle);
   getConversationManager().post(cmd);
}


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
