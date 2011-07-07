#include "BridgeMixer.hxx"
#include "ReconSubsystem.hxx"
#include "MediaResourceParticipant.hxx"
#include "ConversationManager.hxx"
#include "Conversation.hxx"
#include "UserAgent.hxx"
#include "media/Mixer.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/stack/ExtensionParameter.hxx>
#include <rutil/WinLeakCheck.hxx>

#include "ConversationManagerCmds.hxx"

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

static const resip::ExtensionParameter p_local("local");
static const resip::ExtensionParameter p_remote("remote");
static const resip::ExtensionParameter p_oob("oob");  // should we send out-of-band?
static const resip::ExtensionParameter p_inband("inband");
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

      Message* clone() const { assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << "MediaResourceParticipantDeleterCmd: partHandle=" << mParticipantHandle; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
      
   private:
      ConversationManager& mConversationManager;
      ParticipantHandle mParticipantHandle;
};
}

MediaResourceParticipant::MediaResourceParticipant(ParticipantHandle partHandle,
                                                   ConversationManager& conversationManager,
                                                   Uri& mediaUrl)
: Participant(partHandle, conversationManager),
  mMediaUrl(mediaUrl),
  mLocal(false),
  mRemote(false),
  mInBand(false),
  mOutOfBand(false),
  mRepeat(false),
  mPrefetch(false),
  mDurationMs(0),
  mPlaying(false),
  mDestroying(false),
  mToneGenPortOnBridge(-1),
  mFromFilePortOnBridge(-1)
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

   // !jjg! fixme
   //((CpTopologyGraphInterface*)mConversationManager.getMediaInterface())->getResourceInputPortOnBridge(DEFAULT_TONE_GEN_RESOURCE_NAME,0,mToneGenPortOnBridge);
   //((CpTopologyGraphInterface*)mConversationManager.getMediaInterface())->getResourceInputPortOnBridge(DEFAULT_FROM_FILE_RESOURCE_NAME,0,mFromFilePortOnBridge);
}

