#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "resip/recon/UserAgent.hxx"
#include "resip/recon/ReconSubsystem.hxx"
#include "resip/recon/SipXMediaStackAdapter.hxx"

#include <signal.h>
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"

#include <atomic>
#include <chrono>
#include <cstring>
#include <functional>
#include <iostream>

// Calls between two recon user agents that negotiate DTLS-SRTP: each end has to take the
// remote fingerprint from the SDP and, if it's the active end, start the DTLS handshake.

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

static int failures = 0;

static void
check(const char* what, bool ok)
{
   cerr << (ok ? "   ok: " : "   FAILED: ") << what << endl;
   if (!ok)
   {
      ++failures;
   }
}

// Counts what reflow logs about DTLS-SRTP on RTP flows: recon has nothing else that shows it
class DtlsSrtpCounter : public ExternalLogger
{
public:
   bool operator()(Log::Level level, const Subsystem&, const Data&, const char* file, int,
                   const Data& message, const Data&, const Data&) override
   {
      if (file && strstr(file, "FlowDtlsSocketContext") && strstr(message.c_str(), "ComponentId=1"))
      {
         if (strstr(message.c_str(), "Remote certificate matches the SDP fingerprint"))
         {
            ++mSrtpSetUp;
         }
         else if (strstr(message.c_str(), "Handshake failed") || strstr(message.c_str(), "does not match the SDP fingerprint"))
         {
            ++mFailed;
         }
      }
      return level <= Log::Warning;
   }
   void reset() { mSrtpSetUp = 0; mFailed = 0; }

   std::atomic<int> mSrtpSetUp{0};
   std::atomic<int> mFailed{0};
};
static DtlsSrtpCounter dtlsSrtp;

class TestConversationManager : public ConversationManager
{
public:
   TestConversationManager() :
      ConversationManager(make_shared<SipXMediaStackAdapter>(*this, false, SipXMediaStackAdapter::sipXConversationMediaInterfaceMode)) {}

   // Answers calls
   void onIncomingParticipant(ParticipantHandle partHandle, const SipMessage&, bool, ConversationProfile&) override
   {
      ConversationHandle convHandle = createConversation();
      addParticipant(convHandle, partHandle);
      answerParticipant(partHandle);
   }
   void onParticipantConnected(ParticipantHandle, const SipMessage&) override { mConnected = true; }
   void onParticipantTerminated(ParticipantHandle, unsigned int) override { mTerminated = true; }

   void onRequestOutgoingParticipant(ParticipantHandle, const SipMessage&, ConversationProfile&) override {}
   void onConversationDestroyed(ConversationHandle) override {}
   void onParticipantDestroyed(ParticipantHandle) override {}
   void onRelatedConversation(ConversationHandle, ParticipantHandle, ConversationHandle, ParticipantHandle) override {}
   void onParticipantAlerting(ParticipantHandle, const SipMessage&) override {}
   void onParticipantRedirectSuccess(ParticipantHandle) override {}
   void onParticipantRedirectFailure(ParticipantHandle, unsigned int) override {}
   void onDtmfEvent(ParticipantHandle, int, int, bool) override {}
   void onParticipantRequestedHold(ParticipantHandle, bool) override {}

   bool mConnected = false;
   bool mTerminated = false;
};

static std::shared_ptr<UserAgentMasterProfile>
createUserAgentMasterProfile(int sipPort, unsigned short rtpPortMin)
{
   auto profile = std::make_shared<UserAgentMasterProfile>();
   profile->statisticsManagerEnabled() = false;
   profile->clearSupportedMimeTypes();
   profile->addSupportedMimeType(INVITE, Mime("application", "sdp"));
   profile->addSupportedMimeType(UPDATE, Mime("application", "sdp"));
   profile->addTransport(UDP, sipPort, V4, StunDisabled, "127.0.0.1");
   // Each user agent has its own RTP ports: sockets don't share them
   profile->rtpPortRangeMin() = rtpPortMin;
   profile->rtpPortRangeMax() = rtpPortMin + 39;
   return profile;
}

static std::shared_ptr<ConversationProfile>
createConversationProfile(std::shared_ptr<UserAgentMasterProfile> profile, const NameAddr& from, int rtpPort,
                          ConversationProfile::SecureMediaMode secureMediaMode, bool secureMediaRequired)
{
   auto conversationProfile = std::make_shared<ConversationProfile>(profile);
   conversationProfile->setDefaultRegistrationTime(0);
   conversationProfile->setDefaultFrom(from);

   Data address("127.0.0.1");
   SdpContents::Session::Origin origin("-", 0, 0, SdpContents::IP4, address);
   SdpContents::Session session(0, origin, "-");
   session.connection() = SdpContents::Session::Connection(SdpContents::IP4, address);
   session.addTime(SdpContents::Session::Time(0, 0));
   SdpContents::Session::Medium medium("audio", rtpPort, 0, "RTP/AVP");
   SdpContents::Session::Codec g711ucodec("PCMU", 8000);
   g711ucodec.payloadType() = 0;
   medium.addCodec(g711ucodec);
   medium.addAttribute("ptime", Data(20));
   medium.addAttribute("sendrecv");
   session.addMedium(medium);
   SdpContents sessionCaps;
   sessionCaps.session() = session;
   conversationProfile->sessionCaps() = sessionCaps;

   conversationProfile->mediaEndpointMode() = ConversationProfile::Base;
   conversationProfile->delayedMediaOutboundMode() = false;
   conversationProfile->secureMediaMode() = secureMediaMode;
   conversationProfile->secureMediaRequired() = secureMediaRequired;
   return conversationProfile;
}

