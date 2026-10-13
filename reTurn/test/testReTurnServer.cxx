#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#include <openssl/ssl.h>
#endif

#include <rutil/Log.hxx>

#include "../ChannelManager.hxx"
#include "../ReTurnConfig.hxx"
#include "../RequestHandler.hxx"
#include "../TcpServer.hxx"
#include "../TurnManager.hxx"
#include "../UdpServer.hxx"
#include "../client/TurnAsyncSocketHandler.hxx"
#include "../client/TurnAsyncTcpSocket.hxx"
#include "../client/TurnAsyncUdpSocket.hxx"
#include "../client/TurnTcpSocket.hxx"
#include "../client/TurnUdpSocket.hxx"
#ifdef USE_SSL
#include "../TlsServer.hxx"
#include "../client/TurnAsyncTlsSocket.hxx"
#include "../client/TurnTlsSocket.hxx"
#include "TlsTestSupport.hxx"
#endif

using namespace reTurn;
using namespace std;

// reTurnServer end to end: its UDP, TCP and TLS servers, RequestHandler and TurnManager run in
// this process, configured from a config file as reTurnServer's main() does, and the sync
// (TurnUdpSocket, TurnTcpSocket, TurnTlsSocket) and async (TurnAsync*Socket) clients do STUN
// binds and TURN allocations with it over each transport, relaying data both ways between the
// client and UDP peers through each allocation.  Over TLS it also checks the server
// certificate's name is checked, that a client that never handshakes is disconnected
// (TlsHandshakeTimeout), and that OpenSSLCTXSetOptions reaches the server's TLS context.

#ifndef USE_SSL
// The checks TlsTestSupport.hxx provides with SSL
namespace TlsTestSupport
{
inline int& failures() { static int count = 0; return count; }
inline void check(const char* what, bool cond)
{
   std::cerr << (cond ? "ok   " : "FAIL ") << what << std::endl;
   if (!cond) ++failures();
}
}
#endif
using namespace TlsTestSupport;

static const char* const caFile = "testReTurnServer_ca.pem";
static const char* const certFile = "testReTurnServer_server.pem";
static const char* const keyFile = "testReTurnServer_server.key";
static const char* const usersFile = "testReTurnServer_users.txt";
static const char* const defaultConfig = "testReTurnServer_default.config";
static const char* const noTls12Config = "testReTurnServer_notls12.config";
static const unsigned short turnPort = 45478;      // UDP and TCP
static const unsigned short tlsPort = 45349;
static const unsigned short noTls12TurnPort = 45480;
static const unsigned short noTls12TlsPort = 45351;
static const char* const idleConfig = "testReTurnServer_idle.config";
static const unsigned short idleTurnPort = 45482;
static const unsigned short idleTlsPort = 45353;
static const char* const limitsConfig = "testReTurnServer_limits.config";
static const unsigned short limitsTurnPort = 45484;
static const unsigned short limitsTlsPort = 45355;
static const asio::ip::address localhost = asio::ip::make_address("127.0.0.1");

// A local TCP port that's free now, for a client asked to use a given port.  (Not a fixed one: a
// connection from it in an earlier run may still be in TIME_WAIT.)
static unsigned short
freeTcpPort()
{
   asio::io_context io;
   asio::ip::tcp::acceptor acceptor(io, asio::ip::tcp::endpoint(localhost, 0));
   return acceptor.local_endpoint().port();
}

// reTurnServer's UDP, TCP and TLS servers and what's behind them, configured from configFile
class InProcessServer
{
public:
   InProcessServer(const char* configFile)
   {
      char program[] = "testReTurnServer";
      char* argv[] = { program, nullptr };
      mConfig.parseConfig(1, argv, configFile);
      mTurnManager.reset(new TurnManager(mIo, mConfig));
      mRequestHandler.reset(new RequestHandler(*mTurnManager));
      mUdpServer = std::make_shared<UdpServer>(mIo, *mRequestHandler, mConfig.mTurnAddress, mConfig.mTurnPort);
      mTcpServer = std::make_shared<TcpServer>(mIo, *mRequestHandler, mConfig.mTurnAddress, mConfig.mTurnPort);
      mUdpServer->start();
      mTcpServer->start();
#ifdef USE_SSL
      mTlsServer = std::make_shared<TlsServer>(mIo, *mRequestHandler, mConfig.mTurnAddress, mConfig.mTlsTurnPort);
      mTlsServer->start();
#endif
      mThread = std::thread([this]() { mIo.run(); });
   }

   ~InProcessServer()
   {
      mIo.stop();
      mThread.join();
   }

private:
   ReTurnConfig mConfig;
   asio::io_context mIo;
   std::unique_ptr<TurnManager> mTurnManager;
   std::unique_ptr<RequestHandler> mRequestHandler;
   std::shared_ptr<UdpServer> mUdpServer;
   std::shared_ptr<TcpServer> mTcpServer;
#ifdef USE_SSL
   std::shared_ptr<TlsServer> mTlsServer;
#endif
   std::thread mThread;
};

