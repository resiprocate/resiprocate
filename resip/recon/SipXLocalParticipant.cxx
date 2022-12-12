#include "SipXBridgeMixer.hxx"
#include "ConversationManager.hxx"
#include "Conversation.hxx"
#include "UserAgent.hxx"
#include "ReconSubsystem.hxx"
#include "SipXLocalParticipant.hxx"
#include <CpTopologyGraphInterface.h>

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

SipXLocalParticipant::SipXLocalParticipant(ParticipantHandle partHandle,
                                   ConversationManager& conversationManager,
                                   SipXMediaStackAdapter& sipXMediaStackAdapter)
: Participant(partHandle, ConversationManager::ParticipantType_Local, conversationManager),
  LocalParticipant(partHandle, conversationManager),
  SipXParticipant(partHandle, ConversationManager::ParticipantType_Local, conversationManager, sipXMediaStackAdapter),
  mLocalPortOnBridge(-1)
{
   InfoLog(<< "SipXLocalParticipant created, handle=" << mHandle);
}

SipXLocalParticipant::~SipXLocalParticipant()
{
   // Note:  Ideally this call would exist in the Participant Base class - but this call requires 
   //        dynamic_casts and virtual methods to function correctly during destruction.
   //        If the call is placed in the base Participant class then these things will not
   //        function as desired because a classes type changes as the descructors unwind.
   //        See https://stackoverflow.com/questions/10979250/usage-of-this-in-destructor.
   unregisterFromAllConversations();

   InfoLog(<< "SipXLocalParticipant destroyed, handle=" << mHandle);
}

int 
SipXLocalParticipant::getConnectionPortOnBridge()
{
   if(mLocalPortOnBridge == -1)
   {
      resip_assert(getMediaInterface() != 0);       
      getMediaInterface()->getInterface()->getResourceInputPortOnBridge(VIRTUAL_NAME_LOCAL_STREAM_OUTPUT,0,mLocalPortOnBridge);
      InfoLog(<< "SipXLocalParticipant getConnectionPortOnBridge, handle=" << mHandle << ", localPortOnBridge=" << mLocalPortOnBridge);
   }
   return mLocalPortOnBridge;
}

void 
SipXLocalParticipant::addToConversation(Conversation *conversation, unsigned int inputGain, unsigned int outputGain)
{
    Participant::addToConversation(conversation, inputGain, outputGain);

    if(mSipXMediaStackAdapter.getMediaInterfaceMode() == SipXMediaStackAdapter::sipXConversationMediaInterfaceMode)
    {
       // The Local participant is in a new Conversation, give that conversation focus
       resip_assert(getMediaInterface() != 0);       
       getMediaInterface()->getInterface()->giveFocus();
    }
}


/* ====================================================================

 Copuright (c) 2021, SIP Spectrum, Inc. www.sipspectrum.com
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
