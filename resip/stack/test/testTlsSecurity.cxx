#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <atomic>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>

#ifdef USE_SSL

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/ssl/Security.hxx"
#include "resip/stack/ssl/TlsTransport.hxx"
#include "rutil/Data.hxx"
#include "rutil/Log.hxx"
#include "rutil/Socket.hxx"

#include "TlsTestSupport.hxx"
#include "testPortOffset.hxx"

using namespace resip;
using namespace std;
using namespace TlsTest;

// Certificate name matching, subjectAltName parsing, reloading a TLS
// transport's certificate, and session resumption.

static int failures = 0;

static void
check(const char* what, bool cond)
{
   cerr << (cond ? "ok   " : "FAIL ") << what << endl;
   if (!cond) ++failures;
}

static void
checkMatch(const char* certificateName, const char* domainName, bool expected)
{
   const Data what = Data(expected ? "matches:      " : "doesn't match: ") +
                     certificateName + " / " + domainName;
   check(what.c_str(), (BaseSecurity::matchHostName(certificateName, domainName) != 0) == expected);
}

static void
testMatchHostName()
{
   cerr << "-- matchHostName, wildcards not allowed" << endl;
   BaseSecurity::setAllowWildcardCertificates(false);
   checkMatch("example.com", "example.com", true);
   checkMatch("EXAMPLE.com", "example.COM", true);
   checkMatch("*.example.com", "a.example.com", false);
   checkMatch("example.com", "other.com", false);

   cerr << "-- matchHostName, wildcards allowed (RFC 6125 section 6.4.3)" << endl;
   BaseSecurity::setAllowWildcardCertificates(true);
   checkMatch("example.com", "example.com", true);
   checkMatch("*.example.com", "a.example.com", true);
   checkMatch("*.example.com", "A.Example.COM", true);
   checkMatch("*.example.com", "a.b.example.com", false);   // one label only
   checkMatch("*.example.com", "example.com", false);
   checkMatch("*.example.com", ".example.com", false);      // empty label
   checkMatch("*.com", "a.com", false);                     // whole top-level domain
   checkMatch("*.com.", "a.com.", false);
   checkMatch("*.0.0.1", "127.0.0.1", false);               // never an IP address
   checkMatch("f*.example.com", "foo.example.com", false);  // only a whole label
   checkMatch("pbx.attacker.com", "pbx", false);            // used to match

   // The old code wrote a NUL into the certificate name when the domain name
   // had no dot, corrupting the caller's (const) peer name
   const Data certificateName("pbx.attacker.com");
   BaseSecurity::matchHostName(certificateName, "pbx");
   check("certificate name is left unchanged",
         certificateName == "pbx.attacker.com" &&
         strlen(certificateName.c_str()) == certificateName.size());

   BaseSecurity::setAllowWildcardCertificates(false);
}

static void
testUnparsableUriSubjectAltName(X509* caCert, EVP_PKEY* caKey)
{
   cerr << "-- getCertNames with a subjectAltName URI that doesn't parse" << endl;
   EVP_PKEY* key = makeKey();
   // "urn:x:bad" fails to parse: a urn: NID needs 2 to 32 characters
   X509* cert = key ? makeCert(key, "names.test",
                               "URI:urn:x:bad, DNS:ok.test, URI:sip:uri.test",
                               caCert, caKey) : nullptr;
   check("certificate could be created", cert != nullptr);
   if (cert)
   {
      std::list<BaseSecurity::PeerName> names;
      bool threw = false;
      try
      {
         BaseSecurity::getCertNames(cert, names);
      }
      catch (...)
      {
         threw = true;
      }
      check("getCertNames doesn't throw", !threw);

      bool haveDns = false;
      bool haveUri = false;
      for (const BaseSecurity::PeerName& name : names)
      {
         haveDns = haveDns || name.mName == "ok.test";
         haveUri = haveUri || name.mName == "uri.test";
      }
      check("the names after the bad one are still found", haveDns && haveUri);
      check("nothing else is returned", names.size() == 2);
   }
   X509_free(cert);
   EVP_PKEY_free(key);
}

static const char* const serverCertFile = "testTlsSecurity_server_cert.pem";
static const char* const serverKeyFile = "testTlsSecurity_server_key.pem";
static const char* const clientCertFile = "testTlsSecurity_client_cert.pem";
static const char* const clientKeyFile = "testTlsSecurity_client_key.pem";

static int verifyCallbackCalls = 0;

// Server side verification of the client's certificate, through the callback
// set with TlsBaseTransport::setPeerCertificateVerificationCallback()
static int
countingVerifyCallback(X509_STORE_CTX* storeCtx, void* arg)
{
   ++*static_cast<int*>(arg);
   return X509_verify_cert(storeCtx);
}

