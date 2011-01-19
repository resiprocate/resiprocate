#include "AppSubsystem.hxx"
#include "MOHManager.hxx"
#include "Server.hxx"
#include "../UserAgent.hxx"

#include <resip/stack/ExtensionParameter.hxx>
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::MOHPARKSERVER

static const resip::ExtensionParameter p_automaton("automaton");
static const resip::ExtensionParameter p_byeless("+sip.byeless");
static const resip::ExtensionParameter p_rendering("+sip.rendering");

namespace mohparkserver 
{

MOHManager::MOHManager(Server& server) :
   mServer(server)
{
}

MOHManager::~MOHManager()
{
}

void
MOHManager::startup()
{
   // Setup ConversationProfile
   SharedPtr<ConversationProfile> mohConversationProfile = SharedPtr<ConversationProfile>(new ConversationProfile(mServer.mUserAgentMasterProfile));
   mohConversationProfile->setDefaultRegistrationTime(mServer.mConfig.mMOHRegistrationTime);  
   mohConversationProfile->setDefaultRegistrationRetryTime(120);  // 2 mins
   mohConversationProfile->setDefaultFrom(mServer.mConfig.mMOHUri);
   mohConversationProfile->setDigestCredential(mServer.mConfig.mMOHUri.uri().host(), mServer.mConfig.mMOHUri.uri().user(), mServer.mConfig.mMOHPassword);  
   mohConversationProfile->challengeOODReferRequests() = false;
   mohConversationProfile->setExtraHeadersInReferNotifySipFragEnabled(true);  // Enable dialog identifying headers in SipFrag bodies of Refer Notifies - required for a music on hold server
   NameAddr capabilities;
   capabilities.param(p_automaton);
   capabilities.param(p_byeless);
   capabilities.param(p_rendering) = "\"no\"";
   mohConversationProfile->setUserAgentCapabilities(capabilities);
   mohConversationProfile->natTraversalMode() = ConversationProfile::NoNatTraversal;
   mohConversationProfile->secureMediaMode() = ConversationProfile::NoSecureMedia;
   mServer.buildSessionCapabilities(mohConversationProfile->sessionCaps());
   mConversationProfile = mohConversationProfile;
   mServer.mMyUserAgent->addConversationProfile(mConversationProfile);

   // Create an initial conversation and start music
   ConversationHandle convHandle = mServer.createConversation(true /* broadcast only*/);

   // Play Music
   mServer.createMediaResourceParticipant(convHandle, mServer.mConfig.mMOHFilenameUrl);
   mConversations[convHandle];
}

void
MOHManager::shutdown(bool shuttingDownServer)
{
   // Destroy all conversations
   ConversationMap::iterator it = mConversations.begin();
   for(; it != mConversations.end(); it++)
   {
       mServer.destroyConversation(it->first);
   }
   mConversations.clear();

   // If shutting down server, then we shouldn't remove the conversation profiles here
   // shutting down the ConversationManager will take care of this.  We need to be sure
   // we don't remove all conversation profiles when we are still processing SipMessages,
   // since recon requires at least one to be present for inbound processing.
   if(mConversationProfile && !shuttingDownServer)
   {
       mServer.mMyUserAgent->destroyConversationProfile(mConversationProfile->getHandle());
   }
}

bool 
MOHManager::isMyProfile(recon::ConversationProfile& profile)
{
    return profile.getHandle() == mConversationProfile->getHandle();
}

void 
MOHManager::addParticipant(ParticipantHandle participantHandle)
{
   ConversationHandle conversationToUse = 0;
   // Check if we have an existing conversation with room to add this party
   ConversationMap::iterator it = mConversations.begin();
   for(; it != mConversations.end(); it++)
   {
      if(it->second.size() < DEFAULT_BRIDGE_MAX_IN_OUTPUTS-3)
      {
         // Found an existing conversation with room - add the participant here
         conversationToUse = it->first;
         break;
      }
   }

   // No conversation found that we can use - create a new one
   if(!conversationToUse)
   {
      conversationToUse = mServer.createConversation(true /* broadcast only*/);
      InfoLog(<< "MOHManager::addParticipant created new conversation for music on hold, id=" << conversationToUse);

      // Play Music
      mServer.createMediaResourceParticipant(conversationToUse, mServer.mConfig.mMOHFilenameUrl);
   }

   assert(conversationToUse);

   mServer.addParticipant(conversationToUse, participantHandle);
   mServer.modifyParticipantContribution(conversationToUse, participantHandle, 100, 0 /* Mute participant */);
   mServer.answerParticipant(participantHandle);
   mConversations[conversationToUse].insert(participantHandle);
}

bool
MOHManager::removeParticipant(ParticipantHandle participantHandle)
{
   // Find Conversation that participant is in
   ConversationMap::iterator it = mConversations.begin();
   for(; it != mConversations.end(); it++)
   {
      std::set<ParticipantHandle>::iterator partIt = it->second.find(participantHandle);
      if(partIt != it->second.end())
      {
         InfoLog(<< "MOHManager::removeParticipant found in conversation id=" << it->first << ", size=" << it->second.size());

         // Found! Remove from conversation
         it->second.erase(partIt);

         // Check if conversation is now empty, and it's not the last conversation
         if(it->second.size() == 0 && mConversations.size() > 1)
         {
            // Destroy conversation (and containing media participant)
            mServer.destroyConversation(it->first);

            // Remove Conversation from Map
            mConversations.erase(it);

            InfoLog(<< "MOHManager::removeParticipant last participant in conversation, destroying conversation, num conversations now=" << mConversations.size());
         }
         return true;
      }
   }
   return false;
}


}

/* ====================================================================

 Copyright (c) 2010, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
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

