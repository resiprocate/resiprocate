#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <cstdio>
#include <iostream>
#include <memory>

#ifdef USE_SSL

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/TransportFailure.hxx"
#include "resip/stack/ssl/Security.hxx"
#include "resip/stack/ssl/TlsTransport.hxx"
#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Log.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Timer.hxx"

#include "testPortOffset.hxx"

using namespace resip;
using namespace std;

// Checks that the SNI a TLS client sends is read by the server side of the
// connection and handed to the application on every SIP message received on
// it, through the ConnectionInfo the messages share.

static int failures = 0;

static void
check(const char* what, bool cond)
{
   cerr << (cond ? "ok   " : "FAIL ") << what << endl;
   if (!cond) ++failures;
}

static const char* const serverDomain = "sni.test";
static const char* const certFile = "testTlsSni_cert.pem";
static const char* const keyFile = "testTlsSni_key.pem";

static EVP_PKEY*
makeKey()
{
   EVP_PKEY* key = nullptr;
   EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
   if (ctx &&
       EVP_PKEY_keygen_init(ctx) == 1 &&
       EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_X9_62_prime256v1) == 1)
   {
      EVP_PKEY_keygen(ctx, &key);
   }
   EVP_PKEY_CTX_free(ctx);
   return key;
}

static bool
addExtension(X509* cert, X509V3_CTX* v3ctx, int nid, const char* value)
{
   X509_EXTENSION* ext = X509V3_EXT_conf_nid(nullptr, v3ctx, nid, value);
   if (!ext) return false;
   const bool added = X509_add_ext(cert, ext, -1) == 1;
   X509_EXTENSION_free(ext);
   return added;
}

// A CA certificate when issuer is null, otherwise a server certificate for
// name signed by issuer
static X509*
makeCert(EVP_PKEY* key, const char* name, X509* issuer, EVP_PKEY* issuerKey)
{
   static long serial = 1;
   X509* cert = X509_new();
   X509_set_version(cert, 2L);
   ASN1_INTEGER_set(X509_get_serialNumber(cert), serial++);
   X509_gmtime_adj(X509_getm_notBefore(cert), -60 * 60);
   X509_gmtime_adj(X509_getm_notAfter(cert), 60 * 60 * 24);
   X509_set_pubkey(cert, key);
   X509_NAME_add_entry_by_txt(X509_get_subject_name(cert), "CN", MBSTRING_ASC,
                              (const unsigned char*)name, -1, -1, 0);
   X509_set_issuer_name(cert, X509_get_subject_name(issuer ? issuer : cert));

   X509V3_CTX v3ctx;
   X509V3_set_ctx_nodb(&v3ctx);
   X509V3_set_ctx(&v3ctx, issuer ? issuer : cert, cert, nullptr, nullptr, 0);
   bool ok = addExtension(cert, &v3ctx, NID_subject_key_identifier, "hash");
   if (issuer)
   {
      const Data san = Data("DNS:") + name;
      ok = ok &&
           addExtension(cert, &v3ctx, NID_basic_constraints, "critical,CA:FALSE") &&
           addExtension(cert, &v3ctx, NID_key_usage, "critical,digitalSignature") &&
           addExtension(cert, &v3ctx, NID_ext_key_usage, "serverAuth,clientAuth") &&
           addExtension(cert, &v3ctx, NID_authority_key_identifier, "keyid:always") &&
           addExtension(cert, &v3ctx, NID_subject_alt_name, san.c_str());
   }
   else
   {
      ok = ok &&
           addExtension(cert, &v3ctx, NID_basic_constraints, "critical,CA:TRUE") &&
           addExtension(cert, &v3ctx, NID_key_usage, "critical,keyCertSign,cRLSign");
   }

   if (!ok || X509_sign(cert, issuerKey ? issuerKey : key, EVP_sha256()) <= 0)
   {
      X509_free(cert);
      return nullptr;
   }
   return cert;
}

static Data
toPem(X509* cert)
{
   BIO* bio = BIO_new(BIO_s_mem());
   PEM_write_bio_X509(bio, cert);
   char* pem = nullptr;
   const long len = BIO_get_mem_data(bio, &pem);
   Data result(pem, (Data::size_type)len);
   BIO_free(bio);
   return result;
}

static bool
writePemFiles(X509* cert, EVP_PKEY* key)
{
   bool ok = false;
   BIO* bio = BIO_new_file(certFile, "w");
   if (bio)
   {
      ok = PEM_write_bio_X509(bio, cert) == 1;
      BIO_free(bio);
   }
   bio = BIO_new_file(keyFile, "w");
   if (bio)
   {
      ok = ok && PEM_write_bio_PrivateKey(bio, key, nullptr, nullptr, 0, nullptr, nullptr) == 1;
      BIO_free(bio);
   }
   return ok;
}

static Data
makeRequest(int clientPort, int serverPort)
{
   NameAddr target;
   target.uri().scheme() = "sip";
   target.uri().user() = "server";
   target.uri().host() = serverDomain;
   target.uri().port() = serverPort;
   target.uri().param(p_transport) = "tls";

   NameAddr from;
   from.uri().scheme() = "sip";
   from.uri().user() = "client";
   from.uri().host() = "127.0.0.1";
   from.uri().port() = clientPort;

   std::unique_ptr<SipMessage> msg(Helper::makeRequest(target, from, OPTIONS));
   msg->header(h_Vias).front().transport() = Tuple::toData(TLS);
   msg->header(h_Vias).front().sentHost() = "127.0.0.1";
   msg->header(h_Vias).front().sentPort() = clientPort;

   Data encoded;
   {
      DataStream strm(encoded);
      msg->encode(strm);
   }
   return encoded;
}

