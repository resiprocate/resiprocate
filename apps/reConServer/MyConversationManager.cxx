
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rutil/Log.hxx>

#include "MyConversationManager.hxx"

#include <rutil/Logger.hxx>
#include <AppSubsystem.hxx>

#include <resip/recon/LocalParticipant.hxx>
#include <resip/recon/RemoteParticipant.hxx>

// Test Prompts for cache testing
#include "playback_prompt.h"
#include "record_prompt.h"

#define RESIPROCATE_SUBSYSTEM AppSubsystem::RECONSERVER

using namespace std;
using namespace resip;
using namespace recon;
using namespace reconserver;

MyConversationManager::MyConversationManager(bool localAudioEnabled, recon::SipXMediaStackAdapter::MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate, bool autoAnswerEnabled)
      : ConversationManager(nullptr),
        mAutoAnswerEnabled(autoAnswerEnabled)
{ 
#ifdef USE_SIPXTAPI
   shared_ptr<MediaStackAdapter> mediaStackAdapter = make_shared<SipXMediaStackAdapter>(*this, localAudioEnabled, mediaInterfaceMode, defaultSampleRate, maxSampleRate, false);
#else
   #error Need Kurento or sipXtapi
#endif
   setMediaStackAdapter(mediaStackAdapter);
}

void
MyConversationManager::startup()
{      
   if(getMediaStackAdapter().supportsLocalAudio())
   {
      // Create initial local participant and conversation  
      ConversationHandle initialConversation = createConversation();
      addParticipant(initialConversation, createLocalParticipant());
      resip::Uri uri("tone:dialtone;duration=1000");
      createMediaResourceParticipant(initialConversation, uri);
   }
   else
   {
      // If no local audio - just create a starter conversation
      // FIXME - do we really need an empty conversation on startup?
      // If in B2BUA mode, this will never be used
      createConversation();
   }

   // Load 2 items into cache for testing
   {
      resip::Data buffer(Data::Share, (const char*)playback_prompt, sizeof(playback_prompt));
      resip::Data name("playback");
      addBufferToMediaResourceCache(name, buffer, 0);
   }
   {
      resip::Data buffer(Data::Share, (const char *)record_prompt, sizeof(record_prompt));
      resip::Data name("record");
      addBufferToMediaResourceCache(name, buffer, 0);
   }      
}

void
MyConversationManager::onConversationDestroyed(ConversationHandle convHandle)
{
   InfoLog(<< "onConversationDestroyed: handle=" << convHandle);
}

void
MyConversationManager::onParticipantDestroyed(ParticipantHandle partHandle)
{
   InfoLog(<< "onParticipantDestroyed: handle=" << partHandle);
}

void
MyConversationManager::onDtmfEvent(ParticipantHandle partHandle, int dtmf, int duration, bool up)
{
   InfoLog(<< "onDtmfEvent: handle=" << partHandle << " tone=" << dtmf << " dur=" << duration << " up=" << up);
}

void
MyConversationManager::onIncomingParticipant(ParticipantHandle partHandle, const SipMessage& msg, bool autoAnswer, ConversationProfile& conversationProfile)
{
   InfoLog(<< "onIncomingParticipant: handle=" << partHandle << "auto=" << autoAnswer << " msg=" << msg.brief());
   if(mAutoAnswerEnabled)
   {
      const resip::Data& room = msg.header(h_RequestLine).uri().user();
      RoomMap::const_iterator it = mRooms.find(room);
      if(it == mRooms.end())
      {
         InfoLog(<<"creating Conversation for room: " << room);
         ConversationHandle convHandle = createConversation();
         mRooms[room] = convHandle;
         // ensure a local participant is in the conversation - create one if one doesn't exist
         if(getMediaStackAdapter().supportsLocalAudio())
         {
            ParticipantHandle localPartHandle = 0;
            const set<ParticipantHandle> participantHandles = getParticipantHandlesByType(ConversationManager::ParticipantType_Local);
            // If no local participant then create one, otherwise use first in set
            if (participantHandles.empty())
            {
               localPartHandle = createLocalParticipant();
            }
            else
            {
               localPartHandle = *participantHandles.begin();
            }
            // Add local participant to conversation
            addParticipant(convHandle, localPartHandle);
         }
         addParticipant(convHandle, partHandle);
         answerParticipant(partHandle);
      }
      else
      {
         InfoLog(<<"found Conversation for room: " << room);
         addParticipant(it->second, partHandle);
         answerParticipant(partHandle);
      }
   }
}

void
MyConversationManager::onRequestOutgoingParticipant(ParticipantHandle partHandle, const SipMessage& msg, ConversationProfile& conversationProfile)
{
   InfoLog(<< "onRequestOutgoingParticipant: handle=" << partHandle << " msg=" << msg.brief());
   /*
   if(mConvHandles.empty())
   {
      ConversationHandle convHandle = createConversation();
      addParticipant(convHandle, partHandle);
   }*/
}
 
