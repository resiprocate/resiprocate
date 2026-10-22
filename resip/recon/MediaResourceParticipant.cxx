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
#include "RemoteParticipant.hxx"

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

// Url schemes
static const Data toneScheme("tone");
static const Data fileScheme("file");
static const Data cacheScheme("cache");
static const Data httpScheme("http");
static const Data httpsScheme("https");
static const Data recordScheme("record");
static const Data recordMultiChannelScheme("record-mc");
static const Data bufferScheme("buffer");

static const resip::ExtensionParameter p_repeat("repeat");
static const resip::ExtensionParameter p_prefetch("prefetch");
static const resip::ExtensionParameter p_startOffset("startoffset");

MediaResourceParticipant::MediaResourceParticipant(ParticipantHandle partHandle,
                                                   ConversationManager& conversationManager,
                                                   const Uri& mediaUrl,
                                                   const std::shared_ptr<Data>& audioBuffer)
: Participant(partHandle, ConversationManager::ParticipantType_MediaResource, conversationManager),
  mMediaUrl(mediaUrl),
  mAudioBuffer(audioBuffer),
  mRepeat(false),
  mPrefetch(false),
  mDurationMs(0),
  mStartOffsetMs(0),
  mRunning(false),
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
      else if (isEqualNoCase(mMediaUrl.scheme(), recordScheme))
      {
         mResourceType = Record;
      }
      else if (isEqualNoCase(mMediaUrl.scheme(), recordMultiChannelScheme))
      {
         mResourceType = RecordMultiChannel;
      }
      else if (isEqualNoCase(mMediaUrl.scheme(), bufferScheme))
      {
         mResourceType = Buffer;
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "MediaResourceParticipant::MediaResourceParticipant exception for url=" << mMediaUrl << ": " << e);
   }
   catch(...)
   {
      WarningLog(<< "MediaResourceParticipant::MediaResourceParticipant unknown exception for url=" << mMediaUrl);
   }
}

MediaResourceParticipant::~MediaResourceParticipant()
{
   InfoLog(<< "MediaResourceParticipant destroyed, handle=" << mHandle << " url=" << mMediaUrl);
}

bool
MediaResourceParticipant::hasInput()
{
   return mResourceType == Record ||
          mResourceType == RecordMultiChannel;
}

bool
MediaResourceParticipant::hasOutput()
{
   return mResourceType == Tone ||
      mResourceType == File ||
      mResourceType == Cache ||
      mResourceType == Http ||
      mResourceType == Https ||
      mResourceType == Buffer;
}

void 
MediaResourceParticipant::startResource()
{
   resip_assert(!mRunning);
   try
   {
      InfoLog(<< "MediaResourceParticipant::startResource, handle=" << mHandle << " url=" << mMediaUrl);

      // Common processing
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
      if (mMediaUrl.exists(p_startOffset))
      {
         mStartOffsetMs = mMediaUrl.param(p_startOffset).convertUnsignedLong();
         mMediaUrl.remove(p_startOffset);
      }

      startResourceImpl();
   }
   catch(BaseException &e)
   {
      WarningLog(<< "MediaResourceParticipant::startResource exception for url=" << mMediaUrl << ": " << e);
   }
   catch(...)
   {
      WarningLog(<< "MediaResourceParticipant::startResource unknown exception for url=" << mMediaUrl);
   }

   if(mRunning)  // If play started successfully
   {
      if(mDurationMs > 0)
      {
         // Start timer to destroy media resource participant automatically
         DestroyParticipantCmd destroyer(&mConversationManager, mHandle);
         mConversationManager.post(destroyer, std::chrono::milliseconds(mDurationMs));
      }
   }
   else
   {
      delete this;
   }
}

void
MediaResourceParticipant::resourceDone()
{
   setRunning(false);
   destroyParticipant(); // may destory this
}

void
MediaResourceParticipant::destroyParticipant()
{
   bool deleteNow = true;

   if (isDestroying()) return;
   setDestroying(true);

   deleteNow = stopResource();
   
   if (deleteNow) delete this;
}



/* ====================================================================

 Copyright (c) 2021-2023, SIP Spectrum, Inc. http://www.sipspectrum.com
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