// Runs both transports until the server has received a SIP message, the
// client has reported a transport failure, or a few seconds have passed
static std::unique_ptr<SipMessage>
receive(TlsTransport& client, Fifo<TransactionMessage>& clientFifo,
        TlsTransport& server, Fifo<TransactionMessage>& serverFifo)
{
   const uint64_t deadline = Timer::getTimeMs() + 5000;
   while (Timer::getTimeMs() < deadline)
   {
      FdSet fdset;
      server.buildFdSet(fdset);
      client.buildFdSet(fdset);
      fdset.selectMilliSeconds(10);
      server.process(fdset);
      client.process(fdset);

      while (clientFifo.messageAvailable())
      {
         std::unique_ptr<TransactionMessage> msg(clientFifo.getNext());
         if (TransportFailure* failure = dynamic_cast<TransportFailure*>(msg.get()))
         {
            cerr << "     client transport failure: " << *failure << endl;
            return nullptr;
         }
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
   cerr << "     timed out waiting for the request" << endl;
   return nullptr;
}

// SipMessage keeps the connection's details in one ConnectionInfo shared by
// every message from that connection, so a setter has to change only the
// message it is called on
static void
testSettersDoNotLeakIntoSharedInfo()
{
   auto shared = std::make_shared<ConnectionInfo>();
   shared->mTlsSni = "shared.test";
   shared->mTlsPeerNames.push_back("peer.test");

   SipMessage first;
   SipMessage second;
   first.setConnectionInfo(shared);
   second.setConnectionInfo(shared);

   first.setTlsPeerNames(std::list<Data>(1, "changed.test"));
   check("setter: changes the message it is called on",
         first.getTlsPeerNames().size() == 1 && first.getTlsPeerNames().front() == "changed.test");
   check("setter: keeps the rest of the connection's details", first.getTlsSni() == "shared.test");
   check("setter: other messages from the connection are unchanged",
         second.getTlsPeerNames().size() == 1 && second.getTlsPeerNames().front() == "peer.test" &&
         second.getConnectionInfo() == shared);

   SipMessage none;
   check("no ConnectionInfo: empty SNI", none.getTlsSni().empty());
   check("no ConnectionInfo: no peer names", none.getTlsPeerNames().empty());
   check("no ConnectionInfo: no WebSocket cookies", none.getWsCookies().empty());
   check("no ConnectionInfo: no WebSocket cookie context", !none.getWsCookieContext());
}

int
main(int, char**)
{
#ifdef WIN32
   initNetwork();
#endif
   //Log::initialize(Log::Cout, Log::Debug, "testTlsSni");
   Log::initialize(Log::Cout, Log::None, "testTlsSni");

   testSettersDoNotLeakIntoSharedInfo();

   EVP_PKEY* caKey = makeKey();
   EVP_PKEY* serverKey = makeKey();
   X509* caCert = caKey ? makeCert(caKey, "testTlsSni CA", nullptr, nullptr) : nullptr;
   X509* serverCert = (caCert && serverKey) ? makeCert(serverKey, serverDomain, caCert, caKey) : nullptr;
   const bool haveCerts = serverCert && writePemFiles(serverCert, serverKey);
   check("test certificates could be created", haveCerts);

   if (haveCerts)
   {
      Security security;
      security.addRootCertPEM(toPem(caCert));

      const int serverPort = resipTestPort(5161);
      const int clientPort = resipTestPort(5171);

      Fifo<TransactionMessage> serverFifo;
      TlsTransport server(serverFifo, serverPort, V4, "127.0.0.1", security, serverDomain,
                          SecurityTypes::SSLv23, nullptr, Compression::Disabled, 0,
                          SecurityTypes::None, false, certFile, keyFile);

      // No domain, so the client presents no certificate and verifies the server
      // against the root added above
      Fifo<TransactionMessage> clientFifo;
      TlsTransport client(clientFifo, clientPort, V4, "127.0.0.1", security, Data::Empty,
                          SecurityTypes::SSLv23);

      // The target domain is what the client sends as its SNI
      const Tuple dest("127.0.0.1", serverPort, V4, TLS, serverDomain);

      client.send(client.makeSendData(dest, makeRequest(clientPort, serverPort), "tid1"));
      std::unique_ptr<SipMessage> first = receive(client, clientFifo, server, serverFifo);
      check("server received the first request over TLS", first != nullptr);

      client.send(client.makeSendData(dest, makeRequest(clientPort, serverPort), "tid2"));
      std::unique_ptr<SipMessage> second = receive(client, clientFifo, server, serverFifo);
      check("server received the second request over TLS", second != nullptr);

      if (first && second)
      {
         check("received request has ConnectionInfo", first->getConnectionInfo() != nullptr);
         check("received request has the SNI the client sent", first->getTlsSni() == serverDomain);
         check("no client certificate, so no TLS peer names", first->getTlsPeerNames().empty());
         check("messages from one connection share its ConnectionInfo",
               first->getConnectionInfo() == second->getConnectionInfo());

         SipMessage copy(*first);
         check("a copy of a received message keeps its SNI", copy.getTlsSni() == serverDomain);
      }
   }

   X509_free(serverCert);
   X509_free(caCert);
   EVP_PKEY_free(serverKey);
   EVP_PKEY_free(caKey);
   remove(certFile);
   remove(keyFile);

   cerr << (failures == 0 ? "\nall checks passed\n" : "\nFAILURES\n");
   return failures == 0 ? 0 : -1;
}

#else // USE_SSL

int
main(int, char**)
{
   // SNI only exists on TLS connections, so there is nothing to check here.
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
