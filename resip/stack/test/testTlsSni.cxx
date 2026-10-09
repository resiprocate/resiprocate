#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <cstdio>
#include <iostream>
#include <memory>

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
   X509* caCert = caKey ? makeCert(caKey, "testTlsSni CA", nullptr, nullptr, nullptr) : nullptr;
   // The IP address lets a client that connects to 127.0.0.1 accept the certificate
   X509* serverCert = (caCert && serverKey) ?
      makeCert(serverKey, serverDomain, (Data("DNS:") + serverDomain + ", IP:127.0.0.1").c_str(), caCert, caKey) : nullptr;
   const bool haveCerts = serverCert && writeCertFile(certFile, serverCert) && writeKeyFile(keyFile, serverKey);
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

      client.send(client.makeSendData(dest, makeRequest(serverDomain, clientPort, serverPort), "tid1"));
      std::unique_ptr<SipMessage> first = receive(client, clientFifo, server, serverFifo);
      check("server received the first request over TLS", first != nullptr);

      client.send(client.makeSendData(dest, makeRequest(serverDomain, clientPort, serverPort), "tid2"));
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

      // RFC 6066 doesn't allow an IP address as SNI, so a client connecting to
      // one sends none.  A client of its own, so the connection is a new one.
      const int ipClientPort = resipTestPort(5172);
      Fifo<TransactionMessage> ipClientFifo;
      TlsTransport ipClient(ipClientFifo, ipClientPort, V4, "127.0.0.1", security, Data::Empty,
                            SecurityTypes::SSLv23);
      const Tuple ipDest("127.0.0.1", serverPort, V4, TLS, "127.0.0.1");
      ipClient.send(ipClient.makeSendData(ipDest, makeRequest("127.0.0.1", ipClientPort, serverPort), "tid3"));
      std::unique_ptr<SipMessage> byIp = receive(ipClient, ipClientFifo, server, serverFifo);
      check("server received a request sent to its IP address", byIp != nullptr);
      if (byIp)
      {
         check("no SNI for a client connecting to an IP address", byIp->getTlsSni().empty());
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
