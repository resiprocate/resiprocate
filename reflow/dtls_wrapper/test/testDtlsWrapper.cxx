#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cctype>
#include <cstring>
#include <deque>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/srtp.h>
#include <openssl/x509.h>

#include "rutil/Data.hxx"

#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"
#include "DtlsTimer.hxx"
#include "CreateCert.hxx"

using namespace dtls;

// DTLS handshakes between a client and a server DtlsSocket, run in memory: each
// side's output is handed to the other.  Checks that both sides see each other's
// certificate fingerprint and agree an SRTP profile, that SDP fingerprints are checked
// properly, that SRTP is set up with the negotiated profile, that a failed handshake
// leaves nothing on this thread's OpenSSL error queue, and that a handshake isn't
// disturbed by an error an earlier call left there.

static int failures = 0;

static void
check(const char* what, bool cond)
{
   std::cerr << (cond ? "ok   " : "FAIL ") << what << std::endl;
   if (!cond) ++failures;
}

typedef std::vector<unsigned char> Datagram;

class PipeContext : public DtlsSocketContext
{
public:
   void write(const unsigned char* data, unsigned int len) override { mOut.push_back(Datagram(data, data + len)); }
   void handshakeCompleted() override { mCompleted = true; }
   void handshakeFailed(const char*) override { mFailed = true; }

   std::deque<Datagram> mOut;
   bool mCompleted = false;
   bool mFailed = false;
};

// Keeps the timers so they can be released at the end: an invalidated timer deletes
// itself when fired, and the sockets invalidate theirs when they are destroyed.
// (TestTimerContext only keeps the latest timer, so can't be shared by several sockets.)
class CollectingTimerContext : public DtlsTimerContext
{
public:
   ~CollectingTimerContext() override
   {
      for (DtlsTimer* timer : mTimers) fire(timer);
   }
   void addTimer(DtlsTimer* timer, unsigned int) override { mTimers.push_back(timer); }

   std::vector<DtlsTimer*> mTimers;
};

struct Pair
{
   Pair(DtlsFactory& clientFactory, DtlsFactory& serverFactory)
   {
      std::unique_ptr<PipeContext> clientContext(new PipeContext);
      std::unique_ptr<PipeContext> serverContext(new PipeContext);
      mClientContext = clientContext.get();
      mServerContext = serverContext.get();
      mClient.reset(clientFactory.createClient(std::move(clientContext)));
      mServer.reset(serverFactory.createServer(std::move(serverContext)));
   }

   // Starts the handshake and passes each side's output to the other until neither
   // has anything more to say
   void run()
   {
      mClient->startClient();
      for (int i = 0; i < 50 && (!mClientContext->mOut.empty() || !mServerContext->mOut.empty()); i++)
      {
         while (!mClientContext->mOut.empty())
         {
            Datagram d = mClientContext->mOut.front();
            mClientContext->mOut.pop_front();
            mServer->handlePacketMaybe(d.data(), d.size());
         }
         while (!mServerContext->mOut.empty())
         {
            Datagram d = mServerContext->mOut.front();
            mServerContext->mOut.pop_front();
            mClient->handlePacketMaybe(d.data(), d.size());
         }
      }
   }

   bool completed() const
   {
      return mClientContext->mCompleted && mServerContext->mCompleted &&
             !mClientContext->mFailed && !mServerContext->mFailed;
   }

   std::unique_ptr<DtlsSocket> mClient;
   std::unique_ptr<DtlsSocket> mServer;
   PipeContext* mClientContext;
   PipeContext* mServerContext;
};