// Alice calls Bob, who answers: returns whether both ends set up DTLS-SRTP for RTP, with the
// fingerprint each was given in the SDP
static bool
dtlsSrtpCall(int basePort, ConversationProfile::SecureMediaMode aliceMode, bool aliceRequired,
             ConversationProfile::SecureMediaMode bobMode, bool bobRequired)
{
   NameAddr aliceUri("sip:alice@127.0.0.1:" + Data(basePort));
   NameAddr bobUri("sip:bob@127.0.0.1:" + Data(basePort + 1));
   bool connected = false;
   bool setUp = false;
   {
      auto profileAlice = createUserAgentMasterProfile(basePort, basePort + 10);
      TestConversationManager alice;
      UserAgent aliceUa(&alice, profileAlice);
      aliceUa.addConversationProfile(createConversationProfile(profileAlice, aliceUri, basePort + 10, aliceMode, aliceRequired));

      auto profileBob = createUserAgentMasterProfile(basePort + 1, basePort + 50);
      TestConversationManager bob;
      UserAgent bobUa(&bob, profileBob);
      bobUa.addConversationProfile(createConversationProfile(profileBob, bobUri, basePort + 50, bobMode, bobRequired));

      aliceUa.startup();
      bobUa.startup();

      auto processUntil = [&](const std::function<bool()>& cond, int timeoutMs)
      {
         const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
         while (!cond() && std::chrono::steady_clock::now() < deadline)
         {
            aliceUa.process(10);
            bobUa.process(10);
         }
         return cond();
      };

      dtlsSrtp.reset();
      ConversationHandle convHandle = alice.createConversation();
      alice.createRemoteParticipant(convHandle, bobUri, ConversationManager::ForkSelectAutomatic);
      connected = processUntil([&]() { return alice.mConnected; }, 5000);  // (only the calling end is told)
      setUp = connected && processUntil([]() { return dtlsSrtp.mSrtpSetUp >= 2 || dtlsSrtp.mFailed > 0; }, 5000) &&
              dtlsSrtp.mFailed == 0;

      alice.destroyConversation(convHandle);
      processUntil([&]() { return alice.mTerminated && bob.mTerminated; }, 2000);
      aliceUa.shutdown();
      bobUa.shutdown();
   }
   check("the call is answered", connected);
   return setUp;
}

int
main(int argc, char** argv)
{
#ifndef _WIN32
   signal(SIGPIPE, SIG_IGN);
#endif
   Log::initialize(Log::Cout, Log::Info, argv[0], 0, &dtlsSrtp);
   initNetwork();

#ifdef USE_SSL
   cerr << "-- DTLS-SRTP required: UDP/TLS/RTP/SAVP" << endl;
   check("DTLS-SRTP required: both ends set up SRTP from DTLS",
         dtlsSrtpCall(33100, ConversationProfile::SrtpDtls, true, ConversationProfile::SrtpDtls, true));

   // Alice offers RTP/AVP with a fingerprint and UDP/TLS/RTP/SAVP as a potential configuration
   // (tcap/pcfg), which Bob's answer takes
   cerr << "-- DTLS-SRTP optional: RTP/AVP offer, potential configuration UDP/TLS/RTP/SAVP" << endl;
   check("DTLS-SRTP optional: both ends set up SRTP from DTLS",
         dtlsSrtpCall(33200, ConversationProfile::SrtpDtls, false, ConversationProfile::SrtpDtls, false));

   // Bob, without DTLS-SRTP as his secure media mode, still accepts it if it's offered
   cerr << "-- DTLS-SRTP optional, offered to an end that doesn't ask for it" << endl;
   check("DTLS-SRTP offered: both ends set up SRTP from DTLS",
         dtlsSrtpCall(33300, ConversationProfile::SrtpDtls, false, ConversationProfile::NoSecureMedia, false));
#else
   cerr << "built without SSL: nothing to test" << endl;
#endif

   cerr << (failures ? "FAILED" : "all checks passed") << endl;
   return failures ? 1 : 0;
}


/* ====================================================================

 Copyright (c) 2026, SIP Spectrum, Inc. https://www.sipspectrum.com
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
