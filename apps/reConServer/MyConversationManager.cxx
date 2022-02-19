
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rutil/Log.hxx>

#include "MyConversationManager.hxx"

#include <rutil/Logger.hxx>
#include <AppSubsystem.hxx>

#include <media/kurento/Object.hxx>  // FIXME Kurento includes
#include <resip/recon/Conversation.hxx> // FIXME Kurento includes
#include <resip/recon/KurentoRemoteParticipant.hxx> // FIXME Kurento includes

// Test Prompts for cache testing
#include "playback_prompt.h"
#include "record_prompt.h"

#define RESIPROCATE_SUBSYSTEM AppSubsystem::RECONSERVER

using namespace resip;
using namespace recon;
using namespace reconserver;

#ifdef USE_KURENTO
// FIXME: see comments in MyConversationManager.hxx
MyConversationManager::MyConversationManager(const ReConServerConfig& config, const Data& kurentoUri, bool autoAnswerEnabled)
      : KurentoConversationManager(kurentoUri),
        mConfig(config),
#else
MyConversationManager::MyConversationManager(bool localAudioEnabled, recon::SipXConversationManager::MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate, bool autoAnswerEnabled)
      : SipXConversationManager(localAudioEnabled, mediaInterfaceMode, defaultSampleRate, maxSampleRate, false),
#endif
        mAutoAnswerEnabled(autoAnswerEnabled)
{ 
}

void
MyConversationManager::startup()
{      
   if(supportsLocalAudio())
   {
      // Create initial local participant and conversation  
      addParticipant(createConversation(), createLocalParticipant());
      resip::Uri uri("tone:dialtone;duration=1000");
      createMediaResourceParticipant(mConversationHandles.front(), uri);
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
MyConversationManager::createConversation(AutoHoldMode autoHoldMode)
{
   ConversationHandle convHandle = ConversationManager::createConversation(autoHoldMode);
   mConversationHandles.push_back(convHandle);
   return convHandle;
}

ParticipantHandle
MyConversationManager::createRemoteParticipant(ConversationHandle convHandle, const NameAddr& destination, ParticipantForkSelectMode forkSelectMode, const std::shared_ptr<ConversationProfile>& conversationProfile, const std::multimap<resip::Data, resip::Data>& extraHeaders)
{
   ParticipantHandle partHandle = ConversationManager::createRemoteParticipant(convHandle, destination, forkSelectMode, conversationProfile, extraHeaders);
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
   // FIXME - why is this duplicated here, why not call superclass?
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
      const resip::Data& room = msg.header(h_RequestLine).uri().user();
      RoomMap::const_iterator it = mRooms.find(room);
      if(it == mRooms.end())
      {
         InfoLog(<<"creating Conversation for room: " << room);
         ConversationHandle convHandle = createConversation();
         mRooms[room] = convHandle;
         // ensure a local participant is in the conversation - create one if one doesn't exist
         if(supportsLocalAudio() && mLocalParticipantHandles.empty())
         {
            createLocalParticipant();
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
MyConversationManager::onIncomingKurento(ParticipantHandle partHandle, const SipMessage& msg)
{
   const resip::Data& room = msg.header(h_RequestLine).uri().user();
   RoomMap::const_iterator it = mRooms.find(room);
   if(it == mRooms.end())
   {
      ErrLog(<<"invalid room!");
      resip_assert(0);
   }
   Conversation* conversation = getConversation(it->second);
   unsigned int numRemoteParticipants = conversation->getNumRemoteParticipants();
   KurentoRemoteParticipant *_p = dynamic_cast<KurentoRemoteParticipant*>(conversation->getParticipant(partHandle));
   std::shared_ptr<kurento::BaseRtpEndpoint> answeredEndpoint = _p->getEndpoint();
   if(numRemoteParticipants < 2)
   {
      DebugLog(<<"we are first in the conversation");
      _p->waitingMode();
      return;
   }
   if(numRemoteParticipants > 2)
   {
      WarningLog(<<"participants already here, can't join, numRemoteParticipants = " << numRemoteParticipants);
      return;
   }
   DebugLog(<<"joining a Conversation with an existing Participant");

   if(!answeredEndpoint)
   {
      ErrLog(<<"our endpoint is not initialized"); // FIXME
      return;
   }
   _p->getWaitingModeElement()->disconnect([this, _p, answeredEndpoint, conversation]{
      // Find the other Participant / endpoint

      Conversation::ParticipantMap& m = conversation->getParticipants();
      KurentoRemoteParticipant* krp = 0; // FIXME - better to use shared_ptr
      Conversation::ParticipantMap::iterator _it = m.begin();
      for(;_it != m.end() && krp == 0; _it++)
      {
         krp = dynamic_cast<KurentoRemoteParticipant*>(_it->second.getParticipant());
         if(krp == _p)
         {
            krp = 0;
         }
      }
      resip_assert(krp);
      std::shared_ptr<kurento::BaseRtpEndpoint> otherEndpoint = krp->getEndpoint();

      krp->getWaitingModeElement()->disconnect([this, _p, answeredEndpoint, otherEndpoint, krp]{
         otherEndpoint->connect([this, _p, answeredEndpoint, otherEndpoint, krp]{
            //krp->setLocalHold(false); // FIXME - the Conversation does this automatically
            answeredEndpoint->connect([this, _p, answeredEndpoint, otherEndpoint, krp]{
               //_p->setLocalHold(false); // FIXME - the Conversation does this automatically
               _p->requestKeyframeFromPeer();
               krp->requestKeyframeFromPeer();
            }, *otherEndpoint);
         }, *answeredEndpoint);
      }); // otherEndpoint->disconnect()
   });  // answeredEndpoint->disconnect()
}

void
MyConversationManager::onParticipantDestroyedKurento(ParticipantHandle partHandle)
{
   RoomMap::const_iterator it = mRooms.begin();
   for(;it != mRooms.end();it++)
   {
      Conversation* conversation = getConversation(it->second);
      KurentoRemoteParticipant *_p = dynamic_cast<KurentoRemoteParticipant*>(conversation->getParticipant(partHandle));
      if(_p)
      {
         DebugLog(<<"found participant in room " << it->first);
         std::shared_ptr<kurento::BaseRtpEndpoint> myEndpoint = _p->getEndpoint();
         Conversation::ParticipantMap& m = conversation->getParticipants();
         KurentoRemoteParticipant* krp = 0; // FIXME - better to use shared_ptr
         Conversation::ParticipantMap::iterator _it = m.begin();
         for(;_it != m.end() && krp == 0; _it++)
         {
            krp = dynamic_cast<KurentoRemoteParticipant*>(_it->second.getParticipant());
            if(krp == _p)
            {
               krp = 0;
            }
         }
         if(krp)
         {
            std::shared_ptr<kurento::BaseRtpEndpoint> otherEndpoint = krp->getEndpoint();
            otherEndpoint->disconnect([this, krp]{
               krp->waitingMode();
            });
         }
         else
         {
            /*myEndpoint->release([this]{
               DebugLog(<<"release completed");
            });*/
         }

         return;
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
   onParticipantDestroyedKurento(partHandle);
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
}

void
MyConversationManager::onParticipantConnectedConfirmed(ParticipantHandle partHandle, const SipMessage& msg)
{
   InfoLog(<< "onParticipantConnectedConfirmed: handle=" << partHandle << " msg=" << msg.brief());

   onIncomingKurento(partHandle, msg); // FIXME - Kurento
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
MyConversationManager::configureRemoteParticipant(KurentoRemoteParticipant *rp)
{
   rp->mRemoveExtraMediaDescriptors = mConfig.getConfigBool("KurentoRemoveExtraMediaDescriptors", false);
   rp->mSipRtpEndpoint = mConfig.getConfigBool("KurentoSipRtpEndpoint", true);
   rp->mReuseSdpAnswer = mConfig.getConfigBool("KurentoReuseSdpAnswer", false);
   rp->mWSAcceptsKeyframeRequests = mConfig.getConfigBool("KurentoWebSocketAcceptsKeyframeRequests", true);
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

