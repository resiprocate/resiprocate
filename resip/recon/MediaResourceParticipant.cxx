#include "BridgeMixer.hxx"
#include "ReconSubsystem.hxx"
#include "MediaResourceParticipant.hxx"
#include "ConversationManager.hxx"
#include "Conversation.hxx"
#include "UserAgent.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/stack/ExtensionParameter.hxx>
#include <rutil/WinLeakCheck.hxx>

#include "ConversationManagerCmds.hxx"

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

namespace recon
{

// Used to delete a resource, from a sipX thread
class MediaResourceParticipantDeleterCmd : public DumCommand
{
   public:
      MediaResourceParticipantDeleterCmd(ConversationManager& conversationManager, ParticipantHandle participantHandle) :
         mConversationManager(conversationManager), mParticipantHandle(participantHandle) {}
      ~MediaResourceParticipantDeleterCmd() {}

      void executeCommand() { Participant* participant = mConversationManager.getParticipant(mParticipantHandle); if(participant) delete participant; }

      Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << "MediaResourceParticipantDeleterCmd: partHandle=" << mParticipantHandle; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
      
   private:
      ConversationManager& mConversationManager;
      ParticipantHandle mParticipantHandle;
};
}

MediaResourceParticipant::MediaResourceParticipant(ParticipantHandle partHandle,
                                                   ConversationManager& conversationManager,
                                                   const Uri& mediaUrl)
: Participant(partHandle, conversationManager),
  mMediaUrl(mediaUrl),
  mStreamPlayer(0),
  mToneGenPortOnBridge(-1),
  mFromFilePortOnBridge(-1),
  mLocalOnly(false),
  mRemoteOnly(false),
  mRepeat(false),
  mPrefetch(false),
  mDurationMs(0),
  mPlaying(false),
  mDestroying(false)
{
   InfoLog(<< "MediaResourceParticipant created, handle=" << mHandle << " url=" << mMediaUrl);
   mResourceType = Invalid;  // default
   try
   {
      if(isEqualNoCase(mMediaUrl.scheme(), toneScheme))
      {
         mResourceType = Tone;
      }
      else if(isEqualNoCase(mMediaUrl.scheme(), fileScheme))
      {
         mResourceType = File;
      }
      else if(isEqualNoCase(mMediaUrl.scheme(), cacheScheme))
      {
         mResourceType = Cache;
      }
      else if(isEqualNoCase(mMediaUrl.scheme(), httpScheme))
      {
         mResourceType = Http;
      }
      else if(isEqualNoCase(mMediaUrl.scheme(), httpsScheme))
      {
         mResourceType = Https;
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "MediaResourceParticipant::MediaResourceParticipant exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "MediaResourceParticipant::MediaResourceParticipant unknown exception");
   }
}

MediaResourceParticipant::~MediaResourceParticipant()
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

   InfoLog(<< "MediaResourceParticipant destroyed, handle=" << mHandle << " url=" << mMediaUrl);
}

