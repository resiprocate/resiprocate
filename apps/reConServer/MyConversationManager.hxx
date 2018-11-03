#ifndef MYCONVERSATIONMANAGER_HXX
#define MYCONVERSATIONMANAGER_HXX

#include <os/OsIntTypes.h>

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include <rutil/Data.hxx>
#include <resip/recon/ConversationManager.hxx>

namespace reconserver
{

class MyConversationManager : public recon::ConversationManager
{
public:

   MyConversationManager(bool localAudioEnabled, recon::ConversationManager::MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate, bool autoAnswerEnabled);
   virtual ~MyConversationManager() {};

   virtual void startup();
   
   virtual recon::ConversationHandle createConversation();
   virtual recon::ParticipantHandle createRemoteParticipant(recon::ConversationHandle convHandle, resip::NameAddr& destination, recon::ConversationManager::ParticipantForkSelectMode forkSelectMode = ForkSelectAutomatic);
   virtual recon::ParticipantHandle createMediaResourceParticipant(recon::ConversationHandle convHandle, const resip::Uri& mediaUrl);
   virtual recon::ParticipantHandle createLocalParticipant();
   virtual void onConversationDestroyed(recon::ConversationHandle convHandle);
   virtual void onParticipantDestroyed(recon::ParticipantHandle partHandle);
   virtual void onDtmfEvent(recon::ParticipantHandle partHandle, int dtmf, int duration, bool up);
   virtual void onIncomingParticipant(recon::ParticipantHandle partHandle, const resip::SipMessage& msg, bool autoAnswer, recon::ConversationProfile& conversationProfile);
   virtual void onRequestOutgoingParticipant(recon::ParticipantHandle partHandle, const resip::SipMessage& msg, recon::ConversationProfile& conversationProfile);
   virtual void onParticipantTerminated(recon::ParticipantHandle partHandle, unsigned int statusCode);
   virtual void onParticipantProceeding(recon::ParticipantHandle partHandle, const resip::SipMessage& msg);
   virtual void onRelatedConversation(recon::ConversationHandle relatedConvHandle, recon::ParticipantHandle relatedPartHandle,
                                      recon::ConversationHandle origConvHandle, recon::ParticipantHandle origPartHandle);
   virtual void onParticipantAlerting(recon::ParticipantHandle partHandle, const resip::SipMessage& msg);
   virtual void onParticipantConnected(recon::ParticipantHandle partHandle, const resip::SipMessage& msg);
   virtual void onParticipantRedirectSuccess(recon::ParticipantHandle partHandle);
   virtual void onParticipantRedirectFailure(recon::ParticipantHandle partHandle, unsigned int statusCode);
   virtual void onParticipantRequestedHold(recon::ParticipantHandle partHandle, bool held);
   virtual void displayInfo();

protected:
   std::list<recon::ConversationHandle> mConversationHandles;
   std::list<recon::ParticipantHandle> mLocalParticipantHandles;
   std::list<recon::ParticipantHandle> mRemoteParticipantHandles;
   std::list<recon::ParticipantHandle> mMediaParticipantHandles;
   bool mLocalAudioEnabled;
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

