#if !defined(MediaStackAdapter_hxx)
#define MediaStackAdapter_hxx

#include "BridgeMixer.hxx"
#include "ConversationManager.hxx"

#include <resip/stack/Uri.hxx>
#include <resip/stack/Contents.hxx>

#include <reflow/RTCPEventLoggingHandler.hxx>

#include "MediaResourceCache.hxx"
#include "MediaEvent.hxx"
#include "HandleTypes.hxx"

#include <memory>

namespace resip
{
class DialogUsageManager;
}

namespace recon
{
class Conversation;
class RelatedConversationSet;
class Participant;
class UserAgent;
class ConversationProfile;
class LocalParticipant;
class MediaResourceParticipant;
class RemoteParticipant;
class RemoteParticipantDialogSet;


/**
  This class is one of two main classes of concern to an application
*/

class MediaStackAdapter
{
public:

   MediaStackAdapter(ConversationManager& conversationManager);
   virtual ~MediaStackAdapter();

   virtual void conversationManagerReady(ConversationManager* conversationManager) = 0;
   virtual void shutdown() = 0;

   ///////////////////////////////////////////////////////////////////////
   // Participant methods  ///////////////////////////////////////////////
   ///////////////////////////////////////////////////////////////////////

   /**
     Builds a session capabilties SDPContents based on the passed in ipaddress
     and codec ordering.
     Note:  Codec ordering is an array of sipX internal codecId's.  Id's for
            codecs not loaded are ignored.
   */
   virtual void buildSessionCapabilities(const resip::Data& ipaddress,
      const std::vector<unsigned int>& codecIds, resip::SdpContents& sessionCaps) = 0;

   ///////////////////////////////////////////////////////////////////////
   // Media Related Methods - this may not be the right spot for these - move to LocalParticipant?
   ///////////////////////////////////////////////////////////////////////

   virtual Conversation* createConversationInstance(ConversationHandle handle,
      RelatedConversationSet* relatedConversationSet,  // Pass NULL to create new RelatedConversationSet
      ConversationHandle sharedMediaInterfaceConvHandle,
      ConversationManager::AutoHoldMode autoHoldMode) = 0;
   virtual LocalParticipant* createLocalParticipantInstance(ParticipantHandle partHandle) = 0;
   virtual MediaResourceParticipant* createMediaResourceParticipantInstance(ParticipantHandle partHandle, const resip::Uri& mediaUrl, const std::shared_ptr<resip::Data>& audioBuffer) = 0;
   virtual RemoteParticipant* createRemoteParticipantInstance(resip::DialogUsageManager& dum, RemoteParticipantDialogSet& rpds) = 0;
   virtual RemoteParticipant* createRemoteParticipantInstance(ParticipantHandle partHandle, resip::DialogUsageManager& dum, RemoteParticipantDialogSet& rpds) = 0;
   virtual RemoteParticipantDialogSet* createRemoteParticipantDialogSetInstance(
         ConversationManager::ParticipantForkSelectMode forkSelectMode = ConversationManager::ForkSelectAutomatic,
         std::shared_ptr<ConversationProfile> conversationProfile = nullptr) = 0;

   virtual bool supportsMultipleMediaInterfaces() = 0;
   virtual bool canConversationsShareParticipants(Conversation* conversation1, Conversation* conversation2) = 0;
   virtual bool supportsLocalAudio() = 0;

   virtual void setRTCPEventLoggingHandler(std::shared_ptr<flowmanager::RTCPEventLoggingHandler> h) { mRTCPEventLoggingHandler = h; };
   virtual std::shared_ptr<flowmanager::RTCPEventLoggingHandler> getRTCPEventLoggingHandler() const { return mRTCPEventLoggingHandler; };

protected:
   friend class ConversationManager;
   virtual void setUserAgent(UserAgent *userAgent) = 0;
   virtual UserAgent* getUserAgent() { return getConversationManager().getUserAgent(); };
   ConversationManager& getConversationManager() { return mConversationManager; };
   std::shared_ptr<BridgeMixer>& getBridgeMixer() { return getConversationManager().getBridgeMixer(); }
   bool isShuttingDown() { return getConversationManager().isShuttingDown(); };
   ConversationHandle getNewConversationHandle() { return getConversationManager().getNewConversationHandle(); };
   void post(resip::Message *message) { getConversationManager().post(message); };
   Conversation* getConversation(ConversationHandle convHandle) { return getConversationManager().getConversation(convHandle); };

   std::shared_ptr<resip::ConfigParse> getConfig() { return mConversationManager.getConfig(); };

private:
   ConversationManager& mConversationManager;

   std::shared_ptr<flowmanager::RTCPEventLoggingHandler> mRTCPEventLoggingHandler;

   friend class ConversationManager;
   /* Called periodically in the event loop to give the MediaStackAdapter
      the opportunity to do any pending work */
   virtual void process() = 0;

   friend class UserAgent;
   virtual void initializeDtlsFactory(const resip::Data& defaultAoR) = 0;

   friend class OutputBridgeMixWeightsCmd;
   virtual void outputBridgeMatrixImpl(ConversationHandle convHandle = 0) = 0;
};

}

#endif


/* ====================================================================

 Copyright (c) 2021-2022, SIP Spectrum, Inc. www.sipspectrum.com
 Copyright (c) 2021-2022, Daniel Pocock https://danielpocock.com
 Copyright (c) 2021-2022, Software Freedom Institute SA https://softwarefreedom.institute
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
