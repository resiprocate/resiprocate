#ifndef B2BCALLMANAGER_HXX
#define B2BCALLMANAGER_HXX

#include <os/OsIntTypes.h>

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include <memory>

#include "apps/reConServer/CredentialInfo.hxx"
#include <resip/stack/Dispatcher.hxx>
#include <resip/stack/ExtensionHeader.hxx>
#include <rutil/Data.hxx>
#include <rutil/Time.hxx>
#include <resip/recon/ConversationManager.hxx>

#include "reConServerConfig.hxx"
#include "MyConversationManager.hxx"

#include "soci.h"

namespace reconserver
{

class MyUserAgent;

class B2BCall
{
public:
   B2BCall(const recon::ConversationHandle& conv, const recon::ParticipantHandle& a, const resip::SipMessage& msg, const resip::Data& originZone, const resip::Data& destinationZone, const resip::Data& b2bCallID);

   void onConnect() { mConnect = resip::ResipClock::getTimeMs(); };
   void onFinish(const int responseCode = 200) { mFinish = resip::ResipClock::getTimeMs(); mResponseCode = responseCode; };

   const recon::ConversationHandle& conversation() { return mConversation; };
   const recon::ParticipantHandle& participantA() { return mPartA; };
   void setParticipantB(const recon::ParticipantHandle b) { mPartB = b; };
   const recon::ParticipantHandle& participantB() { return mPartB; };
   const recon::ParticipantHandle& peer(const recon::ParticipantHandle& partHandle);

   const resip::SharedPtr<resip::SipMessage> getInviteMessage() const { return mInviteMessage; };

   const resip::Data& getOriginZone() const { return mOriginZone; };
   const resip::Data& getDestinationZone() const { return mDestinationZone; };
   const resip::Data& getB2BCallID() const { return mB2BCallID; };
   const resip::Data& getCaller() const { return mCaller; };
   const resip::Data& getCallee() const { return mCallee; };
   int getResponseCode() const { return mResponseCode; };
   const uint64_t& getStart() const { return mStart; };
   const uint64_t& getConnect() const { return mConnect; };
   bool answered() const { return mConnect != 0; };
   const uint64_t& getFinish() const { return mFinish; };

private:
   const recon::ConversationHandle mConversation;
   const recon::ParticipantHandle mPartA;
   recon::ParticipantHandle mPartB;

   resip::SharedPtr<resip::SipMessage> mInviteMessage;

   const resip::Data mOriginZone;
   const resip::Data mDestinationZone;

   const resip::Data mB2BCallID;

   const resip::Data mCaller;
   const resip::Data mCallee;

   int mResponseCode;

   // times are in milliseconds since the UNIX epoch
   uint64_t mStart;
   uint64_t mConnect;
   uint64_t mFinish;
};

class B2BCallLogger
{
public:
   virtual void log(resip::SharedPtr<B2BCall> call) = 0;
};

class B2BCallManager : public MyConversationManager
{
public:

   B2BCallManager(recon::ConversationManager::MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate, ReConServerConfig& config, resip::SharedPtr<B2BCallLogger> b2bCallLogger = resip::SharedPtr<B2BCallLogger>());
   ~B2BCallManager();

   virtual void init(MyUserAgent& ua);

   virtual void onDtmfEvent(recon::ParticipantHandle partHandle, int dtmf, int duration, bool up);
   virtual void onIncomingParticipant(recon::ParticipantHandle partHandle, const resip::SipMessage& msg, bool autoAnswer, recon::ConversationProfile& conversationProfile);
   virtual void makeBLeg(resip::SharedPtr<B2BCall> call, CredentialInfo* ci);
   virtual void rejectCall(resip::SharedPtr<B2BCall> call);
   virtual void onParticipantTerminated(recon::ParticipantHandle partHandle, unsigned int statusCode);
   virtual void onParticipantProceeding(recon::ParticipantHandle partHandle, const resip::SipMessage& msg);
   virtual void onParticipantAlerting(recon::ParticipantHandle partHandle, const resip::SipMessage& msg);
   virtual void onParticipantConnected(recon::ParticipantHandle partHandle, const resip::SipMessage& msg);
   virtual void onParticipantRequestedHold(recon::ParticipantHandle partHandle, bool held);

   resip::SharedPtr<recon::ConversationProfile> getInternalConversationProfile();
   resip::SharedPtr<recon::ConversationProfile> getExternalConversationProfile();
   resip::SharedPtr<recon::ConversationProfile> getIncomingConversationProfile(const resip::SipMessage& msg, resip::SharedPtr<recon::ConversationProfile> defaultProfile);

   void loadUserCredentials(resip::Data filename);

protected:
   virtual bool isSourceInternal(const resip::SipMessage& msg);

   struct UserCredentials
   {
      resip::Data mUsername;
      resip::Data mPassword;
   };

   resip::SharedPtr<B2BCallLogger> mB2BCallLogger;
   std::vector<resip::Data> mInternalHosts;
   std::vector<resip::Data> mInternalTLSNames;
   bool mInternalAllPrivate;
   resip::Data mInternalMediaAddress;
   std::vector<resip::Data> mReplicatedHeaders;

   std::map<resip::Data,UserCredentials> mUsers;

   std::map<recon::ConversationHandle,resip::SharedPtr<B2BCall> > mCallsByConversation;
   std::map<recon::ParticipantHandle,resip::SharedPtr<B2BCall> > mCallsByParticipant;

   int mDbPoolSize;
   bool mDatabaseCredentialsHashed;
   resip::Data mDatabaseQueryUserCredential;
   resip::SharedPtr<soci::connection_pool> mPool;

   resip::SharedPtr<resip::Dispatcher> mDispatcher;

   static resip::ExtensionHeader h_X_CID;

};

}

#endif


/* ====================================================================
 *
 * Copyright 2014 Daniel Pocock http://danielpocock.com  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