static bool
writeConfig(const char* file, unsigned short port, unsigned short tlsPort, const char* extra)
{
   std::ofstream config(file);
   config << "TurnAddress = 127.0.0.1\n"
          << "TurnPort = " << port << "\n"
          << "TlsTurnPort = " << tlsPort << "\n"
          << "AuthenticationRealm = reTurn\n"
          << "UserDatabaseFile = " << usersFile << "\n"
          << "TlsServerCertificateFilename = " << certFile << "\n"
          << "TlsServerPrivateKeyFilename = " << keyFile << "\n"
          << "TlsTempDhFilename = " << RETURN_TEST_DH_FILE << "\n"
          << "TlsPrivateKeyPassword =\n"
          << "TlsHandshakeTimeout = 1\n"
          << extra;
   return (bool)config;
}

// A peer on the far side of a relay: a UDP socket on 127.0.0.1
class Peer
{
public:
   Peer() : mSocket(mIo, asio::ip::udp::endpoint(localhost, 0))
   {
      mSocket.non_blocking(true);
   }

   unsigned short port() const { return mSocket.local_endpoint().port(); }

   // What arrives within timeout, and where it came from; empty if nothing does
   std::string receive(asio::ip::udp::endpoint& from, std::chrono::milliseconds timeout = std::chrono::seconds(2))
   {
      const auto deadline = std::chrono::steady_clock::now() + timeout;
      char buffer[1500];
      for (;;)
      {
         asio::error_code ec;
         const size_t size = mSocket.receive_from(asio::buffer(buffer), from, 0, ec);
         if (!ec)
         {
            return std::string(buffer, size);
         }
         if (ec != asio::error::would_block || std::chrono::steady_clock::now() >= deadline)
         {
            return std::string();
         }
         std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
   }

   void sendTo(const std::string& data, const asio::ip::udp::endpoint& to)
   {
      mSocket.send_to(asio::buffer(data), to);
   }

private:
   asio::io_context mIo;
   asio::ip::udp::socket mSocket;
};

// The payloads aren't a multiple of 4 bytes long, so ChannelData isn't on a 4 byte boundary
static const std::string toPeerA = "to peer A";
static const std::string fromPeerA = "from peer A";
static const std::string toPeerB = "to peer B";
static const std::string fromPeerB = "from peer B";

// Whether the sync client receives data from the peer on peerPort
static bool
syncReceives(TurnSocket& client, const std::string& expected, unsigned short peerPort)
{
   char buffer[1500];
   unsigned int size = sizeof(buffer);
   asio::ip::address address;
   unsigned short port = 0;
   return !client.receive(buffer, size, 2000, &address, &port) &&
          std::string(buffer, size) == expected && address == localhost && port == peerPort;
}

// Data relayed both ways through the sync client's allocation: over a channel to peer A
// (setActiveDestination binds it, which also gives 127.0.0.1 permission), and in Send and Data
// indications to peer B, which has no channel
static void
testSyncRelay(const std::string& what, TurnSocket& client)
{
   Peer peerA;
   Peer peerB;
   const asio::ip::udp::endpoint relay(client.getRelayTuple().getAddress(), client.getRelayTuple().getPort());
   asio::ip::udp::endpoint from;

   check((what + "a channel is bound to peer A").c_str(), !client.setActiveDestination(localhost, peerA.port()));
   check((what + "sends to peer A, over the channel").c_str(), !client.send(toPeerA.data(), (unsigned int)toPeerA.size()));
   check((what + "peer A gets the data, from the relay address").c_str(), peerA.receive(from) == toPeerA && from == relay);
   peerA.sendTo(fromPeerA, relay);
   check((what + "peer A's answer is relayed to the client, over the channel").c_str(), syncReceives(client, fromPeerA, peerA.port()));

   check((what + "sends to peer B, in a Send indication").c_str(),
         !client.sendTo(localhost, peerB.port(), toPeerB.data(), (unsigned int)toPeerB.size()));
   check((what + "peer B gets the data, from the relay address").c_str(), peerB.receive(from) == toPeerB && from == relay);
   peerB.sendTo(fromPeerB, relay);
   check((what + "peer B's answer is relayed to the client, in a Data indication").c_str(), syncReceives(client, fromPeerB, peerB.port()));
}

// The clients start their channel numbers at a random one, which must be in the range the server
// accepts
static void
testChannelNumbers()
{
   bool inRange = true;
   for (int i = 0; i < 1000 && inRange; ++i)
   {
      ChannelManager channelManager;
      const RemotePeer* remotePeer = channelManager.createChannelBinding(StunTuple(StunTuple::UDP, localhost, 1));
      inRange = remotePeer->getChannel() >= MIN_CHANNEL_NUM && remotePeer->getChannel() <= MAX_CHANNEL_NUM;
   }
   check("channel numbers: the first is always in range", inRange);
}

// A STUN bind and a TURN allocation (of a UDP relay, as reflow asks for), with data relayed
// through it, with the sync client
static void
testSyncClient(const std::string& name, TurnSocket& client, const char* host, unsigned short port)
{
   std::string what = "sync " + name + ": ";
   client.setUsernameAndPassword("test", "1234");
   check((what + "connects").c_str(), !client.connect(host, port));
   check((what + "STUN bind").c_str(), !client.bindRequest());
   check((what + "the reflexive address is the client's").c_str(), client.getReflexiveTuple().getAddress() == localhost);
   check((what + "TURN allocation, with long-term credentials").c_str(),
         !client.createAllocation(TurnSocket::UnspecifiedLifetime, TurnSocket::UnspecifiedBandwidth,
                                  StunMessage::PropsNone, TurnSocket::UnspecifiedToken, StunTuple::UDP));
   check((what + "a relay address is allocated").c_str(),
         client.getRelayTuple().getAddress() == localhost && client.getRelayTuple().getPort() != 0);
   testSyncRelay(what, client);
   check((what + "the allocation is refreshed").c_str(), !client.refreshAllocation());
   check((what + "the allocation is released").c_str(), !client.destroyAllocation());
}

// Collects what the async client reports, and moves it on: bind, allocate, relay data to and
// from peer A then peer B, refresh, release.  The client and the peers run on the test's thread:
// the callbacks are called from io.run_for(), and poll() gives the peers their turn.
class AsyncHandler : public TurnAsyncSocketHandler
{
public:
   std::shared_ptr<TurnAsyncSocket> mSocket;
   // 1 success, -1 failure
   std::atomic<int> mConnected{0};
   std::atomic<int> mBound{0};
   std::atomic<int> mAllocated{0};
   std::atomic<int> mToPeerA{0};
   std::atomic<int> mFromPeerA{0};
   std::atomic<int> mToPeerB{0};
   std::atomic<int> mFromPeerB{0};
   std::atomic<int> mRefreshed{0};
   std::atomic<int> mReleased{0};
   StunTuple mReflexive;
   StunTuple mRelay;
   Peer mPeerA;
   Peer mPeerB;