void 
MediaResourceParticipant::startPlay()
{
   resip_assert(!mPlaying);
   try
   {
      InfoLog(<< "MediaResourceParticipant playing, handle=" << mHandle << " url=" << mMediaUrl);

      // Common processing
      if(mMediaUrl.exists(p_localonly))
      {
         mLocalOnly = true;
         mMediaUrl.remove(p_localonly);
      }
      if(mMediaUrl.exists(p_remoteonly))
      {
         mRemoteOnly = true;
         mMediaUrl.remove(p_remoteonly);
      }
      if(mMediaUrl.exists(p_duration))
      {
         mDurationMs = mMediaUrl.param(p_duration);
         mMediaUrl.remove(p_duration);
      }
      if(mMediaUrl.exists(p_repeat))
      {
         mRepeat = true;
         mMediaUrl.remove(p_repeat);
      }
      if(mMediaUrl.exists(p_prefetch))
      {
         mPrefetch = true;
         mMediaUrl.remove(p_prefetch);
      }

      switch(mResourceType)
      {
      case Tone:
      {
         int toneid;
         bool isDtmf = false;
         if(mMediaUrl.host().size() == 1)
         {
            toneid = mMediaUrl.host().at(0);
            isDtmf = true;
         }
         else
         {
            if(isEqualNoCase(mMediaUrl.host(), dialtoneTone)) toneid = DTMF_TONE_DIALTONE;
            else if(isEqualNoCase(mMediaUrl.host(), busyTone)) toneid = DTMF_TONE_BUSY;
            else if(isEqualNoCase(mMediaUrl.host(), ringbackTone)) toneid = DTMF_TONE_RINGBACK;
            else if(isEqualNoCase(mMediaUrl.host(), ringTone)) toneid = DTMF_TONE_RINGTONE;
            else if(isEqualNoCase(mMediaUrl.host(), fastbusyTone)) toneid = DTMF_TONE_CALLFAILED;
            else if(isEqualNoCase(mMediaUrl.host(), backspaceTone)) toneid = DTMF_TONE_BACKSPACE;
            else if(isEqualNoCase(mMediaUrl.host(), callwaitingTone)) toneid = DTMF_TONE_CALLWAITING;
            else if(isEqualNoCase(mMediaUrl.host(), holdingTone)) toneid = DTMF_TONE_CALLHELD;
            else if(isEqualNoCase(mMediaUrl.host(), loudfastbusyTone)) toneid = DTMF_TONE_LOUD_FAST_BUSY;
            else
            {
               WarningLog(<< "MediaResourceParticipant::startPlay invalid tone identifier: " << mMediaUrl.host());
               return;
            }
         }

         OsStatus status = OS_FAILED;
         if(mMediaUrl.exists(p_participantonly))
         {
            int partHandle = mMediaUrl.param(p_participantonly).convertInt();
            RemoteParticipant* participant = dynamic_cast<RemoteParticipant*>(mConversationManager.getParticipant(partHandle));
            if(participant)
            {
               StackLog(<<"sending tone to sipX connection: " << participant->getMediaConnectionId());
               // this uses the original API, where both inband and RFC2833 tones are always sent simultaneously:
               status = getMediaInterface()->getInterface()->startChannelTone(participant->getMediaConnectionId(), toneid, mRemoteOnly ? FALSE : TRUE /* local */, mLocalOnly ? FALSE : TRUE /* remote */);
               // this is for newer sipXtapi API, option to suppress inband tones:
               //status = getMediaInterface()->getInterface()->startChannelTone(participant->getMediaConnectionId(), toneid, mRemoteOnly ? FALSE : TRUE /* local */, mLocalOnly ? FALSE : TRUE /* remote */, !isDtmf /* inband */, true /* RFC 4733 */);
            }
            else
            {
               WarningLog(<<"Participant " << partHandle << " no longer exists or invalid");
            }
         }
         else
         {
            status = getMediaInterface()->getInterface()->startTone(toneid, mRemoteOnly ? FALSE : TRUE /* local */, mLocalOnly ? FALSE : TRUE /* remote */);
         }
         if(status == OS_SUCCESS)
         {
            mPlaying = true;
         }
         else
         {
            WarningLog(<< "MediaResourceParticipant::startPlay error calling startTone: " << status);
         }
      }
      break;
      case File:
      {
         Data filepath = mMediaUrl.host().urlDecoded();
         if(filepath.size() > 3 && filepath.substr(0, 3) == Data("///")) filepath = filepath.substr(2);
         else if(filepath.size() > 2 && filepath.substr(0, 2) == Data("//")) filepath = filepath.substr(1);
         
         filepath.replace("|", ":");  // For Windows filepath processing - convert | to :

         InfoLog(<< "MediaResourceParticipant playing, handle=" << mHandle << " filepath=" << filepath);

         OsStatus status = getMediaInterface()->getInterface()->playAudio(filepath.c_str(), 
                                                          mRepeat ? TRUE: FALSE /* repeast? */,
                                                          mRemoteOnly ? FALSE : TRUE /* local */, 
                                                          mLocalOnly ? FALSE : TRUE /* remote */,
                                                          FALSE /* mixWithMic */,
                                                          100 /* downScaling */);
         if(status == OS_SUCCESS)
         {
            mPlaying = true;
         }
         else
         {
            WarningLog(<< "MediaResourceParticipant::startPlay error calling playAudio: " << status);
         }
      }
      break;
      case Cache:
      {
         InfoLog(<< "MediaResourceParticipant playing, handle=" << mHandle << " cacheKey=" << mMediaUrl.host());

         Data *buffer;
         int type;
         if(mConversationManager.mMediaResourceCache.getFromCache(mMediaUrl.host(), &buffer, &type))
         {
            OsStatus status = getMediaInterface()->getInterface()->playBuffer((char*)buffer->data(),
                                                              buffer->size(), 
                                                              8000, /* rate */
                                                              type, 
                                                              mRepeat ? TRUE: FALSE /* repeast? */,
                                                              mRemoteOnly ? FALSE : TRUE /* local */, 
                                                              mLocalOnly ? FALSE : TRUE /* remote */,
                                                              NULL /* OsProtectedEvent */,
                                                              FALSE /* mixWithMic */,
                                                              100 /* downScaling */);
            if(status == OS_SUCCESS)
            {
               mPlaying = true;
            }
            else
            {
               WarningLog(<< "MediaResourceParticipant::startPlay error calling playAudio: " << status);
            }
         }
         else
         {
            WarningLog(<< "MediaResourceParticipant::startPlay media not found in cache, key: " << mMediaUrl.host());
         }
      }
      break;
      case Http:
      case Https:
      {
         int flags;

         if(mLocalOnly)
         {
            flags = STREAM_SOUND_LOCAL;
         }
         else if(mRemoteOnly)
         {
            flags = STREAM_SOUND_REMOTE;
         }
         else 
         {
            flags = STREAM_SOUND_LOCAL | STREAM_SOUND_REMOTE;
         }
         OsStatus status = getMediaInterface()->getInterface()->createPlayer(&mStreamPlayer, Data::from(mMediaUrl).c_str(), flags);
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
               mPlaying = true;
            }
         }
         else
         {
            WarningLog(<< "MediaResourceParticipant::startPlay error calling createPlayer: " << status);
         }
      }
      break;
      case Invalid:
         WarningLog(<< "MediaResourceParticipant::startPlay invalid resource type: " << mMediaUrl.scheme());
         break;
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "MediaResourceParticipant::startPlay exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "MediaResourceParticipant::startPlay unknown exception");
   }

   if(mPlaying)  // If play started successfully
   {
      if(mDurationMs > 0)
      {
         // Start timer to destroy media resource participant automatically
         DestroyParticipantCmd destroyer(&mConversationManager, mHandle);
         mConversationManager.post(destroyer, mDurationMs);
      }
   }
   else
   {
      delete this;
   }
}

