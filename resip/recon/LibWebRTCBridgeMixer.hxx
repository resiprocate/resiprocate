#if !defined(LibWebRTCBridgeMixer_hxx)
#define LibWebRTCBridgeMixer_hxx

#if (_MSC_VER >= 1600)
#include <stdint.h>       // Use Visual Studio's stdint.h
#define _MSC_STDINT_H_    // This define will ensure that stdint.h in sipXport tree is not used
#endif

#include <cstdint>

#include "BridgeMixer.hxx"

namespace recon
{
class ConversationManager;
class Participant;

/**
  This class is used to control the sipX Bridge Mixer mix matrix.

  If there is ever a change required in the mixer bridge, the
  application should call calculateMixWeightsForParticipant
  in order to have the changes detected and applied.
*/

class LibWebRTCBridgeMixer : public BridgeMixer
{
public:  
   /**
     Constructor

     @param mediaInterface
   */
   LibWebRTCBridgeMixer();
   virtual ~LibWebRTCBridgeMixer();

   /**
     Calculates all of the current mixer settings required 
     for the passed in participant and applies them.  
     Calculations are based off of Participants membership
     into Conversations and the input/output gain settings.

     @param participant Participant to calculate mixer weights for
   */
   void calculateMixWeightsForParticipant(Participant* participant);

   /**
     Logs a multiline representation of the current state
     of the mixing matrix.
   */
   void outputBridgeMixWeights();

private:
   typedef int16_t LibWebRTCBridgeGain;
   LibWebRTCBridgeGain mMixMatrix[DEFAULT_BRIDGE_MAX_IN_OUTPUTS][DEFAULT_BRIDGE_MAX_IN_OUTPUTS];
};

}

#endif


/* ====================================================================

 Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2022, Daniel Pocock https://danielpocock.com
 Copyright (c) 2021, SIP Spectrum, Inc. www.sipspectrum.com
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