   // All done, or something failed
   bool finished() const
   {
      return mConnected < 0 || mBound < 0 || mAllocated < 0 || mToPeerA < 0 || mFromPeerA < 0 ||
             mToPeerB < 0 || mFromPeerB < 0 || mRefreshed < 0 || mReleased != 0;
   }

   // Each peer answers what the client sends it through the relay
   void poll()
   {
      pollPeer(mPeerA, mToPeerA, toPeerA, fromPeerA);
      pollPeer(mPeerB, mToPeerB, toPeerB, fromPeerB);
   }

   void onConnectSuccess(unsigned int, const asio::ip::address&, unsigned short) override
   {
      mConnected = 1;
      mSocket->bindRequest();
   }
   void onConnectFailure(unsigned int, const asio::error_code&) override { mConnected = -1; }
   void onSharedSecretSuccess(unsigned int, const char*, unsigned int, const char*, unsigned int) override {}
   void onSharedSecretFailure(unsigned int, const asio::error_code&) override {}
   void onBindSuccess(unsigned int, const StunTuple& reflexiveTuple, const StunTuple&) override
   {
      mReflexive = reflexiveTuple;
      mBound = 1;
      mSocket->createAllocation(TurnAsyncSocket::UnspecifiedLifetime, TurnAsyncSocket::UnspecifiedBandwidth,
                                StunMessage::PropsNone, TurnAsyncSocket::UnspecifiedToken, StunTuple::UDP);
   }
   void onBindFailure(unsigned int, const asio::error_code&, const StunTuple&) override { mBound = -1; }
   void onAllocationSuccess(unsigned int, const StunTuple&, const StunTuple& relayTuple, unsigned int, unsigned int, uint64_t) override
   {
      mRelay = relayTuple;
      mAllocated = 1;
      // Binds a channel to peer A, which also gives 127.0.0.1 permission
      mSocket->setActiveDestination(localhost, mPeerA.port());
   }
   void onAllocationFailure(unsigned int, const asio::error_code&) override { mAllocated = -1; }
   // A refresh reports the lifetime granted; releasing the allocation is a refresh to 0
   void onRefreshSuccess(unsigned int, unsigned int lifetimeSecs) override
   {
      if (lifetimeSecs > 0)
      {
         mRefreshed = 1;
         mSocket->destroyAllocation();
      }
      else
      {
         mReleased = 1;
      }
   }
   void onRefreshFailure(unsigned int, const asio::error_code&) override
   {
      if (mRefreshed == 1) mReleased = -1; else mRefreshed = -1;
   }
   // Over UDP this goes in a Send indication, as the channel bind isn't answered yet; over TCP
   // and TLS the channel is used straight away
   void onSetActiveDestinationSuccess(unsigned int) override { mSocket->send(toPeerA.data(), toPeerA.size()); }
   void onSetActiveDestinationFailure(unsigned int, const asio::error_code&) override { mToPeerA = -1; }
   void onClearActiveDestinationSuccess(unsigned int) override {}
   void onClearActiveDestinationFailure(unsigned int, const asio::error_code&) override {}
   void onChannelBindRequestSent(unsigned int, unsigned short) override {}
   void onChannelBindSuccess(unsigned int, unsigned short) override {}
   // Peer A's channel is bound first, then peer B's
   void onChannelBindFailure(unsigned int, const asio::error_code&) override { (mToPeerA == 0 ? mToPeerA : mToPeerB) = -1; }
   // A peer's answer, relayed by the server: from peer A, send to peer B (sendTo binds it a
   // channel too); from peer B, refresh the allocation
   void onReceiveSuccess(unsigned int, const asio::ip::address& address, unsigned short port, const std::shared_ptr<DataBuffer>& data) override
   {
      const std::string received(data->data(), data->size());
      if (address == localhost && port == mPeerA.port() && mFromPeerA == 0)
      {
         mFromPeerA = received == fromPeerA ? 1 : -1;
         mSocket->sendTo(localhost, mPeerB.port(), toPeerB.data(), toPeerB.size());
      }
      else if (address == localhost && port == mPeerB.port() && mFromPeerB == 0)
      {
         mFromPeerB = received == fromPeerB ? 1 : -1;
         mSocket->refreshAllocation(600);
      }
   }
   void onReceiveFailure(unsigned int, const asio::error_code&) override {}
   void onSendSuccess(unsigned int) override {}
   void onSendFailure(unsigned int, const asio::error_code&) override {}
   void onIncomingBindRequestProcessed(unsigned int, const StunTuple&) override {}

private:
   // When the client's data reaches peer from the relay address, the peer answers
   void pollPeer(Peer& peer, std::atomic<int>& reached, const std::string& expected, const std::string& answer)
   {
      asio::ip::udp::endpoint from;
      const std::string received = reached == 0 ? peer.receive(from, std::chrono::milliseconds(0)) : std::string();
      if (!received.empty())
      {
         const asio::ip::udp::endpoint relay(mRelay.getAddress(), mRelay.getPort());
         reached = received == expected && from == relay ? 1 : -1;
         peer.sendTo(answer, relay);
      }
   }
};

typedef std::function<std::shared_ptr<TurnAsyncSocket>(asio::io_context&, AsyncHandler&)> AsyncSocketFactory;

// Runs an async client, made by makeSocket(io, handler), from connecting until handler has
// finished.  The socket is used through TurnAsyncSocket, as reflow's Flow does (the TCP and TLS
// ones also inherit connect() and close() from their AsyncSocketBase).
static void
runAsyncClient(AsyncHandler& handler, const char* host, unsigned short port, const char* password,
               const AsyncSocketFactory& makeSocket)
{
   asio::io_context io;
   std::shared_ptr<TurnAsyncSocket> socket = makeSocket(io, handler);
   handler.mSocket = socket;
   socket->setUsernameAndPassword("test", password);
   socket->connect(host, port);

   const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
   while (!handler.finished() && std::chrono::steady_clock::now() < deadline)
   {
      io.run_for(std::chrono::milliseconds(10));
      handler.poll();
   }

   socket->disableTurnAsyncHandler();
   socket->close();
   handler.mSocket.reset();
   socket.reset();
   io.run_for(std::chrono::milliseconds(100));
}

// A STUN bind and a TURN allocation (of a UDP relay, as reflow asks for), with data relayed
// through it, refreshed and released, with the async client
static void
testAsyncClient(const std::string& name, const char* host, unsigned short port, const AsyncSocketFactory& makeSocket)
{
   std::string what = "async " + name + ": ";
   AsyncHandler handler;
   runAsyncClient(handler, host, port, "1234", makeSocket);
   check((what + "connects").c_str(), handler.mConnected == 1);
   check((what + "STUN bind").c_str(), handler.mBound == 1 && handler.mReflexive.getAddress() == localhost);
   check((what + "TURN allocation, with long-term credentials").c_str(),
         handler.mAllocated == 1 && handler.mRelay.getAddress() == localhost && handler.mRelay.getPort() != 0);
   check((what + "peer A gets data from the client, from the relay address").c_str(), handler.mToPeerA == 1);
   check((what + "peer A's answer is relayed to the client").c_str(), handler.mFromPeerA == 1);
   check((what + "peer B gets data from the client, from the relay address").c_str(), handler.mToPeerB == 1);
   check((what + "peer B's answer is relayed to the client").c_str(), handler.mFromPeerB == 1);
   check((what + "the allocation is refreshed").c_str(), handler.mRefreshed == 1);
   check((what + "the allocation is released").c_str(), handler.mReleased == 1);
}

// With the wrong password: the STUN bind needs no credentials, the allocation is refused
static void
testAsyncWrongPassword(const std::string& name, const char* host, unsigned short port, const AsyncSocketFactory& makeSocket)
{
   std::string what = "async " + name + ", wrong password: ";
   AsyncHandler handler;
   runAsyncClient(handler, host, port, "wrong", makeSocket);
   check((what + "connects and binds").c_str(), handler.mConnected == 1 && handler.mBound == 1);
   check((what + "the allocation is refused").c_str(), handler.mAllocated == -1);
}

static void
testUdpAndTcp()
{
   cerr << "-- UDP" << endl;
   {
      TurnUdpSocket client(localhost, 0);
      testSyncClient("UDP", client, "127.0.0.1", turnPort);
   }
   AsyncSocketFactory asyncUdp = [](asio::io_context& io, AsyncHandler& handler)
   {
      return std::shared_ptr<TurnAsyncSocket>(std::make_shared<TurnAsyncUdpSocket>(io, &handler, localhost, 0));
   };
   testAsyncClient("UDP", "127.0.0.1", turnPort, asyncUdp);
   testAsyncWrongPassword("UDP", "127.0.0.1", turnPort, asyncUdp);

   cerr << "-- TCP" << endl;
   {
      TurnTcpSocket client(localhost, 0);
      testSyncClient("TCP", client, "127.0.0.1", turnPort);
   }
   AsyncSocketFactory asyncTcp = [](asio::io_context& io, AsyncHandler& handler)
   {
      return std::shared_ptr<TurnAsyncSocket>(std::make_shared<TurnAsyncTcpSocket>(io, &handler, localhost, 0));
   };
   testAsyncClient("TCP", "127.0.0.1", turnPort, asyncTcp);
   testAsyncWrongPassword("TCP", "127.0.0.1", turnPort, asyncTcp);
   {
      TurnTcpSocket client(localhost, 0);
      client.setUsernameAndPassword("test", "wrong");
      check("sync TCP, wrong password: connects", !client.connect("127.0.0.1", turnPort));
      check("sync TCP, wrong password: the allocation is refused",
            (bool)client.createAllocation(TurnSocket::UnspecifiedLifetime, TurnSocket::UnspecifiedBandwidth,
                                          StunMessage::PropsNone, TurnSocket::UnspecifiedToken, StunTuple::UDP));
   }
   {
      // The reflexive address is where the server sees the client's connection come from
      const unsigned short clientTcpPort = freeTcpPort();
      TurnTcpSocket client(localhost, clientTcpPort);
      check("sync TCP: connects from the local port asked for",
            !client.connect("127.0.0.1", turnPort) && !client.bindRequest() &&
            client.getReflexiveTuple().getPort() == clientTcpPort);
   }
   {
      const unsigned short clientTcpPort = freeTcpPort();
      AsyncHandler handler;
      runAsyncClient(handler, "127.0.0.1", turnPort, "1234", [clientTcpPort](asio::io_context& io, AsyncHandler& handler)
      {
         return std::shared_ptr<TurnAsyncSocket>(std::make_shared<TurnAsyncTcpSocket>(io, &handler, localhost, clientTcpPort));
      });
      check("async TCP: connects from the local port asked for",
            handler.mBound == 1 && handler.mReflexive.getPort() == clientTcpPort);
   }
}

#ifdef USE_SSL

static void
testTls()
{
   cerr << "-- TLS" << endl;
   {
      // Checking the server certificate is for the name connected to
      TurnTlsSocket client(true, localhost, 0, caFile);
      testSyncClient("TLS", client, "localhost", tlsPort);
   }
   {
      TurnTlsSocket client(true, localhost, 0, caFile);
      check("sync TLS: connecting to 127.0.0.1 fails - the certificate is only for localhost",
            (bool)client.connect("127.0.0.1", tlsPort));
      // As connect() does for the next address a name resolves to, after a failed handshake
      check("sync TLS: after a failed handshake, connecting again works",
            !client.connect("localhost", tlsPort) && !client.bindRequest());
   }
   {
      const unsigned short clientTlsPort = freeTcpPort();
      TurnTlsSocket client(true, localhost, clientTlsPort, caFile);
      check("sync TLS: connects from the local port asked for",
            !client.connect("localhost", tlsPort) && !client.bindRequest() &&
            client.getReflexiveTuple().getPort() == clientTlsPort);
   }

   asio::ssl::context sslContext(asio::ssl::context::tls_client);
   sslContext.set_verify_mode(asio::ssl::context::verify_peer | asio::ssl::context::verify_fail_if_no_peer_cert);
   sslContext.load_verify_file(caFile);
   AsyncSocketFactory asyncTls = [&sslContext](asio::io_context& io, AsyncHandler& handler)
   {
      // Checking the server certificate is for the name connected to
      return std::shared_ptr<TurnAsyncSocket>(std::make_shared<TurnAsyncTlsSocket>(io, sslContext, true, &handler, localhost, 0));
   };
   testAsyncClient("TLS", "localhost", tlsPort, asyncTls);
   testAsyncWrongPassword("TLS", "localhost", tlsPort, asyncTls);
   {
      AsyncHandler handler;
      runAsyncClient(handler, "127.0.0.1", tlsPort, "1234", asyncTls);
      check("async TLS: connecting to 127.0.0.1 fails - the certificate is only for localhost", handler.mConnected == -1);
   }
   {
      const unsigned short clientTlsPort = freeTcpPort();
      AsyncHandler handler;
      runAsyncClient(handler, "localhost", tlsPort, "1234", [&sslContext, clientTlsPort](asio::io_context& io, AsyncHandler& handler)
      {
         return std::shared_ptr<TurnAsyncSocket>(std::make_shared<TurnAsyncTlsSocket>(io, sslContext, true, &handler, localhost, clientTlsPort));
      });
      check("async TLS: connects from the local port asked for",
            handler.mBound == 1 && handler.mReflexive.getPort() == clientTlsPort);
   }

   cerr << "-- TLS: a client that never handshakes" << endl;
   {
      asio::io_context io;
      asio::ip::tcp::socket socket(io);
      socket.connect(asio::ip::tcp::endpoint(localhost, tlsPort));
      const auto start = std::chrono::steady_clock::now();
      char buffer[16];
      asio::error_code ec;
      socket.read_some(asio::buffer(buffer), ec);  // returns when the server closes the connection
      const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
      check("TLS timeout: the server closes the connection", (bool)ec);
      check("TLS timeout: after TlsHandshakeTimeout (1 second)", seconds > 0.8 && seconds < 4);
   }
}

// Whether a client limited to maxVersion completes a TLS handshake with the server on port
static bool
handshakes(unsigned short port, int maxVersion)
{
   asio::io_context io;
   asio::ssl::context context(asio::ssl::context::tls_client);
   SSL_CTX_set_max_proto_version(context.native_handle(), maxVersion);
   asio::ssl::stream<asio::ip::tcp::socket> stream(io, context);
   asio::error_code ec;
   stream.lowest_layer().connect(asio::ip::tcp::endpoint(localhost, port), ec);
   if (!ec)
   {
      stream.handshake(asio::ssl::stream_base::client, ec);
   }
   return !ec;
}

static bool
writeCertificates()
{
   EVP_PKEY* caKey = makeKey();
   X509* caCert = caKey ? makeCert(caKey, "testReTurnServer CA", nullptr, nullptr, nullptr) : nullptr;
   EVP_PKEY* serverKey = makeKey();
   X509* serverCert = serverKey && caCert ? makeCert(serverKey, "localhost", "DNS:localhost", caCert, caKey) : nullptr;
   bool ok = serverCert && writePem(caFile, caCert, nullptr) &&
             writePem(certFile, serverCert, nullptr) && writePem(keyFile, nullptr, serverKey);
   X509_free(serverCert);
   EVP_PKEY_free(serverKey);
   X509_free(caCert);
   EVP_PKEY_free(caKey);
   return ok;
}

#endif // USE_SSL

// Seconds until the server closes socket (anything it sends is read and ignored), or -1 if it's
// still open after maxSeconds
static double
secondsUntilClosed(asio::ip::tcp::socket& socket, double maxSeconds)
{
   socket.non_blocking(true);
   const auto start = std::chrono::steady_clock::now();
   for (;;)
   {
      char buffer[1024];
      asio::error_code ec;
      socket.read_some(asio::buffer(buffer), ec);
      const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
      if (ec && ec != asio::error::would_block)
      {
         return seconds;
      }
      if (seconds > maxSeconds)
      {
         return -1;
      }
      if (ec)
      {
         std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
   }
}

#ifdef USE_SSL
// A TLS connection to the server on port, once its handshake is done (the server's certificate
// isn't checked): ec is the outcome
struct TlsClientConnection
{
   TlsClientConnection(asio::io_context& io, unsigned short port) :
      context(asio::ssl::context::tls_client),
      stream(io, context)
   {
      stream.lowest_layer().connect(asio::ip::tcp::endpoint(localhost, port), ec);
      if (!ec)
      {
         stream.handshake(asio::ssl::stream_base::client, ec);
      }
   }
   asio::ssl::context context;
   asio::ssl::stream<asio::ip::tcp::socket> stream;
   asio::error_code ec;
};
#endif

// TcpConnectionIdleTimeout (1 second here): a TCP or TLS connection that receives nothing while it
// carries no allocation is closed
static void
testIdleTimeout()
{
   cerr << "-- TcpConnectionIdleTimeout" << endl;
   asio::io_context io;
   {
      asio::ip::tcp::socket socket(io);
      socket.connect(asio::ip::tcp::endpoint(localhost, idleTurnPort));
      const double seconds = secondsUntilClosed(socket, 4);
      check("idle: a TCP connection that sends nothing is closed after the timeout", seconds > 0.8 && seconds < 3);
   }
#ifdef USE_SSL
   {
      TlsClientConnection tls(io, idleTlsPort);
      check("idle: a TLS connection completes its handshake", !tls.ec);
      const double seconds = secondsUntilClosed(tls.stream.next_layer(), 4);
      check("idle: a TLS connection that sends nothing after its handshake is closed after the timeout", seconds > 0.8 && seconds < 3);
   }
#endif
   {
      TurnTcpSocket client(localhost, 0);
      client.setUsernameAndPassword("test", "1234");
      check("idle: STUN bind", !client.connect("127.0.0.1", idleTurnPort) && !client.bindRequest());
      std::this_thread::sleep_for(std::chrono::seconds(2));
      check("idle: a connection gone quiet since a STUN bind is closed", (bool)client.bindRequest());
   }
   {
      TurnTcpSocket client(localhost, 0);
      client.setUsernameAndPassword("test", "1234");
      check("idle: TURN allocation",
            !client.connect("127.0.0.1", idleTurnPort) &&
            !client.createAllocation(TurnSocket::UnspecifiedLifetime, TurnSocket::UnspecifiedBandwidth,
                                     StunMessage::PropsNone, TurnSocket::UnspecifiedToken, StunTuple::UDP));
      std::this_thread::sleep_for(std::chrono::seconds(2));
      check("idle: a quiet connection carrying an allocation is kept open", !client.refreshAllocation());
      check("idle: the allocation is released", !client.destroyAllocation());
      std::this_thread::sleep_for(std::chrono::seconds(2));
      check("idle: once its allocation is gone, the quiet connection is closed", (bool)client.bindRequest());
   }
}

// MaxTcpConnectionsPerIp (4 here), counting TCP and TLS connections together, and
// MaxAllocationsPerUser (2 here), counting allocations over all transports
static void
testLimits()
{
   cerr << "-- MaxTcpConnectionsPerIp" << endl;
   {
      asio::io_context io;
      asio::ip::tcp::socket tcp1(io);
      asio::ip::tcp::socket tcp2(io);
      tcp1.connect(asio::ip::tcp::endpoint(localhost, limitsTurnPort));
      tcp2.connect(asio::ip::tcp::endpoint(localhost, limitsTurnPort));
#ifdef USE_SSL
      TlsClientConnection tls1(io, limitsTlsPort);
      TlsClientConnection tls2(io, limitsTlsPort);
      check("connections: TLS connections are accepted alongside TCP ones, up to the limit", !tls1.ec && !tls2.ec);
#else
      asio::ip::tcp::socket tcp3(io);
      asio::ip::tcp::socket tcp4(io);
      tcp3.connect(asio::ip::tcp::endpoint(localhost, limitsTurnPort));
      tcp4.connect(asio::ip::tcp::endpoint(localhost, limitsTurnPort));
#endif
      check("connections: up to the limit, they stay open", secondsUntilClosed(tcp1, 0.3) < 0);

      asio::ip::tcp::socket overTcp(io);
      overTcp.connect(asio::ip::tcp::endpoint(localhost, limitsTurnPort));
      const double tcpSeconds = secondsUntilClosed(overTcp, 2);
      check("connections: one more TCP connection is closed at once", tcpSeconds >= 0 && tcpSeconds < 0.5);
#ifdef USE_SSL
      asio::ip::tcp::socket overTls(io);
      overTls.connect(asio::ip::tcp::endpoint(localhost, limitsTlsPort));
      const double tlsSeconds = secondsUntilClosed(overTls, 2);
      check("connections: so is one more TLS connection - the limit counts both", tlsSeconds >= 0 && tlsSeconds < 0.5);
#endif

      tcp2.close();
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
      asio::ip::tcp::socket again(io);
      again.connect(asio::ip::tcp::endpoint(localhost, limitsTurnPort));
      check("connections: once one is closed, a new one is accepted", secondsUntilClosed(again, 0.3) < 0);
   }

   cerr << "-- MaxAllocationsPerUser" << endl;
   {
      auto allocate = [](TurnSocket& client)
      {
         client.setUsernameAndPassword("test", "1234");
         asio::error_code ec = client.connect("127.0.0.1", limitsTurnPort);
         if (!ec)
         {
            ec = client.createAllocation(TurnSocket::UnspecifiedLifetime, TurnSocket::UnspecifiedBandwidth,
                                         StunMessage::PropsNone, TurnSocket::UnspecifiedToken, StunTuple::UDP);
         }
         return ec;
      };
      TurnUdpSocket first(localhost, 0);
      TurnUdpSocket second(localhost, 0);
      TurnUdpSocket third(localhost, 0);
      TurnTcpSocket overTcp(localhost, 0);
      check("allocations: a user's allocations are accepted up to the limit", !allocate(first) && !allocate(second));
      check("allocations: one more is refused with 486 (Allocation Quota Reached)", allocate(third).value() == 486);
      check("allocations: over TCP too - the limit counts all transports", allocate(overTcp).value() == 486);
      check("allocations: once one is released, another is accepted",
            !first.destroyAllocation() &&
            !third.createAllocation(TurnSocket::UnspecifiedLifetime, TurnSocket::UnspecifiedBandwidth,
                                    StunMessage::PropsNone, TurnSocket::UnspecifiedToken, StunTuple::UDP));
      // Released, so none are left when the server is stopped
      second.destroyAllocation();
      third.destroyAllocation();
   }
}

// Runs test with a server configured from configFile
static void
runWithServer(const char* configFile, const std::function<void()>& test)
{
   InProcessServer server(configFile);
   try
   {
      test();
   }
   catch (const std::exception& e)
   {
      // Caught while the server is still running (see InProcessServer)
      check((std::string("the tests ran without an exception: ") + e.what()).c_str(), false);
   }
}

int
main(int argc, char** argv)
{
   // Run with a log level (eg. testReTurnServer INFO) to see reTurn's logging
   resip::Log::initialize(resip::Log::Cout, argc > 1 ? resip::Log::toLevel(resip::Data(argv[1])) : resip::Log::None, "testReTurnServer");

   bool ready = true;
#ifdef USE_SSL
   ready = writeCertificates();
#endif
   {
      std::ofstream users(usersFile);
      users << "test:1234:reTurn:authorized\n";
      ready = ready && (bool)users;
   }
   ready = ready &&
           writeConfig(defaultConfig, turnPort, tlsPort, "") &&
           writeConfig(noTls12Config, noTls12TurnPort, noTls12TlsPort,
                       "OpenSSLCTXSetOptions = SSL_OP_NO_SSLv2, SSL_OP_NO_SSLv3, SSL_OP_NO_COMPRESSION, SSL_OP_NO_TLSv1_2\n") &&
           writeConfig(idleConfig, idleTurnPort, idleTlsPort,
                       "TcpConnectionIdleTimeout = 1\nAllocationPortRangeMin = 60000\nAllocationPortRangeMax = 60999\n") &&
           writeConfig(limitsConfig, limitsTurnPort, limitsTlsPort,
                       "TcpConnectionIdleTimeout = 0\nMaxTcpConnectionsPerIp = 4\nMaxAllocationsPerUser = 2\n"
                       "AllocationPortRangeMin = 61000\nAllocationPortRangeMax = 61999\n");
   check("certificates, users and config files written", ready);

   if (ready)
   {
      try
      {
         InProcessServer server(defaultConfig);
         try
         {
            testChannelNumbers();
            testUdpAndTcp();
#ifdef USE_SSL
            testTls();

            cerr << "-- OpenSSLCTXSetOptions" << endl;
            check("options: by default, a TLS 1.2 client is accepted", handshakes(tlsPort, TLS1_2_VERSION));
            InProcessServer noTls12(noTls12Config);
            check("options: with SSL_OP_NO_TLSv1_2, a TLS 1.2 client is refused", !handshakes(noTls12TlsPort, TLS1_2_VERSION));
            check("options: with SSL_OP_NO_TLSv1_2, a TLS 1.3 client is accepted", handshakes(noTls12TlsPort, TLS1_3_VERSION));
#endif
            runWithServer(idleConfig, testIdleTimeout);
            runWithServer(limitsConfig, testLimits);
         }
         catch (const std::exception& e)
         {
            check((std::string("the tests ran without an exception: ") + e.what()).c_str(), false);
         }
      }
      catch (const std::exception& e)
      {
         check((std::string("the server could be run: ") + e.what()).c_str(), false);
      }
   }

   for (const char* file : { caFile, certFile, keyFile, usersFile, defaultConfig, noTls12Config, idleConfig, limitsConfig })
   {
      remove(file);
   }

   cerr << (failures() == 0 ? "\nall checks passed\n" : "\nFAILURES\n");
   return failures() == 0 ? 0 : -1;
}

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
 */
