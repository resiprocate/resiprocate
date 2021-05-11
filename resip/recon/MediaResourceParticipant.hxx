#if !defined(MediaResourceParticipant_hxx)
#define MediaResourceParticipant_hxx

#include <resip/stack/Uri.hxx>

#include "ConversationManager.hxx"
#include "Participant.hxx"

namespace recon
{
class ConversationManager;

// Used to delete a resource, from a media stack thread
class MediaResourceParticipantDeleterCmd : public resip::DumCommand
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

/**
  This class represents a media resource participant.
  A media resource participant is a representation of the a playing
  audio file or tone.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class MediaResourceParticipant : public Participant
{
public:  
   typedef enum
   {
      Invalid,
      Tone,
      File,
      Cache,
      Http,
      Https,
      Record
   } ResourceType;

   MediaResourceParticipant(ParticipantHandle partHandle,
      ConversationManager& conversationManager,
      const resip::Uri& mediaUrl);  
   virtual ~MediaResourceParticipant();

   virtual void startResource();
   virtual bool hasInput();
   virtual bool hasOutput();
   virtual ResourceType getResourceType() { return mResourceType; }
   virtual void destroyParticipant();
   virtual void resourceDone();

protected:
   virtual void startResourceImpl() = 0;
   virtual bool stopResource() = 0;  // returns true if it's ok to destroy now, or false if we need to wait for an event
   virtual ConversationManager& getConversationManager() { return mConversationManager; }
   virtual resip::Uri& getMediaUrl() { return mMediaUrl; }
   virtual bool isRepeat() { return mRepeat; }
   virtual void setRepeat(bool repeat) { mRepeat = repeat; }
   virtual bool isPrefetch() { return mPrefetch; }
   virtual unsigned int getDurationMs() { return mDurationMs; }
   virtual bool isRunning() { return mRunning; }
   virtual void setRunning(bool running) { mRunning = running; }
   virtual bool isDestroying() { return mDestroying; }
   virtual void setDestroying(bool destroying) { mDestroying = destroying; }


private:
   resip::Uri mMediaUrl;
   ResourceType mResourceType;

   // Play settings
   bool mRepeat;
   bool mPrefetch;
   unsigned int mDurationMs;

   bool mRunning;
   bool mDestroying;
};

}

#endif


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
