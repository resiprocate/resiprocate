#ifndef B2BCALLMANAGER_HXX
#define B2BCALLMANAGER_HXX

#include <os/OsIntTypes.h>

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include <memory>

#include <resip/stack/ExtensionHeader.hxx>
#include <rutil/Data.hxx>
#include <recon/ConversationManager.hxx>

#include "reConServerConfig.hxx"
#include "MyConversationManager.hxx"

namespace reconserver
{

class B2BCallManager : public MyConversationManager
{
public:

   B2BCallManager(recon::ConversationManager::MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate, ReConServerConfig& config);

   virtual void onDtmfEvent(recon::ParticipantHandle partHandle, int dtmf, int duration, bool up);
   virtual void onIncomingParticipant(recon::ParticipantHandle partHandle, const resip::SipMessage& msg, bool autoAnswer, recon::ConversationProfile& conversationProfile);
   virtual void onParticipantTerminated(recon::ParticipantHandle partHandle, unsigned int statusCode);
   virtual void onParticipantProceeding(recon::ParticipantHandle partHandle, const resip::SipMessage& msg);
   virtual void onParticipantAlerting(recon::ParticipantHandle partHandle, const resip::SipMessage& msg);
   virtual void onParticipantConnected(recon::ParticipantHandle partHandle, const resip::SipMessage& msg);

   resip::SharedPtr<recon::ConversationProfile> getIncomingConversationProfile(const resip::SipMessage& msg, resip::SharedPtr<recon::ConversationProfile> defaultProfile);

   void loadUserCredentials(resip::Data filename);

protected:
   resip::SharedPtr<recon::ConversationProfile> getInternalConversationProfile();
   virtual bool isSourceInternal(const resip::SipMessage& msg);

   struct UserCredentials
   {
      resip::Data mUsername;
      resip::Data mPassword;
   };

   struct B2BCall
   {
      recon::ConversationHandle conv;
      recon::ParticipantHandle a;
      recon::ParticipantHandle b;
   };

   std::vector<resip::Data> mInternalHosts;
   std::vector<resip::Data> mInternalTLSNames;
   bool mInternalAllPrivate;
   resip::Data mInternalMediaAddress;
   std::vector<resip::Data> mReplicatedHeaders;

   std::map<resip::Data,UserCredentials> mUsers;

   std::map<recon::ConversationHandle,resip::SharedPtr<B2BCall> > mCallsByConversation;
   std::map<recon::ParticipantHandle,resip::SharedPtr<B2BCall> > mCallsByParticipant;
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

