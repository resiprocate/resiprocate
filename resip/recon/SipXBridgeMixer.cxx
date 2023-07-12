#include "SipXBridgeMixer.hxx"
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

SipXBridgeMixer::SipXBridgeMixer(CpTopologyGraphInterface& mediaInterface) :
   mMediaInterface(mediaInterface)
{
   // Set default to 0 gain for entire matrix
   memset(mMixMatrix, 0, sizeof(mMixMatrix));
}

SipXBridgeMixer::~SipXBridgeMixer()
{
}

void
SipXBridgeMixer::calculateMixWeightsForParticipant(Participant* participant)
{
   int bridgePort = participant->getConnectionPortOnBridge();
   MpBridgeGain inputBridgeWeights[DEFAULT_BRIDGE_MAX_IN_OUTPUTS];

   InfoLog( << "calculatingMixWeigthsForParticipant, handle=" << participant->getParticipantHandle() << ", bridgePort=" << bridgePort << ", hasInput=" << participant->hasInput() << ", hasOutput=" << participant->hasOutput());

   if(bridgePort != -1)
   {
      bool processingMultiChannelRecorder = false;
      if (participant->hasInput() && bridgePort == 0)
      {
         // If input bridgePort is 0, then this is the multichannel recorder.
         processingMultiChannelRecorder = true;
      }
      // Default All Inputs/Outputs to 0, then calculate non-zero inputs/outputs
      for(int i = 0; i < DEFAULT_BRIDGE_MAX_IN_OUTPUTS; i++)
      {
         // If we have input capabilities, then clear out other participants outputs to us
         if (participant->hasInput())
         {
            mMixMatrix[bridgePort][i] = 0;
            if (processingMultiChannelRecorder)
            {
               // BridgePort 0 is for the left channel.  BridgePort 1 is for the right channel.
               // Note:  getConnectionPortOnBridge always only returns BridgePort 0 for the 
               //        multichannel recorder.
               mMixMatrix[1][i] = 0; // Clear out right channel (bridge port 1)
            }
         }
         // If we have output capabilites, then clear out other participants inputs from us
         if (participant->hasOutput())
         {
            mMixMatrix[i][bridgePort] = 0;
         }
         inputBridgeWeights[i] = 0;
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
            //InfoLog( << "calculateMixWeightsForParticipant: part=" << participant->getParticipantHandle() << ", conv=" << it1->second->getHandle() << ", otherpart=" << it2->second.getParticipant()->getParticipantHandle() << ", hasInput=" << it2->second.getParticipant()->hasInput() << ", hasOutput=" << it2->second.getParticipant()->hasOutput());
            // If we found a participant that is not ourself
            if(it2->second.getParticipant()->getParticipantHandle() != participant->getParticipantHandle())
            {
               int otherBridgePort = it2->second.getParticipant()->getConnectionPortOnBridge();
               if (otherBridgePort != -1)
               {
                  int bridgePortToUse = bridgePort;
                  if (participant->hasInput() &&                  // Only look at outputs from other participants if we have an input,  and
                      it2->second.getParticipant()->hasOutput())  // Only look at other participant if it has an output
                  {
                     // Calculate the mixed output gain from the other participant to us
                     unsigned int outputGain = ((it2->second.getOutputGain() * participantInputGain) / 100) * 10;  // 10 factor is to bring inline with MrpBridgeWeight int type
                     if (processingMultiChannelRecorder)
                     {
                        // If other party is supposed to record to channel 2 then update 
                        // bridgePortToUse from 0 to 1
                        if (it2->second.getParticipant()->getRecordChannelNum() == 2)
                        {
                           // Send audio to right channel
                           bridgePortToUse = 1;
                        }
                     }
                     mMixMatrix[bridgePortToUse][otherBridgePort] = max((int)outputGain, mMixMatrix[bridgePortToUse][otherBridgePort]);
                  }

                  if (participant->hasOutput() &&                // Only look at inputs to other participants if we have an output, and
                      it2->second.getParticipant()->hasInput())  // Only look at other participants that have an input and can accept our output
                  {
                     // Calculate the mixed input gain for the other participant
                     unsigned int inputGain = ((it2->second.getInputGain() * participantOutputGain) / 100) * 10;  // 10 factor is to bring inline with MrpBridgeWeight int type

                     // OtherParty has input and is bridgePort is 0, then other party is the multichannel recorder.
                     if (otherBridgePort == 0)
                     {
                        // If participant in question needs channel 2 recording then change otherBridgePort from 0 to 1.
                        if (participant->getRecordChannelNum() == 2)
                        {
                           // Send audio to right channel
                           otherBridgePort = 1;
                        }
                     }
                     inputBridgeWeights[otherBridgePort] = mMixMatrix[otherBridgePort][bridgePortToUse] = max((int)inputGain, mMixMatrix[otherBridgePort][bridgePortToUse]);
                  }
                  //InfoLog(<< "Setting mix level for bridge ports: " << bridgePortToUse << " and " << otherBridgePort << ": outputGain=" << mMixMatrix[bridgePortToUse][otherBridgePort]/10 << ", inputGain=" << inputBridgeWeights[otherBridgePort]/10);
               }
            }
         }
      }

      //outputBridgeMixWeights();

      // Apply new bridge weights
      if (participant->hasInput())
      {
         // Apply mix outputs for other participants if we have an input
         MprBridge::setMixWeightsForOutput(DEFAULT_BRIDGE_RESOURCE_NAME, *mMediaInterface.getMsgQ(), bridgePort, DEFAULT_BRIDGE_MAX_IN_OUTPUTS, mMixMatrix[bridgePort]);
         if (processingMultiChannelRecorder)
         {
            // If processing the multichannel recorder than also update the secondary/right channel
            MprBridge::setMixWeightsForOutput(DEFAULT_BRIDGE_RESOURCE_NAME, *mMediaInterface.getMsgQ(), 1, DEFAULT_BRIDGE_MAX_IN_OUTPUTS, mMixMatrix[1]);
         }
      }
      if (participant->hasOutput())
      {
         // Apply mix inputs to other participants if we have an output
         MprBridge::setMixWeightsForInput(DEFAULT_BRIDGE_RESOURCE_NAME, *mMediaInterface.getMsgQ(), bridgePort, DEFAULT_BRIDGE_MAX_IN_OUTPUTS, inputBridgeWeights);
      }
   }
}

void
SipXBridgeMixer::outputBridgeMixWeights()
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
