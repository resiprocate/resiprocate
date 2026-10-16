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

#include <openssl/err.h>
#include <openssl/rsa.h>  // EVP_PKEY_CTX_set_rsa_keygen_bits: a macro there in OpenSSL 1.1.1

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
   checkMatch("", "", false);                               // empty names never match
   checkMatch("example.com", "", false);

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
   checkMatch("", "", false);

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
                       SecurityTypes::DynamicTLS, nullptr, Compression::Disabled, 0,
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
                          SecurityTypes::DynamicTLS, nullptr, Compression::Disabled, 0,
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
                          SecurityTypes::DynamicTLS, nullptr, Compression::Disabled, 0,
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

static const char* const nonameCertFile = "testTlsSecurity_noname_cert.pem";
static const char* const nonameKeyFile = "testTlsSecurity_noname_key.pem";

// A certificate with neither a subjectAltName nor a commonName used to give
// one empty peer name, which matched a connection with no target domain
static void
testCertificateWithoutNames(Security& security, X509* caCert, EVP_PKEY* caKey)
{
   cerr << "-- a certificate without any names" << endl;
   EVP_PKEY* key = makeKey();
   X509* cert = key ? makeCert(key, nullptr, nullptr, caCert, caKey) : nullptr;
   const bool haveCerts = cert && writeCertFile(nonameCertFile, cert) && writeKeyFile(nonameKeyFile, key);
   check("test certificates could be created", haveCerts);

   if (haveCerts)
   {
      std::list<BaseSecurity::PeerName> names;
      BaseSecurity::getCertNames(cert, names);
      check("getCertNames finds no names, not an empty one", names.empty());

      const int serverPort = resipTestPort(5291);
      const int clientPort = resipTestPort(5292);
      Fifo<TransactionMessage> serverFifo;
      TlsTransport server(serverFifo, serverPort, V4, "127.0.0.1", security, "noname.test",
                          SecurityTypes::DynamicTLS, nullptr, Compression::Disabled, 0,
                          SecurityTypes::None, false, nonameCertFile, nonameKeyFile);
      Fifo<TransactionMessage> clientFifo;
      TlsTransport client(clientFifo, clientPort, V4, "127.0.0.1", security, Data::Empty,
                          SecurityTypes::DynamicTLS);

      // No target domain, so there's no name the server's certificate could match
      const Tuple dest("127.0.0.1", serverPort, V4, TLS);
      client.send(client.makeSendData(dest, makeRequest("127.0.0.1", clientPort, serverPort), "tid"));
      check("client with no target domain rejects a certificate without names",
            receive(client, clientFifo, server, serverFifo) == nullptr);
   }

   X509_free(cert);
   EVP_PKEY_free(key);
   remove(nonameCertFile);
   remove(nonameKeyFile);
}

static const char* const lateServerCertFile = "testTlsSecurity_late_server_cert.pem";
static const char* const lateServerKeyFile = "testTlsSecurity_late_server_key.pem";
static const char* const lateClientCertFile = "testTlsSecurity_late_client_cert.pem";
static const char* const lateClientKeyFile = "testTlsSecurity_late_client_key.pem";

// Transports with a domain used to copy the root certificates when they were
// created, so a root added later wasn't trusted by them
static void
testRootAddedAfterTransports(X509* caCert, EVP_PKEY* caKey)
{
   cerr << "-- a root certificate added after the transports were created" << endl;
   EVP_PKEY* serverKey = makeKey();
   EVP_PKEY* clientKey = makeKey();
   X509* serverCert = serverKey ? makeCert(serverKey, "late.test", "DNS:late.test", caCert, caKey) : nullptr;
   X509* clientCert = clientKey ? makeCert(clientKey, "client.test", "DNS:client.test", caCert, caKey) : nullptr;
   const bool haveCerts = serverCert && clientCert &&
                          writeCertFile(lateServerCertFile, serverCert) && writeKeyFile(lateServerKeyFile, serverKey) &&
                          writeCertFile(lateClientCertFile, clientCert) && writeKeyFile(lateClientKeyFile, clientKey);
   check("test certificates could be created", haveCerts);

   if (haveCerts)
   {
      // No root certificates yet
      Security security;

      // Both have a domain: the client verifies the server, and the server
      // (Optional) verifies the client's certificate
      const int serverPort = resipTestPort(5293);
      const int clientPort = resipTestPort(5294);
      Fifo<TransactionMessage> serverFifo;
      TlsTransport server(serverFifo, serverPort, V4, "127.0.0.1", security, "late.test",
                          SecurityTypes::DynamicTLS, nullptr, Compression::Disabled, 0,
                          SecurityTypes::Optional, false, lateServerCertFile, lateServerKeyFile);
      Fifo<TransactionMessage> clientFifo;
      TlsTransport client(clientFifo, clientPort, V4, "127.0.0.1", security, "client.test",
                          SecurityTypes::DynamicTLS, nullptr, Compression::Disabled, 0,
                          SecurityTypes::None, false, lateClientCertFile, lateClientKeyFile);

      security.addRootCertPEM(toPem(caCert));

      const Tuple dest("127.0.0.1", serverPort, V4, TLS, "late.test");
      client.send(client.makeSendData(dest, makeRequest("late.test", clientPort, serverPort), "tid"));
      std::unique_ptr<SipMessage> received = receive(client, clientFifo, server, serverFifo);
      check("client trusts the server through the root added later", received != nullptr);
      if (received)
      {
         // Peer names are only filled in for a certificate that verified
         const std::list<Data>& peerNames = received->getTlsPeerNames();
         check("server trusts the client through the root added later",
               peerNames.size() == 1 && peerNames.front() == "client.test");
      }
   }

   X509_free(clientCert);
   X509_free(serverCert);
   EVP_PKEY_free(clientKey);
   EVP_PKEY_free(serverKey);
   remove(lateServerCertFile);
   remove(lateServerKeyFile);
   remove(lateClientCertFile);
   remove(lateClientKeyFile);
}

