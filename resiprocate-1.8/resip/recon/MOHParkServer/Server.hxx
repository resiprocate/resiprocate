#if !defined(Server_hxx)
#define Server_hxx

#include "ConfigParser.hxx"
#include "MOHManager.hxx"
#include "ParkManager.hxx"
#include "ActiveCallInfo.hxx"
#include "../UserAgent.hxx"
#include "../HandleTypes.hxx"

#ifdef WIN32
   #define sleepMs(t) Sleep(t)
#else
   #define sleepMs(t) usleep(t*1000)
#endif

#define MAXPARKTIMEOUT 1

namespace mohparkserver
{
class WebAdminThread;
class WebAdmin;

class Server : public recon::ConversationManager
{
public:
   Server(ConfigParser& config);  
   virtual ~Server();

   // Called in contructor, but can be used to modify logging at runtime
   void initializeResipLogging(unsigned int maxByteCount, const resip::Data& level, const resip::Data& resipFilename);

   /**
     Starts the recon thread(s).
     
     @note This should be called before calling process() in a loop
   */
   void startup(); 

   /**
     This should be called in a loop to give process cycles to the server.

     @param timeoutMs Will return after timeoutMs if nothing to do.
                      Application can do some work, but should call
                      process again ASAP.
   */
   void process(int timeoutMs); // call this in a loop

   /**
     Used to initiate a shutdown of the server.  This function blocks 
     until the shutdown is complete.  

     @note There should not be an active process request when this 
           is called.
   */
   void shutdown();

   recon::UserAgent* getMyUserAgent() { return mMyUserAgent; }
   MOHManager& getMOHManager() { return mMOHManager; }
   ParkManager& getParkManager() { return mParkManager; }
   void getActiveCallsInfo(CallInfoList& callInfos);

   // Timer handler
   virtual void onMaxParkTimeout(recon::ParticipantHandle participantHandle);

   // Configuration Container
   ConfigParser& mConfig;

protected:
   // Conversation Manager Handlers
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

private:
   friend class MOHManager;
   friend class ParkManager;
   void buildSessionCapabilities(resip::SdpContents& sessionCaps);

   bool mIsV6Avail;
   recon::UserAgent* mMyUserAgent;
   resip::SharedPtr<recon::UserAgentMasterProfile> mUserAgentMasterProfile;
   MOHManager mMOHManager;
   ParkManager mParkManager;
   WebAdmin* mWebAdmin;
   WebAdminThread* mWebAdminThread;
};
 
}

#endif

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