// Opens a new connection (a new client transport each time) to the server
// with targetDomain as the name the client expects in the server's
// certificate.  Returns true if the server received the request.
static bool
connect(Security& security, TlsTransport& server, Fifo<TransactionMessage>& serverFifo,
        int serverPort, const char* targetDomain)
{
   static int nextClientPort = resipTestPort(5271);
   const int clientPort = nextClientPort++;

   // The client has a domain and certificate of its own, which it presents
   // when the server asks for one
   Fifo<TransactionMessage> clientFifo;
   TlsTransport client(clientFifo, clientPort, V4, "127.0.0.1", security, "client.test",
                       SecurityTypes::SSLv23, nullptr, Compression::Disabled, 0,
                       SecurityTypes::None, false, clientCertFile, clientKeyFile);

   const Tuple dest("127.0.0.1", serverPort, V4, TLS, targetDomain);
   client.send(client.makeSendData(dest, makeRequest(targetDomain, clientPort, serverPort), "tid"));
   std::unique_ptr<SipMessage> received = receive(client, clientFifo, server, serverFifo);
   if (received)
   {
      const std::list<Data>& peerNames = received->getTlsPeerNames();
      check("server sees the client certificate's name",
            peerNames.size() == 1 && peerNames.front() == "client.test");
   }
   return received != nullptr;
}

static void
testReload(Security& security, X509* caCert, EVP_PKEY* caKey)
{
   cerr << "-- reloading the server's certificate" << endl;
   EVP_PKEY* keyA = makeKey();
   EVP_PKEY* keyB = makeKey();
   EVP_PKEY* otherKey = makeKey();
   EVP_PKEY* clientKey = makeKey();
   X509* certA = keyA ? makeCert(keyA, "a.test", "DNS:a.test", caCert, caKey) : nullptr;
   X509* certB = keyB ? makeCert(keyB, "b.test", "DNS:b.test", caCert, caKey) : nullptr;
   X509* clientCert = clientKey ? makeCert(clientKey, "client.test", "DNS:client.test", caCert, caKey) : nullptr;
   const bool haveCerts = certA && certB && otherKey && clientCert &&
                          writeCertFile(serverCertFile, certA) && writeKeyFile(serverKeyFile, keyA) &&
                          writeCertFile(clientCertFile, clientCert) && writeKeyFile(clientKeyFile, clientKey);
   check("test certificates could be created", haveCerts);

   if (haveCerts)
   {
      const int serverPort = resipTestPort(5261);
      // Asks clients for a certificate, so the verify callback runs on every
      // new connection
      Fifo<TransactionMessage> serverFifo;
      TlsTransport server(serverFifo, serverPort, V4, "127.0.0.1", security, "reload.test",
                          SecurityTypes::SSLv23, nullptr, Compression::Disabled, 0,
                          SecurityTypes::Optional, false, serverCertFile, serverKeyFile);
      server.setPeerCertificateVerificationCallback(SecurityTypes::OpenSSL,
                                                    (void*)&countingVerifyCallback,
                                                    &verifyCallbackCalls);

      check("before reloading: connection accepted with the first certificate",
            connect(security, server, serverFifo, serverPort, "a.test"));
      check("verify callback ran", verifyCallbackCalls == 1);
      SSL_CTX* const original = server.getCtx();

      // A certificate file that can't be read
      {
         std::ofstream f(serverCertFile, std::ios::trunc);
         f << "not a certificate" << endl;
      }
      server.onReload();
      check("unreadable certificate: still using the previous SSL_CTX", server.getCtx() == original);
      check("unreadable certificate: new connections still accepted",
            connect(security, server, serverFifo, serverPort, "a.test"));

      // A private key that doesn't belong to the certificate
      writeCertFile(serverCertFile, certA);
      writeKeyFile(serverKeyFile, otherKey);
      server.onReload();
      check("mismatched private key: still using the previous SSL_CTX", server.getCtx() == original);
      check("mismatched private key: new connections still accepted",
            connect(security, server, serverFifo, serverPort, "a.test"));

      // A new, good certificate
      writeCertFile(serverCertFile, certB);
      writeKeyFile(serverKeyFile, keyB);
      server.onReload();
      check("good certificate: SSL_CTX replaced", server.getCtx() != original);
      const int callsBefore = verifyCallbackCalls;
      check("good certificate: new connections get the new certificate",
            connect(security, server, serverFifo, serverPort, "b.test"));
      check("good certificate: verify callback still runs after the SSL_CTX was replaced",
            verifyCallbackCalls == callsBefore + 1);
      check("good certificate: the old certificate is no longer presented",
            !connect(security, server, serverFifo, serverPort, "a.test"));
   }

   X509_free(clientCert);
   X509_free(certB);
   X509_free(certA);
   EVP_PKEY_free(clientKey);
   EVP_PKEY_free(otherKey);
   EVP_PKEY_free(keyB);
   EVP_PKEY_free(keyA);
   remove(serverCertFile);
   remove(serverKeyFile);
   remove(clientCertFile);
   remove(clientKeyFile);
}

static const char* const resumeCertFile = "testTlsSecurity_resume_cert.pem";
static const char* const resumeKeyFile = "testTlsSecurity_resume_key.pem";

