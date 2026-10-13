#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstring>
#include <deque>
#include <iostream>
#include <memory>
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
// certificate fingerprint and agree an SRTP profile, that a failed handshake leaves
// nothing on this thread's OpenSSL error queue, and that a handshake isn't disturbed
// by an error an earlier call left there.

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

int
main(int, char**)
{
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
               pair.mServer->getRemoteFingerprint(remote) && strcmp(remote, clientFingerprint) == 0);
         check("handshake: the client sees the server's certificate",
               pair.mClient->getRemoteFingerprint(remote) && strcmp(remote, serverFingerprint) == 0);
         check("handshake: the client accepts the server's fingerprint",
               pair.mClient->checkFingerprint(serverFingerprint, strlen(serverFingerprint)));
         check("handshake: the server accepts the client's fingerprint",
               pair.mServer->checkFingerprint(clientFingerprint, strlen(clientFingerprint)));
         check("handshake: the client rejects a fingerprint that isn't the server's",
               !pair.mClient->checkFingerprint(clientFingerprint, strlen(clientFingerprint)));

         SRTP_PROTECTION_PROFILE* clientProfile = pair.mClient->getSrtpProfile();
         SRTP_PROTECTION_PROFILE* serverProfile = pair.mServer->getSrtpProfile();
         check("handshake: an SRTP profile was agreed",
               clientProfile && serverProfile && clientProfile->id == serverProfile->id);
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
