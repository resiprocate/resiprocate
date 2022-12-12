#if !defined(SipXLocalParticipant_hxx)
#define SipXLocalParticipant_hxx

#include "SipXMediaStackAdapter.hxx"
#include "LocalParticipant.hxx"
#include "SipXParticipant.hxx"

// Disable warning 4250
// VS2019 give a 4250 warning:  
// SipXLocalParticipant.hxx(36,1): warning C4250: 'recon::SipXLocalParticipant': inherits 'recon::LocalParticipant::recon::LocalParticipant::destroyParticipant' via dominance
#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4250 )
#endif

namespace recon
{
class ConversationManager;

/**
  This class represents a local participant.
  A local participant is a representation of the local source (speaker) 
  and sink (microphone).  The local participant is generally only 
  created once and is added to conversations in which the local speaker 
  and/or microphone should be involved. 

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class SipXLocalParticipant : public virtual LocalParticipant, public virtual SipXParticipant
{
   public:  
      SipXLocalParticipant(ParticipantHandle partHandle,
                       ConversationManager& conversationManager,
                       SipXMediaStackAdapter& sipXMediaStackAdapter);
      virtual ~SipXLocalParticipant();

      virtual int getConnectionPortOnBridge();
      virtual bool hasInput() { return true; }
      virtual bool hasOutput() { return true; }
      virtual void addToConversation(Conversation *conversation, unsigned int inputGain = 100, unsigned int outputGain = 100);

   private:
      int mLocalPortOnBridge;
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
