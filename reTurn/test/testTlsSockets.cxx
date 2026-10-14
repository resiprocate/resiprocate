#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <atomic>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#ifdef USE_SSL

#include <asio.hpp>
#include <asio/ssl.hpp>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <rutil/Log.hxx>

#include "../AsyncTlsSocketBase.hxx"
#include "../client/TurnTlsSocket.hxx"
#include "TlsTestSupport.hxx"

using namespace reTurn;
using namespace std;

// reTurn's TLS sockets: the client sends SNI and checks the server certificate is for the host
// name or IP address it connects to (TurnTlsSocket, which shares AsyncTlsSocketBase's setup),
// and the server closes a connection that doesn't complete its handshake in time
// (AsyncTlsSocketBase::doHandshake).

using namespace TlsTestSupport;

static const char* const caFile = "testTlsSockets_ca.pem";
static const char* const serverFile = "testTlsSockets_server.pem";

struct ServerResult
{
   bool handshakeOk = false;
   std::string sni;
};

// Accepts one TLS connection on 127.0.0.1 using serverFile and records whether the handshake
// succeeded and what SNI the client sent.
class OneShotTlsServer
{
public:
   OneShotTlsServer() :
      mContext(asio::ssl::context::tls_server),
      mAcceptor(mIo, asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), 0))
   {
      mContext.use_certificate_chain_file(serverFile);
      mContext.use_private_key_file(serverFile, asio::ssl::context::pem);
      mThread = std::thread([this] { run(); });
   }

   ~OneShotTlsServer()
   {
      mIo.stop();
      join();
   }

   unsigned short port() const { return mAcceptor.local_endpoint().port(); }

   // Waits for the connection to be done with
   ServerResult result()
   {
      join();
      return mResult;
   }

private:
   void run()
   {
      asio::ssl::stream<asio::ip::tcp::socket> stream(mIo, mContext);
      mAcceptor.async_accept(stream.lowest_layer(), [&](const asio::error_code& ec)
      {
         if (ec) return;
         stream.async_handshake(asio::ssl::stream_base::server, [&](const asio::error_code& hec)
         {
            mResult.handshakeOk = !hec;
            const char* sni = SSL_get_servername(stream.native_handle(), TLSEXT_NAMETYPE_host_name);
            mResult.sni = sni ? sni : "";
            asio::error_code ignored;
            stream.lowest_layer().close(ignored);
         });
      });
      mIo.run_for(std::chrono::seconds(10));
   }

   asio::io_context mIo;
   asio::ssl::context mContext;
   asio::ip::tcp::acceptor mAcceptor;
   ServerResult mResult;
   std::thread mThread;

   void join() { if (mThread.joinable()) mThread.join(); }
};

// Connects a TurnTlsSocket to the one-shot server with the given certificate
static bool
connectTo(const char* hostname, bool validate, ServerResult& serverResult)
{
   OneShotTlsServer server;
   asio::error_code ec;
   {
      TurnTlsSocket client(validate, asio::ip::make_address("127.0.0.1"), 0, caFile);
      ec = client.connect(hostname, server.port());
   }
   serverResult = server.result();
   return !ec;
}

static bool
installServerCert(X509* caCert, EVP_PKEY* caKey, const char* subjectAltName)
{
   EVP_PKEY* key = makeKey();
   X509* cert = key ? makeCert(key, "testTlsSockets server", subjectAltName, caCert, caKey) : nullptr;
   bool ok = cert && writePem(serverFile, cert, key);
   X509_free(cert);
   EVP_PKEY_free(key);
   return ok;
}

static void
testClient(X509* caCert, EVP_PKEY* caKey)
{
   ServerResult server;

   cerr << "-- certificate for DNS:localhost" << endl;
   check("certificate installed", installServerCert(caCert, caKey, "DNS:localhost"));
   check("connecting to localhost succeeds", connectTo("localhost", true, server));
   check("the client sent SNI localhost", server.sni == "localhost");
   check("connecting to 127.0.0.1 fails: the certificate has no IP address",
         !connectTo("127.0.0.1", true, server));
   check("no SNI is sent for an IP address", server.sni.empty());

   cerr << "-- certificate for DNS:other.test" << endl;
   check("certificate installed", installServerCert(caCert, caKey, "DNS:other.test"));
   check("connecting to localhost fails: the certificate is for another name",
         !connectTo("localhost", true, server));
   check("without hostname validation, connecting to localhost succeeds",
         connectTo("localhost", false, server));

   cerr << "-- certificate for IP:127.0.0.1" << endl;
   check("certificate installed", installServerCert(caCert, caKey, "IP:127.0.0.1"));
   check("connecting to 127.0.0.1 succeeds", connectTo("127.0.0.1", true, server));
}

// The server side of a connection, as reTurn's TlsConnection uses AsyncTlsSocketBase
class TestServerConnection : public AsyncTlsSocketBase
{
public:
   TestServerConnection(asio::io_context& io, asio::ssl::context& context) :
      AsyncTlsSocketBase(io, context, false) {}