// One handshake by a plain OpenSSL client, offering the session resume if
// there is one.  It blocks, so it runs on a thread of its own while the test
// drives the server; a receive timeout keeps it from blocking for ever.
static bool
opensslHandshake(SSL_CTX* ctx, int port, SSL_SESSION* resume, SSL_SESSION** session, bool* reused)
{
   const Data address = Data("127.0.0.1:") + Data(port);
   BIO* bio = BIO_new_connect(address.c_str());
   if (!bio || BIO_do_connect(bio) != 1)
   {
      BIO_free_all(bio);
      return false;
   }
   int fd = -1;
   BIO_get_fd(bio, &fd);
#ifdef WIN32
   DWORD timeout = 5000;
#else
   struct timeval timeout = { 5, 0 };
#endif
   setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

   SSL* ssl = SSL_new(ctx);
   SSL_set_bio(ssl, bio, bio);  // ssl owns bio from here
   if (resume)
   {
      SSL_set_session(ssl, resume);
   }
   const bool ok = SSL_connect(ssl) == 1;
   if (ok)
   {
      if (session) *session = SSL_get1_session(ssl);
      if (reused) *reused = SSL_session_reused(ssl) == 1;
      SSL_shutdown(ssl);
   }
   SSL_free(ssl);
   return ok;
}

// Runs handshake on another thread, driving server until it has finished
static bool
runWithServer(TlsTransport& server, const std::function<bool()>& handshake)
{
   std::atomic<bool> done(false);
   bool ok = false;
   std::thread client([&]() { ok = handshake(); done = true; });
   while (!done)
   {
      FdSet fdset;
      server.buildFdSet(fdset);
      fdset.selectMilliSeconds(10);
      server.process(fdset);
   }
   client.join();
   return ok;
}

static void
testSessionResumption(Security& security, X509* caCert, EVP_PKEY* caKey)
{
   cerr << "-- resuming a session with a server that asks for client certificates" << endl;
   EVP_PKEY* key = makeKey();
   X509* cert = key ? makeCert(key, "resume.test", "DNS:resume.test", caCert, caKey) : nullptr;
   const bool haveCerts = cert && writeCertFile(resumeCertFile, cert) && writeKeyFile(resumeKeyFile, key);
   check("test certificates could be created", haveCerts);

   if (haveCerts)
   {
      // Optional sets SSL_VERIFY_PEER, which is what needs the session id context
      const int serverPort = resipTestPort(5281);
      Fifo<TransactionMessage> serverFifo;
      TlsTransport server(serverFifo, serverPort, V4, "127.0.0.1", security, "resume.test",
                          SecurityTypes::SSLv23, nullptr, Compression::Disabled, 0,
                          SecurityTypes::Optional, false, resumeCertFile, resumeKeyFile);

      // TLS 1.2, so the session ticket arrives during the handshake and is
      // there for the second connection.  The client presents no certificate
      // and doesn't check the server's: only resumption is tested here.
      SSL_CTX* clientCtx = SSL_CTX_new(TLS_client_method());
      SSL_CTX_set_max_proto_version(clientCtx, TLS1_2_VERSION);

      SSL_SESSION* session = nullptr;
      check("first connection: handshake succeeds",
            runWithServer(server, [&]() { return opensslHandshake(clientCtx, serverPort, nullptr, &session, nullptr); }));
      check("first connection: client has a session to resume", session != nullptr);

      if (session)
      {
         bool reused = false;
         check("second connection: handshake succeeds",
               runWithServer(server, [&]() { return opensslHandshake(clientCtx, serverPort, session, nullptr, &reused); }));
         check("second connection: session was resumed", reused);
         SSL_SESSION_free(session);
      }
      SSL_CTX_free(clientCtx);
   }

   X509_free(cert);
   EVP_PKEY_free(key);
   remove(resumeCertFile);
   remove(resumeKeyFile);
}

int
main(int, char**)
{
#ifdef WIN32
   initNetwork();
#endif
   Log::initialize(Log::Cout, Log::None, "testTlsSecurity");

   testMatchHostName();

   EVP_PKEY* caKey = makeKey();
   X509* caCert = caKey ? makeCert(caKey, "testTlsSecurity CA", nullptr, nullptr, nullptr) : nullptr;
   check("CA certificate could be created", caCert != nullptr);
   if (caCert)
   {
      testUnparsableUriSubjectAltName(caCert, caKey);

      Security security;
      security.addRootCertPEM(toPem(caCert));
      testReload(security, caCert, caKey);
      testSessionResumption(security, caCert, caKey);
   }
   X509_free(caCert);
   EVP_PKEY_free(caKey);

   cerr << (failures == 0 ? "\nall checks passed\n" : "\nFAILURES\n");
   return failures == 0 ? 0 : -1;
}

#else // USE_SSL

int
main(int, char**)
{
   // Everything checked here is part of the TLS support.
   return 0;
}

#endif // USE_SSL

/* ====================================================================
 *
 * Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
