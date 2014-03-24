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

using namespace resip;


namespace recon
{

class B2BCallManager : public MyConversationManager
{
public:

   B2BCallManager(MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate, ReConServerConfig& config);

   virtual void onDtmfEvent(ParticipantHandle partHandle, int dtmf, int duration, bool up);
   virtual void onIncomingParticipant(ParticipantHandle partHandle, const SipMessage& msg, bool autoAnswer, ConversationProfile& conversationProfile);
   virtual void onParticipantTerminated(ParticipantHandle partHandle, unsigned int statusCode);
   virtual void onParticipantProceeding(ParticipantHandle partHandle, const SipMessage& msg);
   virtual void onParticipantAlerting(ParticipantHandle partHandle, const SipMessage& msg);
   virtual void onParticipantConnected(ParticipantHandle partHandle, const SipMessage& msg);

protected:
   struct B2BCall
   {
      ConversationHandle conv;
      ParticipantHandle a;
      ParticipantHandle b;
   };

   Data mB2BUANextHop;
   std::vector<std::auto_ptr<resip::ExtensionHeader> > mReplicatedHeaders;

   std::map<ConversationHandle,SharedPtr<B2BCall> > mCallsByConversation;
   std::map<ParticipantHandle,SharedPtr<B2BCall> > mCallsByParticipant;
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