void
MyConversationManager::onParticipantTerminated(ParticipantHandle partHandle, unsigned int statusCode)
{
   InfoLog(<< "onParticipantTerminated: handle=" << partHandle);
}
 
void
MyConversationManager::onParticipantProceeding(ParticipantHandle partHandle, const SipMessage& msg)
{
   InfoLog(<< "onParticipantProceeding: handle=" << partHandle << " msg=" << msg.brief());
}

void
MyConversationManager::onRelatedConversation(ConversationHandle relatedConvHandle, ParticipantHandle relatedPartHandle, 
                                   ConversationHandle origConvHandle, ParticipantHandle origPartHandle)
{
   InfoLog(<< "onRelatedConversation: relatedConvHandle=" << relatedConvHandle << " relatedPartHandle=" << relatedPartHandle
           << " origConvHandle=" << origConvHandle << " origPartHandle=" << origPartHandle);
}

void
MyConversationManager::onParticipantAlerting(ParticipantHandle partHandle, const SipMessage& msg)
{
   InfoLog(<< "onParticipantAlerting: handle=" << partHandle << " msg=" << msg.brief());
}
    
void
MyConversationManager::onParticipantConnected(ParticipantHandle partHandle, const SipMessage& msg)
{
   InfoLog(<< "onParticipantConnected: handle=" << partHandle << " msg=" << msg.brief());
}

void
MyConversationManager::onParticipantConnectedConfirmed(ParticipantHandle partHandle, const SipMessage& msg)
{
   InfoLog(<< "onParticipantConnectedConfirmed: handle=" << partHandle << " msg=" << msg.brief());
}

void
MyConversationManager::onParticipantRedirectSuccess(ParticipantHandle partHandle)
{
   InfoLog(<< "onParticipantRedirectSuccess: handle=" << partHandle);
}

void
MyConversationManager::onParticipantRedirectFailure(ParticipantHandle partHandle, unsigned int statusCode)
{
   InfoLog(<< "onParticipantRedirectFailure: handle=" << partHandle << " statusCode=" << statusCode);
}

void
MyConversationManager::onParticipantRequestedHold(ParticipantHandle partHandle, bool held)
{
   InfoLog(<< "onParticipantRequestedHold: handle=" << partHandle << " held=" << held);
}

void
MyConversationManager::displayInfo()
{
   Data output;

   const set<ConversationHandle> conversations = getConversationHandles();
   if (!conversations.empty())
   {
      output = "Active conversation handles: ";
      set<ConversationHandle>::const_iterator it;
      for (it = conversations.begin(); it != conversations.end(); it++)
      {
         output += Data(*it) + " ";
      }
      InfoLog(<< output);
   }
   const set<ParticipantHandle> localParticipantHandles = getParticipantHandlesByType(ConversationManager::ParticipantType_Local);
   if (!localParticipantHandles.empty())
   {
      output = "Local Participant handles: ";
      std::set<ParticipantHandle>::const_iterator it;
      for (it = localParticipantHandles.begin(); it != localParticipantHandles.end(); it++)
      {
         output += Data(*it) + " ";
      }
      InfoLog(<< output);
   }
   const set<ParticipantHandle> remoteParticipantHandles = getParticipantHandlesByType(ConversationManager::ParticipantType_Remote);
   if (!remoteParticipantHandles.empty())
   {
      output = "Remote Participant handles: ";
      std::set<ParticipantHandle>::const_iterator it;
      for (it = remoteParticipantHandles.begin(); it != remoteParticipantHandles.end(); it++)
      {
         output += Data(*it) + " ";
      }
      InfoLog(<< output);
   }
   set<ParticipantHandle> remoteIMParticipantHandles = getParticipantHandlesByType(ConversationManager::ParticipantType_RemoteIMPager);
   const set<ParticipantHandle> remoteIMSessionParticipantHandles = getParticipantHandlesByType(ConversationManager::ParticipantType_RemoteIMSession);
   remoteIMParticipantHandles.insert(remoteIMSessionParticipantHandles.begin(), remoteIMSessionParticipantHandles.end());  // merge the two lists
   if (!remoteIMParticipantHandles.empty())
   {
      output = "Remote IM Participant handles: ";
      std::set<ParticipantHandle>::iterator it;
      for (it = remoteIMParticipantHandles.begin(); it != remoteIMParticipantHandles.end(); it++)
      {
         output += Data(*it) + " ";
      }
      InfoLog(<< output);
   }
   const set<ParticipantHandle> mediaParticipantHandles = getParticipantHandlesByType(ConversationManager::ParticipantType_MediaResource);
   if (!mediaParticipantHandles.empty())
   {
      output = "Media Participant handles: ";
      std::set<ParticipantHandle>::const_iterator it;
      for (it = mediaParticipantHandles.begin(); it != mediaParticipantHandles.end(); it++)
      {
         output += Data(*it) + " ";
      }
      InfoLog(<< output);
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