static const char* const queueServerCertFile = "testTlsSecurity_queue_server_cert.pem";
static const char* const queueServerKeyFile = "testTlsSecurity_queue_server_key.pem";

// Leaves an error on this thread's OpenSSL error queue, as code that reads PEM certificates
// until there are no more does (eg. the STIR/SHAKEN certificate chains)
static void
leaveStaleOpenSSLError()
{
   BIO* empty = BIO_new_mem_buf("", 0);
   X509* none = PEM_read_bio_X509(empty, nullptr, nullptr, nullptr);
   X509_free(none);
   BIO_free(empty);
}

// Runs both transports, with an error left on the queue before each, until the server
// receives a request (or 5 seconds pass)
static std::unique_ptr<SipMessage>
receiveWithStaleErrors(TlsTransport& client, Fifo<TransactionMessage>& clientFifo,
                       TlsTransport& server, Fifo<TransactionMessage>& serverFifo, unsigned int forMs = 5000)
{
   const uint64_t deadline = Timer::getTimeMs() + forMs;
   while (Timer::getTimeMs() < deadline)
   {
      FdSet fdset;
      server.buildFdSet(fdset);
      client.buildFdSet(fdset);
      fdset.selectMilliSeconds(10);
      leaveStaleOpenSSLError();
      server.process(fdset);
      leaveStaleOpenSSLError();
      client.process(fdset);

      while (clientFifo.messageAvailable())
      {
         delete clientFifo.getNext();
      }
      while (serverFifo.messageAvailable())
      {
         std::unique_ptr<TransactionMessage> msg(serverFifo.getNext());
         if (dynamic_cast<SipMessage*>(msg.get()))
         {
            return std::unique_ptr<SipMessage>(static_cast<SipMessage*>(msg.release()));
         }
      }
   }
   return nullptr;
}

// An error something else on the transport's thread left on OpenSSL's error queue mustn't be
// taken as a failure of the connection's own SSL_read/SSL_write: SSL_get_error() checks the
// queue first, so a retryable SSL_ERROR_WANT_READ - eg. when the client reads the server's
// TLS 1.3 session tickets, which carry no application data - would look fatal and the
// connection would be closed
static void
testStaleErrorOnQueue(X509* caCert, EVP_PKEY* caKey)
{
   cerr << "-- an error another caller left on the OpenSSL error queue" << endl;
   EVP_PKEY* serverKey = makeKey();
   X509* serverCert = serverKey ? makeCert(serverKey, "queue.test", "DNS:queue.test", caCert, caKey) : nullptr;
   const bool haveCerts = serverCert &&
                          writeCertFile(queueServerCertFile, serverCert) && writeKeyFile(queueServerKeyFile, serverKey);
   check("test certificates could be created", haveCerts);

   if (haveCerts)
   {
      Security security;
      security.addRootCertPEM(toPem(caCert));

      const int serverPort = resipTestPort(5295);
      const int clientPort = resipTestPort(5296);
      Fifo<TransactionMessage> serverFifo;
      TlsTransport server(serverFifo, serverPort, V4, "127.0.0.1", security, "queue.test",
                          SecurityTypes::DynamicTLS, nullptr, Compression::Disabled, 0,
                          SecurityTypes::None, false, queueServerCertFile, queueServerKeyFile);
      Fifo<TransactionMessage> clientFifo;
      TlsTransport client(clientFifo, clientPort, V4, "127.0.0.1", security, Data::Empty,
                          SecurityTypes::DynamicTLS, nullptr, Compression::Disabled, 0,
                          SecurityTypes::None, false, Data::Empty, Data::Empty);

      const Tuple dest("127.0.0.1", serverPort, V4, TLS, "queue.test");
      client.send(client.makeSendData(dest, makeRequest("queue.test", clientPort, serverPort), "tid1"));
      std::unique_ptr<SipMessage> first = receiveWithStaleErrors(client, clientFifo, server, serverFifo);
      check("first request received", first != nullptr);

      // Give the client time to read what the server sent after the handshake
      receiveWithStaleErrors(client, clientFifo, server, serverFifo, 300);

      client.send(client.makeSendData(dest, makeRequest("queue.test", clientPort, serverPort), "tid2"));
      std::unique_ptr<SipMessage> second = receiveWithStaleErrors(client, clientFifo, server, serverFifo);
      check("second request received", second != nullptr);
      check("second request came over the same connection",
            first && second && first->getSource().getPort() == second->getSource().getPort());
   }

   ERR_clear_error();
   X509_free(serverCert);
   EVP_PKEY_free(serverKey);
   remove(queueServerCertFile);
   remove(queueServerKeyFile);
}