   asio::ip::tcp::socket& tcp() { return mSocket.next_layer(); }
   bool isOpen() { return mSocket.lowest_layer().is_open(); }

   void onServerHandshakeSuccess() override { mResult = 1; }
   void onServerHandshakeFailure(const asio::error_code&) override { mResult = -1; }
   void onReceiveSuccess(const asio::ip::address&, unsigned short, const std::shared_ptr<DataBuffer>&) override {}
   void onReceiveFailure(const asio::error_code&) override {}
   void onSendSuccess() override {}
   void onSendFailure(const asio::error_code&) override {}

   std::atomic<int> mResult{0};
};

// Accepts a connection and starts the server handshake with a 1 second timeout; the client
// handshakes or stays silent.  Returns the server's result: 1 handshake done, -1 failed, 0 neither
// after 4 seconds.  stillOpenAfterwards: the connection was still open 1.5 seconds after a
// handshake that completed (the timer mustn't close it later).
static int
serverHandshake(bool clientHandshakes, bool& stillOpenAfterwards, double& seconds)
{
   asio::io_context io;
   asio::ssl::context context(asio::ssl::context::tls_server);
   context.use_certificate_chain_file(serverFile);
   context.use_private_key_file(serverFile, asio::ssl::context::pem);
   asio::ip::tcp::acceptor acceptor(io, asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), 0));

   std::shared_ptr<TestServerConnection> connection = std::make_shared<TestServerConnection>(io, context);
   const auto start = std::chrono::steady_clock::now();
   acceptor.async_accept(connection->tcp(), [&](const asio::error_code& ec)
   {
      if (!ec) connection->doHandshake(1);
   });

   asio::ssl::context clientContext(asio::ssl::context::tls_client);
   asio::ssl::stream<asio::ip::tcp::socket> client(io, clientContext);
   client.lowest_layer().connect(acceptor.local_endpoint());
   if (clientHandshakes)
   {
      client.async_handshake(asio::ssl::stream_base::client, [](const asio::error_code&) {});
   }

   while (connection->mResult == 0 && std::chrono::steady_clock::now() - start < std::chrono::seconds(4))
   {
      io.run_for(std::chrono::milliseconds(50));
   }
   seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
   if (connection->mResult == 1)
   {
      io.run_for(std::chrono::milliseconds(1500));
   }
   stillOpenAfterwards = connection->isOpen();
   return connection->mResult;
}

static void
testServerHandshakeTimeout(X509* caCert, EVP_PKEY* caKey)
{
   cerr << "-- server handshake timeout" << endl;
   check("certificate installed", installServerCert(caCert, caKey, "DNS:localhost"));

   bool open = false;
   double seconds = 0;
   int result = serverHandshake(false, open, seconds);
   check("a client that never handshakes: the connection is closed", result == -1 && !open);
   check("a client that never handshakes: after about the timeout", seconds > 0.8 && seconds < 3);

   result = serverHandshake(true, open, seconds);
   check("a client that handshakes: the handshake completes", result == 1);
   check("a client that handshakes: the timeout doesn't close the connection later", open);
}

// The sync client's connect timeout: here a server whose TCP connection is made (in the listening
// socket's backlog) but which never answers the handshake
static void
testClientConnectTimeout()
{
   cerr << "-- client connect timeout" << endl;
   asio::io_context io;
   asio::ip::tcp::acceptor neverAccepts(io, asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), 0));
   TurnTlsSocket client(true, asio::ip::make_address("127.0.0.1"), 0, caFile);
   client.setConnectTimeout(500);
   const auto start = std::chrono::steady_clock::now();
   const asio::error_code ec = client.connect("127.0.0.1", neverAccepts.local_endpoint().port());
   const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
   check("a server that never answers the handshake: connect fails with timed_out", ec == asio::error::timed_out);
   check("a server that never answers the handshake: after about the connect timeout", seconds > 0.4 && seconds < 3);
}

int
main(int, char**)
{
   resip::Log::initialize(resip::Log::Cout, resip::Log::None, "testTlsSockets");

   EVP_PKEY* caKey = makeKey();
   X509* caCert = caKey ? makeCert(caKey, "testTlsSockets CA", nullptr, nullptr, nullptr) : nullptr;
   check("CA certificate created", caCert && writePem(caFile, caCert, nullptr));
   if (caCert)
   {
      testClient(caCert, caKey);
      testServerHandshakeTimeout(caCert, caKey);
      testClientConnectTimeout();
   }
   X509_free(caCert);
   EVP_PKEY_free(caKey);
   remove(caFile);
   remove(serverFile);

   cerr << (failures() == 0 ? "\nall checks passed\n" : "\nFAILURES\n");
   return failures() == 0 ? 0 : -1;
}

#else // USE_SSL

int
main(int, char**)
{
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
