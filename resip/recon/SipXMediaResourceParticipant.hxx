#if !defined(SipXMediaResourceParticipant_hxx)
#define SipXMediaResourceParticipant_hxx

#include <resip/stack/Uri.hxx>

#include "ConversationManager.hxx"
#include "MediaResourceParticipant.hxx"

// sipX includes
#include "mp/MpPlayerListener.h"

class MpStreamPlayer;
class MpPlayerEvent;

namespace recon
{
class ConversationManager;

/**
  This class represents a media resource participant.
  A media resource participant is a representation of the a playing
  audio file or tone.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class SipXMediaResourceParticipant : public MediaResourceParticipant, public MpPlayerListener
{
public:  
   SipXMediaResourceParticipant(ParticipantHandle partHandle,
      ConversationManager& conversationManager,
      const resip::Uri& mediaUrl);  
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
   MpStreamPlayer* mStreamPlayer;
   int mToneGenPortOnBridge;
   int mFromFilePortOnBridge;
#ifndef SIPX_NO_RECORD
   int mRecordPortOnBridge;
#endif

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