// An RSA key, as DHE cipher suites (TLS 1.2 has DHE_RSA, and no DHE_ECDSA) need
static EVP_PKEY*
makeRsaKey()
{
   EVP_PKEY* key = nullptr;
   EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
   if (ctx && EVP_PKEY_keygen_init(ctx) == 1 && EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) == 1)
   {
      EVP_PKEY_keygen(ctx, &key);
   }
   EVP_PKEY_CTX_free(ctx);
   return key;
}

// Whether a TLS 1.2 client offering only DHE cipher suites completes a handshake with a server
// using serverCtx (with cert and key): which it can only if the server has DH parameters
static bool
dheHandshake(SSL_CTX* serverCtx, X509* cert, EVP_PKEY* key)
{
   SSL_CTX* clientCtx = SSL_CTX_new(TLS_client_method());
   SSL_CTX_set_max_proto_version(clientCtx, TLS1_2_VERSION);
   SSL_CTX_set_cipher_list(clientCtx, "DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384");
   SSL* client = SSL_new(clientCtx);
   SSL* server = SSL_new(serverCtx);
   SSL_use_certificate(server, cert);
   SSL_use_PrivateKey(server, key);
   SSL_set_verify(server, SSL_VERIFY_NONE, nullptr);
   BIO* clientBio = nullptr;
   BIO* serverBio = nullptr;
   BIO_new_bio_pair(&clientBio, 0, &serverBio, 0);
   SSL_set_bio(client, clientBio, clientBio);
   SSL_set_bio(server, serverBio, serverBio);
   SSL_set_connect_state(client);
   SSL_set_accept_state(server);

   bool clientDone = false;
   bool serverDone = false;
   bool failed = false;
   for (int i = 0; i < 50 && !failed && !(clientDone && serverDone); ++i)
   {
      for (SSL* ssl : { client, server })
      {
         const int r = SSL_do_handshake(ssl);
         if (r == 1)
         {
            (ssl == client ? clientDone : serverDone) = true;
         }
         else
         {
            const int e = SSL_get_error(ssl, r);
            failed = failed || (e != SSL_ERROR_WANT_READ && e != SSL_ERROR_WANT_WRITE);
         }
      }
   }
   const char* cipher = SSL_get_cipher_name(client);
   const bool dhe = clientDone && serverDone && cipher && strncmp(cipher, "DHE-", 4) == 0;
   SSL_free(client);
   SSL_free(server);
   SSL_CTX_free(clientCtx);
   ERR_clear_error();
   return dhe;
}

// The Diffie-Hellman parameters Security is given (TlsDHParamsFilename) are installed in its
// SSL contexts, so DHE cipher suites can be negotiated
static void
testDHParams(X509* caCert, EVP_PKEY* caKey)
{
   cerr << "-- DH parameters" << endl;
   EVP_PKEY* key = makeRsaKey();
   X509* cert = key ? makeCert(key, "dhe.test", "DNS:dhe.test", caCert, caKey) : nullptr;
   check("RSA certificate could be created", cert != nullptr);
   if (cert)
   {
      {
         // (Its SSL context: the other, the TLS context, only does TLS 1.0)
         Security security(BaseSecurity::StrongestSuite, Data::Empty, TEST_DH_FILE);
         check("with DH parameters: a DHE cipher suite is negotiated", dheHandshake(security.getSslCtx(), cert, key));
      }
      {
         Security security(BaseSecurity::StrongestSuite, Data::Empty, "testTlsSecurity_no_such_file.pem");
         check("with a DH parameters file that can't be read: no DHE (and no crash)", !dheHandshake(security.getSslCtx(), cert, key));
         check("with a DH parameters file that can't be read: nothing left on the error queue", ERR_peek_error() == 0);
      }
      {
         Security security;
         check("without DH parameters: no DHE", !dheHandshake(security.getSslCtx(), cert, key));
      }
   }
   X509_free(cert);
   EVP_PKEY_free(key);
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
      testCertificateWithoutNames(security, caCert, caKey);

      testRootAddedAfterTransports(caCert, caKey);
      testStaleErrorOnQueue(caCert, caKey);
      testDHParams(caCert, caKey);
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