// Sets up SRTP from both sockets' DTLS sessions and sends an RTP packet from sender to
// receiver.  tagLength is set to the bytes SRTP protection added.
static bool
srtpRoundTrip(DtlsSocket& sender, DtlsSocket& receiver, int& tagLength)
{
   srtp_policy_t senderOut, senderIn, receiverOut, receiverIn;
   if (!sender.createSrtpSessionPolicies(senderOut, senderIn))
   {
      return false;
   }
   if (!receiver.createSrtpSessionPolicies(receiverOut, receiverIn))
   {
      delete[] senderOut.key;
      delete[] senderIn.key;
      return false;
   }

   srtp_t out = nullptr;
   srtp_t in = nullptr;
   bool ok = srtp_create(&out, &senderOut) == srtp_err_status_ok &&
             srtp_create(&in, &receiverIn) == srtp_err_status_ok;
   delete[] senderOut.key;
   delete[] senderIn.key;
   delete[] receiverOut.key;
   delete[] receiverIn.key;

   if (ok)
   {
      // RTP version 2, payload type 0, sequence number 1, timestamp 0, SSRC 0x12345678
      const unsigned char header[12] = { 0x80, 0x00, 0x00, 0x01, 0, 0, 0, 0, 0x12, 0x34, 0x56, 0x78 };
      const char payload[] = "dtls-srtp payload";
      const int plainLength = (int)(sizeof(header) + sizeof(payload));
      unsigned char packet[sizeof(header) + sizeof(payload) + SRTP_MAX_TRAILER_LEN];
      memcpy(packet, header, sizeof(header));
      memcpy(packet + sizeof(header), payload, sizeof(payload));

      int length = plainLength;
      ok = srtp_protect(out, packet, &length) == srtp_err_status_ok;
      tagLength = length - plainLength;
      ok = ok && memcmp(packet + sizeof(header), payload, sizeof(payload)) != 0 &&
           srtp_unprotect(in, packet, &length) == srtp_err_status_ok &&
           length == plainLength &&
           memcmp(packet + sizeof(header), payload, sizeof(payload)) == 0;
   }

   if (out) srtp_dealloc(out);
   if (in) srtp_dealloc(in);
   return ok;
}

