#include "Conversation.hxx"
#include "ReconSubsystem.hxx"
#include "RelatedConversationSet.hxx"
#include "Participant.hxx"
#include "LocalParticipant.hxx"
#include "RemoteParticipant.hxx"
#include "UserAgent.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>

using namespace recon;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

RelatedConversationSet::RelatedConversationSet(ConversationManager& conversationManager, 
                                               ConversationHandle initialConversationHandle, 
                                               Conversation* initialConversation)
: mConversationManager(conversationManager),
  mInitialConversationHandle(initialConversationHandle)
{
   mRelatedConversationMap[initialConversationHandle] = initialConversation;

   //InfoLog(<< "RelatedConversationSet created, initialConversationHandle=" << mInitialConversationHandle);
}

RelatedConversationSet::~RelatedConversationSet()
{
   //InfoLog(<< "RelatedConversationSet destroyed, initialConversationHandle=" << mInitialConversationHandle);
}

void 
RelatedConversationSet::addRelatedConversation(ConversationHandle relatedConversationHandle, Conversation* relatedConversation)
{
   mRelatedConversationMap[relatedConversationHandle] = relatedConversation;
}

void 
RelatedConversationSet::removeConversation(ConversationHandle conversationHandle)
{
   mRelatedConversationMap.erase(conversationHandle);
   if(mRelatedConversationMap.size() == 0) 
   {
      // No more conversations - destroy self
      delete this;
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