MediaResourceParticipant::~MediaResourceParticipant()
{
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
   assert(!mPlaying);
   try
   {
      InfoLog(<< "MediaResourceParticipant playing, handle=" << mHandle << " url=" << mMediaUrl);

      // Common processing
      if(mMediaUrl.exists(p_local))
      {
         mLocal = true;
         mMediaUrl.remove(p_local);
      }
      if(mMediaUrl.exists(p_remote))
      {
         mRemote = true;
         mMediaUrl.remove(p_remote);
      }
      if(mMediaUrl.exists(p_oob))
      {
         mOutOfBand = true;
         mMediaUrl.remove(p_oob);
      }
      if(mMediaUrl.exists(p_inband))
      {
         mInBand = true;
         mMediaUrl.remove(p_inband);
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
         // !jjg! fixme
         int toneid = -1;
         if(mMediaUrl.host().size() == 1)
         {
            if (resip::isEqualNoCase(mMediaUrl.host(),"*"))
            {
               toneid = 10;
            }
            else if (resip::isEqualNoCase(mMediaUrl.host(),"#"))
            {
               toneid = 11;
            }
            else if (resip::isEqualNoCase(mMediaUrl.host(),"A"))
            {
               toneid = 12;
            }
            else if (resip::isEqualNoCase(mMediaUrl.host(),"B"))
            {
               toneid = 13;
            }
            else if (resip::isEqualNoCase(mMediaUrl.host(),"C"))
            {
               toneid = 14;
            }
            else if (resip::isEqualNoCase(mMediaUrl.host(),"D"))
            {
               toneid = 15;
            }
            else
            {
               char c = mMediaUrl.host().at(0);
               if (c >= 48 && c <= 57)
               {
                  toneid = mMediaUrl.host().convertInt();
               }
            }
         }
         //else
         //{
         //   if(isEqualNoCase(mMediaUrl.host(), dialtoneTone)) toneid = DTMF_TONE_DIALTONE;
         //   else if(isEqualNoCase(mMediaUrl.host(), busyTone)) toneid = DTMF_TONE_BUSY;
         //   else if(isEqualNoCase(mMediaUrl.host(), ringbackTone)) toneid = DTMF_TONE_RINGBACK;
         //   else if(isEqualNoCase(mMediaUrl.host(), ringTone)) toneid = DTMF_TONE_RINGTONE;
         //   else if(isEqualNoCase(mMediaUrl.host(), fastbusyTone)) toneid = DTMF_TONE_CALLFAILED;
         //   else if(isEqualNoCase(mMediaUrl.host(), backspaceTone)) toneid = DTMF_TONE_BACKSPACE;
         //   else if(isEqualNoCase(mMediaUrl.host(), callwaitingTone)) toneid = DTMF_TONE_CALLWAITING;
         //   else if(isEqualNoCase(mMediaUrl.host(), holdingTone)) toneid = DTMF_TONE_CALLHELD;
         //   else if(isEqualNoCase(mMediaUrl.host(), loudfastbusyTone)) toneid = DTMF_TONE_LOUD_FAST_BUSY;
         else
         {
            WarningLog(<< "MediaResourceParticipant::startPlay invalid tone identifier: " << mMediaUrl.host());
            return;
         }
         //}
         if (toneid == -1)
         {
            WarningLog(<< "MediaResourceParticipant::startPlay invalid tone identifier: " << mMediaUrl.host());
            return;
         }

         ConversationMap::const_iterator convIter = getConversations().begin();
         for (; convIter != getConversations().end(); ++convIter)
         {
            Conversation* conv = convIter->second;
            const Mixer::RtpStreams& streams = conv->getMixer()->rtpStreams();
            Mixer::RtpStreams::const_iterator it = streams.begin();
            for (; it != streams.end(); ++it)
            {
               const boost::shared_ptr<RtpStream>& stream = *it;
               if (stream->mediaType() == MediaStack::MediaType_Audio)
               {
                  stream->playTone(toneid, mLocal, mRemote, mInBand, mOutOfBand);
               }
            }
         }
         mPlaying = true;
      }
      break;
      case File:
      {
         // Assume that the file:// URL will point to a single file which can be rendered using platform
         // specific tools etc. i.e. it will either be an audio file like a WAV, MP3, OGG, etc. Or otherwise
         // it will point to a media container format like AVI, MKV etc, which might contain video.
         // It is up to the concrete implementation of RtpStream to decide what is supported and how to
         // parse it, etc.
         //
         // If the file contains video, then video should also be sent in the RTP stream.
         //
         // recon divides its RtpStreams along media type lines. Therefore the file will be "played"
         // on all video or audio streams at the same time. Note that this approach might lose lip-sync
         // for the time being.
         //
         Data filePath = mMediaUrl.host().urlDecoded();
         if( filePath.size() > 2 && filePath.substr( 0, 2 ) == Data( "//" ))
            filePath = filePath.substr( 2 ); // Remove the "//" from the front
         
#ifdef WIN32
         filePath.replace("|", ":");  // For Windows filePath processing - convert | to :
         filePath.replace("/", "\\"); // Also replace "/" with "\"
#endif

         InfoLog(<< "MediaResourceParticipant playing, handle=" << mHandle << " filePath=" << filePath);

         ConversationMap::const_iterator convIter = getConversations().begin();
         for (; convIter != getConversations().end(); ++convIter)
         {
            Conversation* conv = convIter->second;
            const Mixer::RtpStreams& streams = conv->getMixer()->rtpStreams();
            Mixer::RtpStreams::const_iterator it = streams.begin();
            for (; it != streams.end(); ++it)
            {
               const boost::shared_ptr<RtpStream>& stream = *it;
               MediaStack::MediaType mtype = stream->mediaType();
               if( mtype == MediaStack::MediaType_Audio || mtype == MediaStack::MediaType_Video )
               {
                  // NB: file might not be appropriate, it's up to the stream
                  // to figure out whether there is audio or video which can
                  // be played.
                  stream->playFile( filePath, mRepeat );
               }
            }
         }
         mPlaying = true;
      }
      break;
      case Cache:
      {
         InfoLog(<< "MediaResourceParticipant playing, handle=" << mHandle << " cacheKey=" << mMediaUrl.host());

			// !jjg! fixme
         //Data *buffer;
         //int type;
         //if(mConversationManager.mMediaResourceCache.getFromCache(mMediaUrl.host(), &buffer, &type))
         //{
         //   OsStatus status = mConversationManager.getMediaInterface()->playBuffer((char*)buffer->data(),
         //                                                                          buffer->size(), 
         //                                                                          8000, /* rate */
         //                                                                          type, 
         //                                                                          mRepeat ? TRUE: FALSE /* repeast? */,
         //                                                                          mRemoteOnly ? FALSE : TRUE /* local */, 
         //                                                                          mLocalOnly ? FALSE : TRUE /* remote */,
         //                                                                          NULL /* OsProtectedEvent */,
         //                                                                          FALSE /* mixWithMic */,
         //                                                                          100 /* downScaling */);
         //   if(status == OS_SUCCESS)
         //   {
         //      mPlaying = true;
         //   }
         //   else
         //   {
         //      WarningLog(<< "MediaResourceParticipant::startPlay error calling playAudio: " << status);
         //   }
         //}
         //else
         //{
         //   WarningLog(<< "MediaResourceParticipant::startPlay media not found in cache, key: " << mMediaUrl.host());
         //}
      }
      break;
      case Http:
      case Https:
      {
         // !jjg! fixme
         //int flags;

         //if(mLocalOnly)
         //{
         //   flags = STREAM_SOUND_LOCAL;
         //}
         //else if(mRemoteOnly)
         //{
         //   flags = STREAM_SOUND_REMOTE;
         //}
         //else 
         //{
         //   flags = STREAM_SOUND_LOCAL | STREAM_SOUND_REMOTE;
         //}
         //OsStatus status = mConversationManager.getMediaInterface()->createPlayer(&mStreamPlayer, Data::from(mMediaUrl).c_str(), flags);
         //if(status == OS_SUCCESS)
         //{
         //   mStreamPlayer->addListener(this);
         //   status = mStreamPlayer->realize(FALSE /* block? */);
         //   if(status != OS_SUCCESS)
         //   {
         //      WarningLog(<< "MediaResourceParticipant::startPlay error calling StreamPlayer::realize: " << status);
         //   }
         //   else
         //   {
         //      mPlaying = true;
         //   }
         //}
         //else
         //{
         //   WarningLog(<< "MediaResourceParticipant::startPlay error calling createPlayer: " << status);
         //}
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
         mConversationManager.mDum->getSipStack().postMS( destroyer, mDurationMs, mConversationManager.mDum );
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
      connectionPort = mToneGenPortOnBridge;
      break;
   case File:
   case Cache:
   case Http:
   case Https:
      connectionPort = mFromFilePortOnBridge;
      break;
   case Invalid:
      WarningLog(<< "MediaResourceParticipant::getConnectionPortOnBridge invalid resource type: " << mResourceType);
      break;
   }
   return connectionPort;
}

void
MediaResourceParticipant::destroyParticipant(const resip::Data&)
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
            //OsStatus status = mConversationManager.getMediaInterface()->stopTone();
            //if(status != OS_SUCCESS)
            //{
            //   WarningLog(<< "MediaResourceParticipant::destroyParticipant error calling stopTone: " << status);
            //}

            ConversationMap::const_iterator convIter = getConversations().begin();
            for (; convIter != getConversations().end(); ++convIter)
            {
               Conversation* conv = convIter->second;
               const Mixer::RtpStreams& streams = conv->getMixer()->rtpStreams();
               Mixer::RtpStreams::const_iterator it = streams.begin();
               for (; it != streams.end(); ++it)
               {
                  const boost::shared_ptr<RtpStream>& stream = *it;
                  if (stream->mediaType() == MediaStack::MediaType_Audio)
                  {
                     stream->stopTone();
                  }
               }
            }
         }
         break;
      case File:
         {
            ConversationMap::const_iterator convIter = getConversations().begin();
            for (; convIter != getConversations().end(); ++convIter)
            {
               Conversation* conv = convIter->second;
               const Mixer::RtpStreams& streams = conv->getMixer()->rtpStreams();
               Mixer::RtpStreams::const_iterator it = streams.begin();
               for (; it != streams.end(); ++it)
               {
                  const boost::shared_ptr<RtpStream>& stream = *it;
                  MediaStack::MediaType mtype( stream->mediaType() );

                  if( mtype == MediaStack::MediaType_Audio || mtype == MediaStack::MediaType_Video )
                     stream->stopFile();
               }
            }
         }
         break;
      case Cache:
         {
            //OsStatus status = mConversationManager.getMediaInterface()->stopAudio();
            //if(status != OS_SUCCESS)
            //{
            //   WarningLog(<< "MediaResourceParticipant::destroyParticipant error calling stopAudio: " << status);
            //}
         }
         break;
      case Http:
      case Https:
         {
            //mRepeat = false;  // Required so that player will not just repeat on stopped event
            //OsStatus status = mStreamPlayer->stop();
            //if(status != OS_SUCCESS)
            //{
            //   WarningLog(<< "MediaResourceParticipant::destroyParticipant error calling StreamPlayer::stop: " << status);
            //}
            //else
            //{
            //   deleteNow = false;  // Wait for play finished event to come in
            //}
         }
         break;
      case Invalid:
         WarningLog(<< "MediaResourceParticipant::destroyParticipant invalid resource type: " << mResourceType);
         break;
      }
   }
   if(deleteNow) delete this;
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
