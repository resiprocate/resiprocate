
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rutil/Log.hxx>

#include "MyConversationManager.hxx"
#include "MyUserAgent.hxx"

#include <rutil/Logger.hxx>
#include "AppSubsystem.hxx"

#include <resip/recon/LocalParticipant.hxx>
#include <resip/recon/RemoteParticipant.hxx>
#include <resip/recon/Conversation.hxx>
#ifdef USE_GSTREAMER
#include <resip/recon/GstRemoteParticipant.hxx>
#endif
#ifdef USE_KURENTO
#include <media/kurento/Object.hxx>
#include <resip/recon/KurentoRemoteParticipant.hxx>
#endif
#ifdef USE_LIBWEBRTC
#include <resip/recon/LibWebRTCMediaStackAdapter.hxx>
#endif

// Test Prompts for cache testing
#include "media/samples/playback_prompt.h"
#include "media/samples/record_prompt.h"

#define RESIPROCATE_SUBSYSTEM AppSubsystem::RECONSERVER

using namespace std;
using namespace resip;
using namespace recon;
using namespace reconserver;

MyConversationManager::MyConversationManager(const ReConServerConfig& config, bool localAudioEnabled, int defaultSampleRate, int maxSampleRate, bool autoAnswerEnabled)
      : ConversationManager(nullptr, std::shared_ptr<ConfigParse>(new ReConServerConfig(config))),
        mConfig(config),
        mAutoAnswerEnabled(autoAnswerEnabled)
{
   ReConServerConfig::MediaStack mediaStack = config.getConfigMediaStack("MediaStack", ReConServerConfig::sipXtapi);
   shared_ptr<MediaStackAdapter> mediaStackAdapter;
   if(mediaStack == ReConServerConfig::Gstreamer)
   {
#ifdef USE_GSTREAMER
      mediaStackAdapter = make_shared<GstMediaStackAdapter>(*this);
#endif
   }
   else if(mediaStack == ReConServerConfig::LibWebRTC)
   {
#ifdef USE_LIBWEBRTC
      #error libWebRTC not fully implemented yet // FIXME
      mediaStackAdapter = make_shared<LibWebRTCMediaStackAdapter>(*this);
#endif
   }
   else if(mediaStack == ReConServerConfig::Kurento)
   {
#ifdef USE_KURENTO
      Data kurentoUri = config.getConfigData("KurentoURI", "ws://127.0.0.1:8888/kurento");
      mediaStackAdapter = make_shared<KurentoMediaStackAdapter>(*this, kurentoUri);
#endif
   }
   else if(mediaStack == ReConServerConfig::sipXtapi)
   {
#ifdef USE_SIPXTAPI
      SipXMediaStackAdapter::MediaInterfaceMode mediaInterfaceMode = config.getConfigBool("GlobalMediaInterface", false)
         ? SipXMediaStackAdapter::sipXGlobalMediaInterfaceMode : SipXMediaStackAdapter::sipXConversationMediaInterfaceMode;
      mediaStackAdapter = make_shared<SipXMediaStackAdapter>(*this, localAudioEnabled, mediaInterfaceMode, defaultSampleRate, maxSampleRate, false);
#endif
   }
   resip_assert(mediaStackAdapter);
   setMediaStackAdapter(mediaStackAdapter);
}

void
MyConversationManager::startup()
{      
   if(getMediaStackAdapter().supportsLocalAudio())
   {
      // Create initial local participant and conversation  
      ConversationHandle initialConversation = createConversation(getConfig().getConfigAutoHoldMode("AutoHoldMode", ConversationManager::AutoHoldEnabled));
      addParticipant(initialConversation, createLocalParticipant());
      resip::Uri uri("tone:dialtone;duration=1000");
      createMediaResourceParticipant(initialConversation, uri);
   }
   else
   {
      // If no local audio - just create a starter conversation
      // FIXME - do we really need an empty conversation on startup?
      // If in B2BUA mode, this will never be used
      createConversation(getConfig().getConfigAutoHoldMode("AutoHoldMode", ConversationManager::AutoHoldEnabled));
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

ConversationHandle
MyConversationManager::getRoom(const resip::Data& roomName)
{
   RoomMap::const_iterator it = mRooms.find(roomName);
   if(it == mRooms.end())
   {
      InfoLog(<<"creating Conversation for room: " << roomName);
      ConversationHandle convHandle = createConversation(getConfig().getConfigAutoHoldMode("AutoHoldMode", ConversationManager::AutoHoldEnabled));
      mRooms[roomName] = convHandle;
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
      return convHandle;
   }
   else
   {
      InfoLog(<<"found Conversation for room: " << roomName);
      return it->second;
   }
}

void
MyConversationManager::inviteToRoom(const Data& roomName, const NameAddr& destination)
{
   ConversationHandle convHandle = getRoom(roomName);
   MyUserAgent *ua = dynamic_cast<MyUserAgent*>(getUserAgent());
   resip_assert(ua);
   const auto _profile = ua->getDefaultOutgoingConversationProfile();
   std::shared_ptr<ConversationProfile> profile = std::make_shared<ConversationProfile>(*_profile);
   const std::multimap<resip::Data,resip::Data> extraHeaders;
   createRemoteParticipant(convHandle,
         destination,
         ConversationManager::ForkSelectAutomatic,
         profile,
         extraHeaders);
}

void
MyConversationManager::onIncomingParticipant(ParticipantHandle partHandle, const SipMessage& msg, bool autoAnswer, ConversationProfile& conversationProfile)
{
   InfoLog(<< "onIncomingParticipant: handle=" << partHandle << "auto=" << autoAnswer << " msg=" << msg.brief());
   std::stringstream event;
   event << "{\"event\":\"incomingParticipant\",\"participant\":" << partHandle <<
            ",\"brief\":\""<< msg.brief() << "\"}";
   notifyEvent(event.str().c_str());
   if(mAutoAnswerEnabled)
   {
      const resip::Data& room = msg.header(h_RequestLine).uri().user();
      ConversationHandle convHandle = getRoom(room);
      addParticipant(convHandle, partHandle);
      answerParticipant(partHandle);
   }
}

void
MyConversationManager::onRequestOutgoingParticipant(ParticipantHandle partHandle, const SipMessage& msg, ConversationProfile& conversationProfile)
{
   InfoLog(<< "onRequestOutgoingParticipant: handle=" << partHandle << " msg=" << msg.brief());
   std::stringstream event;
   event << "{\"event\":\"requestOutgoingParticipant\",\"participant\":" << partHandle <<
            ",\"brief\":\""<< msg.brief() << "\"}";
   notifyEvent(event.str().c_str());
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
   std::stringstream event;
   event << "{\"event\":\"participantTerminated\",\"participant\":" << partHandle <<
            ",\"statusCode\":"<< statusCode << "}";
   notifyEvent(event.str().c_str());
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
   std::stringstream event;
   event << "{\"event\":\"participantConnected\",\"participant\":" << partHandle <<
            ",\"brief\":\""<< msg.brief() << "\"}";
   notifyEvent(event.str().c_str());
}

void
MyConversationManager::onParticipantConnectedConfirmed(ParticipantHandle partHandle, const SipMessage& msg)
{
   InfoLog(<< "onParticipantConnectedConfirmed: handle=" << partHandle << " msg=" << msg.brief());
   std::stringstream event;
   event << "{\"event\":\"participantConnectedConfirmed\",\"participant\":" << partHandle <<
            ",\"brief\":\""<< msg.brief() << "\"}";
   notifyEvent(event.str().c_str());
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

 Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2013-2022, Daniel Pocock https://danielpocock.com
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