int 
MediaResourceParticipant::getConnectionPortOnBridge()
{
   int connectionPort = -1;
   switch(mResourceType)
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
      WarningLog(<< "MediaResourceParticipant::getConnectionPortOnBridge invalid resource type: " << mResourceType);
      break;
   }
   return connectionPort;
}

void
MediaResourceParticipant::destroyParticipant()
{
   bool deleteNow = true;

   if(mDestroying) return;
   mDestroying = true;

   if(mPlaying)
   {
      switch(mResourceType)
      {
      case Tone:
         {
            OsStatus status = OS_FAILED;
            if(mMediaUrl.exists(p_participantonly))
            {
               bool isDtmf = (mMediaUrl.host().size() == 1);
               int partHandle = mMediaUrl.param(p_participantonly).convertInt();
               RemoteParticipant* participant = dynamic_cast<RemoteParticipant*>(mConversationManager.getParticipant(partHandle));
               if(participant)
               {
                  // this uses the original API, where both inband and RFC2833 tones are always sent simultaneously:
                  status = getMediaInterface()->getInterface()->stopChannelTone(participant->getMediaConnectionId());
                  // this is for newer sipXtapi API, option to suppress inband tones:
                  //status = getMediaInterface()->getInterface()->stopChannelTone(participant->getMediaConnectionId(), !isDtmf, true);
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
            mRepeat = false;  // Required so that player will not just repeat on stopped event
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
         WarningLog(<< "MediaResourceParticipant::destroyParticipant invalid resource type: " << mResourceType);
         break;
      }
   }
   if(deleteNow) delete this;
}

void 
MediaResourceParticipant::playerRealized(MpPlayerEvent& event)
{
   InfoLog(<< "MediaResourceParticipant::playerRealized: handle=" << mHandle);
   if(mPrefetch)
   {
      OsStatus status = mStreamPlayer->prefetch(FALSE);
      if(status != OS_SUCCESS)
      {
         WarningLog(<< "MediaResourceParticipant::playerRealized error calling StreamPlayer::prefetch: " << status);
         MediaResourceParticipantDeleterCmd* cmd = new MediaResourceParticipantDeleterCmd(mConversationManager, mHandle);
         mConversationManager.post(cmd);
      }
   }
   else
   {
      OsStatus status = mStreamPlayer->play(FALSE /*block?*/);
      if(status != OS_SUCCESS)
      {
         WarningLog(<< "MediaResourceParticipant::playerRealized error calling StreamPlayer::play: " << status);
         MediaResourceParticipantDeleterCmd* cmd = new MediaResourceParticipantDeleterCmd(mConversationManager, mHandle);
         mConversationManager.post(cmd);
      }
   }
}

void 
MediaResourceParticipant::playerPrefetched(MpPlayerEvent& event)
{
   InfoLog(<< "MediaResourceParticipant::playerPrefetched: handle=" << mHandle);
   OsStatus status = mStreamPlayer->play(FALSE/*block?*/);
   if(status != OS_SUCCESS)
   {
      WarningLog(<< "MediaResourceParticipant::playerPrefetched error calling StreamPlayer::play: " << status);
       MediaResourceParticipantDeleterCmd* cmd = new MediaResourceParticipantDeleterCmd(mConversationManager, mHandle);
       mConversationManager.post(cmd);
   }
}

void 
MediaResourceParticipant::playerPlaying(MpPlayerEvent& event)
{
   InfoLog(<< "MediaResourceParticipant::playerPlaying: handle=" << mHandle);
}

void 
MediaResourceParticipant::playerPaused(MpPlayerEvent& event)
{
   InfoLog(<< "MediaResourceParticipant::playerPaused: handle=" << mHandle);
}

void 
MediaResourceParticipant::playerStopped(MpPlayerEvent& event)
{
   InfoLog(<< "MediaResourceParticipant::playerStopped: handle=" << mHandle);
   // We get this event when playing is completed
   if(mRepeat)
   {
      OsStatus status = mStreamPlayer->rewind(FALSE/*block?*/);   // Generate playerPrefetched event
      if(status != OS_SUCCESS)
      {
         WarningLog(<< "MediaResourceParticipant::playerStopped error calling StreamPlayer::rewind: " << status);
         MediaResourceParticipantDeleterCmd* cmd = new MediaResourceParticipantDeleterCmd(mConversationManager, mHandle);
         mConversationManager.post(cmd);
      }
   }
   else
   {
      MediaResourceParticipantDeleterCmd* cmd = new MediaResourceParticipantDeleterCmd(mConversationManager, mHandle);
      mConversationManager.post(cmd);
   }
}
 
void 
MediaResourceParticipant::playerFailed(MpPlayerEvent& event)
{
   InfoLog(<< "MediaResourceParticipant::playerFailed: handle=" << mHandle);
   MediaResourceParticipantDeleterCmd* cmd = new MediaResourceParticipantDeleterCmd(mConversationManager, mHandle);
   mConversationManager.post(cmd);
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