int
main(int, char**)
{
   srtp_init();

   X509* clientCert = nullptr;
   X509* serverCert = nullptr;
   EVP_PKEY* clientKey = nullptr;
   EVP_PKEY* serverKey = nullptr;

   // RSA certificates, as FlowManager creates
   createCert(resip::Data("sip:client@example.com"), 365, 2048, clientCert, clientKey);
   createCert(resip::Data("sip:server@example.com"), 365, 2048, serverCert, serverKey);
   bool haveCerts = clientCert && clientKey && serverCert && serverKey;
   check("test certificates could be created", haveCerts);
   if (!haveCerts)
   {
      return -1;
   }

   {
      DtlsFactory clientFactory(std::unique_ptr<DtlsTimerContext>(new CollectingTimerContext), clientCert, clientKey);
      DtlsFactory serverFactory(std::unique_ptr<DtlsTimerContext>(new CollectingTimerContext), serverCert, serverKey);
      char clientFingerprint[EVP_MAX_MD_SIZE * 3];
      char serverFingerprint[EVP_MAX_MD_SIZE * 3];
      clientFactory.getMyCertFingerprint(clientFingerprint);
      serverFactory.getMyCertFingerprint(serverFingerprint);

      // A. A plain handshake
      {
         ERR_clear_error();
         Pair pair(clientFactory, serverFactory);
         pair.run();
         check("handshake: completed on both sides", pair.completed());

         char remote[EVP_MAX_MD_SIZE * 3];
         check("handshake: the server sees the client's certificate",
               pair.mServer->getRemoteFingerprint("sha-256", remote, sizeof(remote)) && strcmp(remote, clientFingerprint) == 0);
         check("handshake: the client sees the server's certificate",
               pair.mClient->getRemoteFingerprint("sha-256", remote, sizeof(remote)) && strcmp(remote, serverFingerprint) == 0);
         check("handshake: the client accepts the server's fingerprint",
               pair.mClient->checkFingerprint("sha-256", serverFingerprint));
         check("handshake: the server accepts the client's fingerprint",
               pair.mServer->checkFingerprint("SHA-256", clientFingerprint));
         check("handshake: the client rejects a fingerprint that isn't the server's",
               !pair.mClient->checkFingerprint("sha-256", clientFingerprint));

         SRTP_PROTECTION_PROFILE* clientProfile = pair.mClient->getSrtpProfile();
         SRTP_PROTECTION_PROFILE* serverProfile = pair.mServer->getSrtpProfile();
         check("handshake: an SRTP profile was agreed",
               clientProfile && serverProfile && clientProfile->id == serverProfile->id);
      }

      // E. SDP fingerprints (RFC 8122) are checked with the hash function they name, as a
      //    whole, with hex digits in either case
      {
         Pair pair(clientFactory, serverFactory);
         pair.run();
         DtlsSocket& client = *pair.mClient;

         char sha1[EVP_MAX_MD_SIZE * 3];
         char sha512[EVP_MAX_MD_SIZE * 3];
         bool computed = DtlsSocket::computeFingerprint(serverCert, EVP_sha1(), sha1, sizeof(sha1)) &&
                         DtlsSocket::computeFingerprint(serverCert, EVP_sha512(), sha512, sizeof(sha512));
         check("fingerprint: SHA-1 and SHA-512 fingerprints computed", computed);
         char remote[EVP_MAX_MD_SIZE * 3];
         check("fingerprint: the remote fingerprint can be read with the SDP's hash function",
               client.getRemoteFingerprint("sha-1", remote, sizeof(remote)) && strcmp(remote, sha1) == 0 &&
               !client.getRemoteFingerprint("md5", remote, sizeof(remote)) &&
               !client.getRemoteFingerprint("sha-512", remote, DtlsSocket::FingerprintSize));
         check("fingerprint: a SHA-1 fingerprint is accepted", client.checkFingerprint("sha-1", sha1));
         check("fingerprint: a SHA-512 fingerprint is accepted", client.checkFingerprint("sha-512", sha512));
         check("fingerprint: a fingerprint is checked with the hash function named",
               !client.checkFingerprint("sha-256", sha1) && !client.checkFingerprint("sha-1", serverFingerprint));

         std::string lower(serverFingerprint);
         for (char& c : lower) c = (char)tolower((unsigned char)c);
         check("fingerprint: lowercase hex digits are accepted", client.checkFingerprint("sha-256", lower.c_str()));

         std::string prefix(serverFingerprint, strlen(serverFingerprint) / 2);
         check("fingerprint: a prefix of the fingerprint is rejected", !client.checkFingerprint("sha-256", prefix.c_str()));
         std::string longer = std::string(serverFingerprint) + ":00";
         check("fingerprint: a longer fingerprint is rejected", !client.checkFingerprint("sha-256", longer.c_str()));
         check("fingerprint: an empty fingerprint is rejected", !client.checkFingerprint("sha-256", ""));

         check("fingerprint: md5 and unknown hash functions are rejected",
               !DtlsSocket::isFingerprintHashFunctionSupported("md5") &&
               !DtlsSocket::isFingerprintHashFunctionSupported("sha-3") &&
               !client.checkFingerprint("md5", serverFingerprint));

         char tooSmall[DtlsSocket::FingerprintSize];
         check("fingerprint: computing into too small a buffer fails",
               !DtlsSocket::computeFingerprint(serverCert, EVP_sha512(), tooSmall, sizeof(tooSmall)));
      }

      // F. The SRTP sessions use the profile DTLS negotiated: SHA1_32 has a 4 byte RTP auth tag
      {
         struct { const char* name; int tagLength; } profiles[] = {
            { "SRTP_AES128_CM_SHA1_80", 10 },
            { "SRTP_AES128_CM_SHA1_32", 4 },
         };
         for (const auto& profile : profiles)
         {
            DtlsFactory profileFactory(std::unique_ptr<DtlsTimerContext>(new CollectingTimerContext), clientCert, clientKey);
            profileFactory.setSrtpProfiles(profile.name);
            Pair pair(profileFactory, serverFactory);
            pair.run();
            std::string what = std::string("srtp ") + profile.name + ": ";
            SRTP_PROTECTION_PROFILE* negotiated = pair.completed() ? pair.mClient->getSrtpProfile() : nullptr;
            check((what + "negotiated").c_str(), negotiated && strcmp(negotiated->name, profile.name) == 0);
            if (!negotiated) continue;

            int tagLength = -1;
            check((what + "client to server").c_str(), srtpRoundTrip(*pair.mClient, *pair.mServer, tagLength));
            check((what + "auth tag length").c_str(), tagLength == profile.tagLength);
            check((what + "server to client").c_str(), srtpRoundTrip(*pair.mServer, *pair.mClient, tagLength));
         }

         // AES-GCM needs libsrtp built with GCM: without it the policies can't be created, rather than asserting
         DtlsFactory gcmFactory(std::unique_ptr<DtlsTimerContext>(new CollectingTimerContext), clientCert, clientKey);
         gcmFactory.setSrtpProfiles("SRTP_AEAD_AES_128_GCM");
         DtlsFactory gcmServerFactory(std::unique_ptr<DtlsTimerContext>(new CollectingTimerContext), serverCert, serverKey);
         gcmServerFactory.setSrtpProfiles("SRTP_AEAD_AES_128_GCM");
         Pair pair(gcmFactory, gcmServerFactory);
         pair.run();
         srtp_crypto_policy_t gcmPolicy;
         bool libsrtpHasGcm = srtp_crypto_policy_set_from_profile_for_rtp(&gcmPolicy, srtp_profile_aead_aes_128_gcm) == srtp_err_status_ok;
         int tagLength = -1;
         check(libsrtpHasGcm ? "srtp SRTP_AEAD_AES_128_GCM: client to server" : "srtp SRTP_AEAD_AES_128_GCM: refused, libsrtp has no GCM",
               pair.completed() && srtpRoundTrip(*pair.mClient, *pair.mServer, tagLength) == libsrtpHasGcm);
      }

      // B. An error an earlier OpenSSL call left on this thread's queue (here a failed
      //    certificate parse) must not be taken as this handshake's error.  OpenSSL's
      //    handshake state machine currently clears the queue itself; DtlsSocket clears it
      //    too, as OpenSSL documents is required
      {
         ERR_clear_error();
         const unsigned char junk[] = { 0x30, 0x80, 0x01 };
         const unsigned char* p = junk;
         X509* bad = d2i_X509(nullptr, &p, sizeof(junk));
         check("stale error: an error is queued", bad == nullptr && ERR_peek_error() != 0);

         Pair pair(clientFactory, serverFactory);
         pair.run();
         check("stale error: handshake still completed on both sides", pair.completed());
      }

      // C. A failed handshake doesn't leave its error for whatever runs next on this thread.
      //    The client offers only an ECDSA cipher suite and the server's certificate is RSA,
      //    so there is no cipher they share.
      {
         DtlsFactory ecdsaOnlyFactory(std::unique_ptr<DtlsTimerContext>(new CollectingTimerContext), clientCert, clientKey);
         ecdsaOnlyFactory.setCipherSuites("ECDHE-ECDSA-AES128-GCM-SHA256");
         ERR_clear_error();
         {
            Pair pair(ecdsaOnlyFactory, serverFactory);
            pair.run();
            check("failed handshake: reported as failed", pair.mServerContext->mFailed);
            check("failed handshake: nothing left on the error queue", ERR_peek_error() == 0);
         }
      }

      // D. And the next handshake on this thread works
      {
         Pair pair(clientFactory, serverFactory);
         pair.run();
         check("after a failed handshake: the next one completes", pair.completed());
      }
   }

   X509_free(clientCert);
   X509_free(serverCert);
   EVP_PKEY_free(clientKey);
   EVP_PKEY_free(serverKey);

   std::cerr << (failures == 0 ? "\nall checks passed\n" : "\nFAILURES\n");
   return failures == 0 ? 0 : -1;
}

/* ====================================================================
 *
 * Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
 * Copyright (c) 2007-2008, Eric Rescorla and Derek MacDonald
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
 */
