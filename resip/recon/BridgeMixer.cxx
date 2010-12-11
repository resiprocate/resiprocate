#include "BridgeMixer.hxx"
#include "ReconSubsystem.hxx"
#include "Participant.hxx"
#include "RemoteParticipant.hxx"
#include "Conversation.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>

#ifndef max
#define max(x,y) (((x)>(y))?(x):(y))
#endif

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

BridgeMixer::BridgeMixer(CpMediaInterface& mediaInterface) :
   mMediaInterface(mediaInterface)
{
   // Set default to 0 gain for entire matrix
   memset(mMixMatrix, 0, sizeof(mMixMatrix));
}

BridgeMixer::~BridgeMixer()
{
}

void
BridgeMixer::calculateMixWeightsForParticipant(Participant* participant)
{
   int bridgePort = participant->getConnectionPortOnBridge();
   MpBridgeGain inputBridgeWeights[DEFAULT_BRIDGE_MAX_IN_OUTPUTS];

   InfoLog( << "calculatingMixWeigthsForParticipant, handle=" << participant->getParticipantHandle() << ", bridgePort=" << bridgePort);

   if(bridgePort != -1)
   {
      // Default All Inputs/Outputs to 0, then calculate non-zero inputs/outputs
      for(int i = 0; i < DEFAULT_BRIDGE_MAX_IN_OUTPUTS; i++)
      {
         mMixMatrix[i][bridgePort] = 0;
         inputBridgeWeights[i] = 0;
         mMixMatrix[bridgePort][i] = 0;
      }

      // Walk through all Conversations this Participant is in
      const Participant::ConversationMap& partConvs = participant->getConversations();
      
      Participant::ConversationMap::const_iterator it1;
      for(it1 = partConvs.begin(); it1 != partConvs.end(); it1++)
      {
         //InfoLog( << "calculateMixWeightsForParticipant: part=" << participant->getParticipantHandle() << ", conv=" << it1->second->getHandle());
         // Walk through each participant in this Conversation
         Conversation::ParticipantMap& convParts = it1->second->getParticipants();
         Conversation::ParticipantMap::iterator it2;
         it2 = convParts.find(participant->getParticipantHandle());  // Get the participants gain settings in this conversation            
         unsigned int participantOutputGain = 0;
         unsigned int participantInputGain = 0;
         if(it2 != convParts.end()) // if we are in the middle of removing a participant from a conversation then they may not exist in the conversation list any more
         {
            participantOutputGain = it2->second.getOutputGain();
            participantInputGain = it2->second.getInputGain();
         }
         for(it2 = convParts.begin(); it2 != convParts.end(); it2++)
         {
            //InfoLog( << "calculateMixWeightsForParticipant: part=" << participant->getParticipantHandle() << ", conv=" << it1->second->getHandle() << ", part=" << it2->second.getParticipant()->getParticipantHandle());
            // If we found a participant that is not ourself
            if(it2->second.getParticipant()->getParticipantHandle() != participant->getParticipantHandle())
            {
               int otherBridgePort = it2->second.getParticipant()->getConnectionPortOnBridge();
               if(otherBridgePort != -1 && otherBridgePort != bridgePort)  // Note otherBridgePort can equal bridge port if multiple media participants of the same type exist
               {
                  //InfoLog( << "Setting mix level for bridge ports: " << bridgePort << " and " << otherBridgePort);
                  // Calculate the mixed output gain
                  unsigned int outputGain = ((it2->second.getOutputGain() * participantInputGain) / 100) * 10;  // 10 factor is to bring inline with MrpBridgeWeight int type
                  mMixMatrix[bridgePort][otherBridgePort] = max((int)outputGain, mMixMatrix[bridgePort][otherBridgePort]);

                  // Calculate the mixed input gain
                  unsigned int inputGain = ((it2->second.getInputGain() * participantOutputGain) / 100) * 10;  // 10 factor is to bring inline with MrpBridgeWeight int type
                  inputBridgeWeights[otherBridgePort] = mMixMatrix[otherBridgePort][bridgePort] = max((int)inputGain, mMixMatrix[otherBridgePort][bridgePort]);                   
               }
            }
         }
      }

      //outputBridgeMixWeights();

      // Apply new bridge weights
      MprBridge::setMixWeightsForOutput(DEFAULT_BRIDGE_RESOURCE_NAME, *mMediaInterface.getMsgQ(), bridgePort, DEFAULT_BRIDGE_MAX_IN_OUTPUTS, mMixMatrix[bridgePort]);
      MprBridge::setMixWeightsForInput(DEFAULT_BRIDGE_RESOURCE_NAME, *mMediaInterface.getMsgQ(), bridgePort, DEFAULT_BRIDGE_MAX_IN_OUTPUTS, inputBridgeWeights);
   }   
}

void
BridgeMixer::outputBridgeMixWeights()
{
   int i;
   Data data;
   data = " ";
   for(i = 0; i < DEFAULT_BRIDGE_MAX_IN_OUTPUTS; i++)
   {
      if(i < 10)
      {
         data += "   " + Data(i);
      }
      else
      {
         data += "  " + Data(i);
      }
   }
   InfoLog( << data);
   
   data = "-";
   for(i = 0; i < DEFAULT_BRIDGE_MAX_IN_OUTPUTS; i++)
   {
      data += "----";
   }
   InfoLog( << data);

   for(i = 0; i < DEFAULT_BRIDGE_MAX_IN_OUTPUTS; i++)
   {
      if(i < 10)
      {
         data = Data(i) + " |";
      } 
      else
      {
         data = Data(i) + "|";
      }
      for(int j = 0; j < DEFAULT_BRIDGE_MAX_IN_OUTPUTS; j++)
      {
         if(mMixMatrix[i][j]/10 < 10)
         {
            data += " " + Data(mMixMatrix[i][j]/10) + "  ";
         }
         else if(mMixMatrix[i][j]/10 < 100)
         {
            data += Data(mMixMatrix[i][j]/10) + "  ";
         }
         else
         {
            data += Data(mMixMatrix[i][j]/10) + " ";
         }
      }
      InfoLog( << data);
   }
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
