#if !defined(SipXMediaResourceParticipant_hxx)
#define SipXMediaResourceParticipant_hxx

#include <resip/stack/Uri.hxx>
#include <rutil/Data.hxx>

#include "ConversationManager.hxx"
#include "MediaResourceParticipant.hxx"
#include "SipXParticipant.hxx"

// sipX includes
#include "mp/MpPlayerListener.h"

class MpStreamPlayer;
class MpPlayerEvent;

// Disable warning 4250
// VS2019 give a 4250 warning:  
// SipXMediaResourceParticipant.hxx(58,1): warning C4250: 'recon::SipXMediaResourceParticipant': inherits 'recon::MediaResourceParticipant::recon::MediaResourceParticipant::hasInput' via dominance
#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4250 )
#endif

namespace recon
{
class ConversationManager;

// Recording Formats
static const resip::Data recordingFormatWAVPCM16("WAV_PCM16");
static const resip::Data recordingFormatWAVMULAW("WAV_MULAW");
static const resip::Data recordingFormatWAVALAW("WAV_ALAW");
static const resip::Data recordingFormatWAVGSM("WAV_GSM");
static const resip::Data recordingFormatOGGOPUS("OGG_OPUS");

// Special Tones
static const resip::Data dialtoneTone("dialtone");
static const resip::Data busyTone("busy");
static const resip::Data ringbackTone("ringback");
static const resip::Data ringTone("ring");
static const resip::Data fastbusyTone("fastbusy");
static const resip::Data backspaceTone("backspace");
static const resip::Data callwaitingTone("callwaiting");
static const resip::Data holdingTone("holding");
static const resip::Data loudfastbusyTone("loudfastbusy");

/**
  This class represents a media resource participant.
  A media resource participant is a representation of the a playing
  audio file or tone.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class SipXMediaResourceParticipant : public virtual MediaResourceParticipant, public virtual SipXParticipant, public MpPlayerListener
{
public:  
   SipXMediaResourceParticipant(ParticipantHandle partHandle,
      ConversationManager& conversationManager,
      SipXMediaStackAdapter& sipXMediaStackAdapter,
      const resip::Uri& mediaUrl,
      const std::shared_ptr<resip::Data>& playAudioBuffer,
      void* recordingCircularBuffer);
   virtual ~SipXMediaResourceParticipant();

   virtual void startResourceImpl();
   virtual bool stopResource();
   virtual int getConnectionPortOnBridge();

   // For Stream Player callbacks
   virtual void playerRealized(MpPlayerEvent& event);
   virtual void playerPrefetched(MpPlayerEvent& event);
   virtual void playerPlaying(MpPlayerEvent& event);
   virtual void playerPaused(MpPlayerEvent& event);
   virtual void playerStopped(MpPlayerEvent& event);
   virtual void playerFailed(MpPlayerEvent& event);

protected:       

private:
   resip::Data mSipXResourceName;
   MpStreamPlayer* mStreamPlayer;
   int mPortOnBridge;
   CircularBufferPtr* mRecordingCircularBuffer;
};

}

#endif


/* ====================================================================

 Copyright (c) 2021-2022, SIP Spectrum, Inc. www.sipspectrum.com
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
