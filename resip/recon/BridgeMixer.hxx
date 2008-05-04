#if !defined(BridgeMixer_hxx)
#define BridgeMixer_hxx

#include <mp/MprBridge.h>
#include <mp/MpResourceTopology.h>

namespace useragent
{

// These must align with sipX implementation
#define DEFAULT_LOCAL_RESOURCE_BRIDGE_CONNECTION_PORT     0
#define DEFAULT_FILE_PLAYER_BRIDGE_CONNECTION_PORT        1
#define DEFAULT_TONE_PLAYER_BRIDGE_CONNECTION_PORT        2

class ConversationManager;
class Participant;

/**
  This class is used to control the sipX Bridge Mixer mix matrix.

  If there is ever a change required in the mixer bridge, the
  application should call calculateMixWeightsForParticipant
  in order to have the changes detected and applied.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class BridgeMixer
{
public:  
   /**
     Constructor

     @param conversationManager required for access to the sipX 
     mediaInterface
   */
   BridgeMixer(ConversationManager& conversationManager);  
   virtual ~BridgeMixer();

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
   MpBridgeGain mMixMatrix[DEFAULT_BRIDGE_MAX_IN_OUTPUTS][DEFAULT_BRIDGE_MAX_IN_OUTPUTS];
   ConversationManager& mConversationManager;
};

}

#endif


/* ====================================================================

 Original contribution Copyright (C) 2008 Plantronics, Inc.
 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */
