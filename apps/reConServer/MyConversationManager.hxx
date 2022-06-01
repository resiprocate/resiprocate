#ifndef MYCONVERSATIONMANAGER_HXX
#define MYCONVERSATIONMANAGER_HXX

#ifdef USE_SIPXTAPI
#include <os/OsIntTypes.h>
#endif

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include <rutil/Data.hxx>
#include <resip/recon/ConversationManager.hxx>
#include "reConServerConfig.hxx"
#include <thread>
#include <chrono>
#include <memory>
#include <mutex>
namespace reconserver
{

#ifdef USE_KURENTO
#define PREFER_KURENTO
// FIXME: hard-coded to use Kurento when selected at compile time
// Need to have both USE_KURENTO and USE_SIPXTAPI as we haven't removed
// some references to sipXtapi in parts of the code
#else
#define PREFER_SIPXTAPI
#endif
class MyConversationManager : public recon::ConversationManager
{
public:

   MyConversationManager(const ReConServerConfig& config, const resip::Data& kurentoUri, bool localAudioEnabled, int defaultSampleRate, int maxSampleRate, bool autoAnswerEnabled);
   virtual ~MyConversationManager() {};

   virtual void startup();
   
   virtual void onConversationDestroyed(recon::ConversationHandle convHandle) override;
   virtual void onParticipantDestroyed(recon::ParticipantHandle partHandle) override;
   virtual void onParticipantDestroyedKurento(recon::ParticipantHandle partHandle);
   virtual void onDtmfEvent(recon::ParticipantHandle partHandle, int dtmf, int duration, bool up) override;
   virtual void onIncomingParticipant(recon::ParticipantHandle partHandle, const resip::SipMessage& msg, bool autoAnswer, recon::ConversationProfile& conversationProfile) override;
   virtual void onRequestOutgoingParticipant(recon::ParticipantHandle partHandle, const resip::SipMessage& msg, recon::ConversationProfile& conversationProfile) override;
   virtual void onParticipantTerminated(recon::ParticipantHandle partHandle, unsigned int statusCode) override;
   virtual void onParticipantProceeding(recon::ParticipantHandle partHandle, const resip::SipMessage& msg) override;
   virtual void onRelatedConversation(recon::ConversationHandle relatedConvHandle, recon::ParticipantHandle relatedPartHandle,
                                      recon::ConversationHandle origConvHandle, recon::ParticipantHandle origPartHandle) override;
   virtual void onParticipantAlerting(recon::ParticipantHandle partHandle, const resip::SipMessage& msg) override;
   virtual void onParticipantConnected(recon::ParticipantHandle partHandle, const resip::SipMessage& msg) override;
   virtual void onParticipantConnectedConfirmed(recon::ParticipantHandle partHandle, const resip::SipMessage& msg) override;
   virtual void onParticipantRedirectSuccess(recon::ParticipantHandle partHandle) override;
   virtual void onParticipantRedirectFailure(recon::ParticipantHandle partHandle, unsigned int statusCode) override;
   virtual void onParticipantRequestedHold(recon::ParticipantHandle partHandle, bool held) override;
   virtual void displayInfo();
   std::shared_ptr<std::thread> FastUpdateRequestThread;
   struct RemoteParticipantFurTrackerStruct
   {
       const std::vector<int> FUR_INTERVALS_SECONDS = { 2, 2, 5, 5, 7, 10, 20, 30 };
       unsigned int CurrentFURIntervalSeconds;
       unsigned int FUROccurences = 0;
       recon::ParticipantHandle Handler;
       std::chrono::system_clock::time_point Timestamp;
       RemoteParticipantFurTrackerStruct(recon::ParticipantHandle handler)
       {
           this->Handler = handler;
           this->Reset();
       }

       bool IsFurDue()
       {
           auto check = std::chrono::system_clock::now();
           if (std::chrono::duration_cast<std::chrono::seconds>(check - this->Timestamp).count() > this->CurrentFURIntervalSeconds)
           {
               this->Timestamp = check;
               this->Touch();
               return true;
           }
           return false;
       }

       void Reset()
       {
           FUROccurences = 0;
           this->CurrentFURIntervalSeconds = FUR_INTERVALS_SECONDS.front();
           // set start time 5 sec into the future to lag the due time
           this->Timestamp = std::chrono::system_clock::now() + std::chrono::seconds(5);
       }
       void Touch()
       {
           if (FUROccurences <  FUR_INTERVALS_SECONDS.size()-1)
           {
               CurrentFURIntervalSeconds = FUR_INTERVALS_SECONDS.at(FUROccurences);
           }
           else
           {
               CurrentFURIntervalSeconds = FUR_INTERVALS_SECONDS.back();
           }
           FUROccurences++;
       }
   };
   std::vector<std::shared_ptr<RemoteParticipantFurTrackerStruct>> RemoteParticipantFURVector;
   std::mutex RemoteParticipantFURVectorMutex;
   void FastUpdateRequestWorkerLoop();

protected:
   virtual void onRemoteParticipantConstructed(recon::RemoteParticipant *rp) override;
   virtual void onIncomingKurento(recon::ParticipantHandle partHandle, const resip::SipMessage& msg);
   ReConServerConfig mConfig;
   typedef std::map<resip::Data, recon::ConversationHandle> RoomMap;
   RoomMap mRooms;
   bool mAutoAnswerEnabled;
};

}

#endif

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

