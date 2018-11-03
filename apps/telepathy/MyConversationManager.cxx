/*
 * Copyright (C) 2015 Daniel Pocock http://danielpocock.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MyConversationManager.hxx"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/recon/ReconSubsystem.hxx>

// Test Prompts for cache testing
#include "playback_prompt.h"
#include "record_prompt.h"

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

using namespace tr;
using namespace resip;
using namespace recon;

MyConversationManager::MyConversationManager(bool localAudioEnabled, MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate, bool autoAnswerEnabled, Connection *connection)
      : ConversationManager(localAudioEnabled, mediaInterfaceMode, defaultSampleRate, maxSampleRate),
        mLocalAudioEnabled(localAudioEnabled),
        mAutoAnswerEnabled(autoAnswerEnabled),
        mConnection(connection)
{ 
}

void
MyConversationManager::startup()
{      
   if(mLocalAudioEnabled)
   {
      // Create initial local participant and conversation  
      ParticipantHandle ph = createLocalParticipant();
      DebugLog(<<"createLocalParticipant returned handle " << ph);
      addParticipant(createConversation(), ph);
      // Play a brief tone at startup
      //resip::Uri uri("tone:dialtone;duration=200");
      //createMediaResourceParticipant(mConversationHandles.front(), uri);
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

ConversationHandle
MyConversationManager::createConversation()
{
   ConversationHandle convHandle = ConversationManager::createConversation();
   mConversationHandles.push_back(convHandle);
   return convHandle;
}

ParticipantHandle
MyConversationManager::createRemoteParticipant(ConversationHandle convHandle, NameAddr& destination, ParticipantForkSelectMode forkSelectMode)
{
   ParticipantHandle partHandle = ConversationManager::createRemoteParticipant(convHandle, destination, forkSelectMode);
   mRemoteParticipantHandles.push_back(partHandle);
   return partHandle;
}

ParticipantHandle
MyConversationManager::createMediaResourceParticipant(ConversationHandle convHandle, const Uri& mediaUrl)
{
   ParticipantHandle partHandle = ConversationManager::createMediaResourceParticipant(convHandle, mediaUrl);
   mMediaParticipantHandles.push_back(partHandle);
   return partHandle;
}

ParticipantHandle
MyConversationManager::createLocalParticipant()
{
   ParticipantHandle partHandle = ConversationManager::createLocalParticipant();
   mLocalParticipantHandles.push_back(partHandle);
   return partHandle;
}

void
MyConversationManager::onConversationDestroyed(ConversationHandle convHandle)
{
   InfoLog(<< "onConversationDestroyed: handle=" << convHandle);
   mConversationHandles.remove(convHandle);
}

void
MyConversationManager::onParticipantDestroyed(ParticipantHandle partHandle)
{
   InfoLog(<< "onParticipantDestroyed: handle=" << partHandle);
   // Remove from whatever list it is in
   mRemoteParticipantHandles.remove(partHandle);
   mLocalParticipantHandles.remove(partHandle);
   mMediaParticipantHandles.remove(partHandle);
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
   mRemoteParticipantHandles.push_back(partHandle);
   if(mAutoAnswerEnabled)
   {
      // If there are no conversations, then create one
      if(mConversationHandles.empty())
      {
         ConversationHandle convHandle = createConversation();
         // ensure a local participant is in the conversation - create one if one doesn't exist
         if(mLocalAudioEnabled && mLocalParticipantHandles.empty())
         {
            createLocalParticipant();
         }
         addParticipant(convHandle, mLocalParticipantHandles.front());
      }
      addParticipant(mConversationHandles.front(), partHandle);
      answerParticipant(partHandle);
   }
   else
   {
      QString caller(msg.header(h_From).uri().getAorNoPort().c_str());
      QMetaObject::invokeMethod(mConnection, "onIncomingCall", Qt::QueuedConnection, Q_ARG(QString, caller), Q_ARG(uint, partHandle));
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
   mConversationHandles.push_back(relatedConvHandle);
   mRemoteParticipantHandles.push_back(relatedPartHandle);
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
      resip::Uri uri("tone:dialtone;duration=250");
      createMediaResourceParticipant(mConversationHandles.front(), uri);
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

   if(!mConversationHandles.empty())
   {
      output = "Active conversation handles: ";
      std::list<ConversationHandle>::iterator it;
      for(it = mConversationHandles.begin(); it != mConversationHandles.end(); it++)
      {
         output += Data(*it) + " ";
      }
      InfoLog(<< output);
   }
   if(!mLocalParticipantHandles.empty())
   {
      output = "Local Participant handles: ";
      std::list<ParticipantHandle>::iterator it;
      for(it = mLocalParticipantHandles.begin(); it != mLocalParticipantHandles.end(); it++)
      {
         output += Data(*it) + " ";
      }
      InfoLog(<< output);
   }
   if(!mRemoteParticipantHandles.empty())
   {
      output = "Remote Participant handles: ";
      std::list<ParticipantHandle>::iterator it;
      for(it = mRemoteParticipantHandles.begin(); it != mRemoteParticipantHandles.end(); it++)
      {
         output += Data(*it) + " ";
      }
      InfoLog(<< output);
   }
   if(!mMediaParticipantHandles.empty())
   {
      output = "Media Participant handles: ";
      std::list<ParticipantHandle>::iterator it;
      for(it = mMediaParticipantHandles.begin(); it != mMediaParticipantHandles.end(); it++)
      {
         output += Data(*it) + " ";
      }
      InfoLog(<< output);
   }
}

