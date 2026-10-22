#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <asio.hpp>

#include <rutil/Data.hxx>
#include <rutil/Log.hxx>

#include "reflow/Flow.hxx"
#include "reflow/FlowManager.hxx"
#include "reflow/MediaStream.hxx"
#ifdef USE_SSL
#include "reflow/dtls_wrapper/DtlsFactory.hxx"
#include "reflow/dtls_wrapper/DtlsSocket.hxx"
#endif

using namespace flowmanager;
using namespace reTurn;

// FlowManager end to end: two FlowManagers, A and B, each with a MediaStream on 127.0.0.1,
// exchange RTP through a relay standing in for the network - plain, with SRTP keyed from SDP
// (SDES), and with DTLS-SRTP, the handshake running over UDP through the Flows.  The relay sees
// what's on the wire, and can alter packets and send its own: media protected by SRTP must be
// encrypted on the wire, and packets that aren't authentic SRTP mustn't reach the application.
//
// Run with a log level (eg. testFlowManager INFO) to see reflow's logging.

static int failures = 0;

static void
check(const std::string& what, bool cond)
{
   std::cerr << (cond ? "ok   " : "FAIL ") << what << std::endl;
   if (!cond) ++failures;
}

static const asio::ip::address localhost = asio::ip::make_address("127.0.0.1");
typedef std::vector<unsigned char> Datagram;

// Counts the warnings reTurn's STUN message parser logs (eg. for a packet that isn't STUN), and
// passes everything on to the normal log if one was asked for
class StunWarningCounter : public resip::ExternalLogger
{
public:
   explicit StunWarningCounter(bool log) : mLog(log) {}
   bool operator()(resip::Log::Level level, const resip::Subsystem&, const resip::Data&, const char* file, int,
                   const resip::Data&, const resip::Data&, const resip::Data&) override
   {
      if (level <= resip::Log::Warning && file && strstr(file, "StunMessage"))
      {
         ++mWarnings;
      }
      return mLog;
   }
   std::atomic<int> mWarnings{0};

private:
   bool mLog;
};
static StunWarningCounter* stunWarnings = nullptr;

// Waits up to timeoutMs for cond to be true
static bool
waitFor(const std::function<bool()>& cond, int timeoutMs = 5000)
{
   const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
   while (!cond())
   {
      if (std::chrono::steady_clock::now() >= deadline)
      {
         return false;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
   }
   return true;
}

// RTP, by its first byte (RFC 7983)
static bool
isRtp(const Datagram& d)
{
   return !d.empty() && d[0] >= 128 && d[0] <= 191;
}

// An RTP packet carrying payload: version 2, payload type 0
static Datagram
rtpPacket(const std::string& payload, uint16_t seq, uint32_t ssrc)
{
   Datagram packet = { 0x80, 0x00, (unsigned char)(seq >> 8), (unsigned char)seq, 0, 0, 0, 0,
                       (unsigned char)(ssrc >> 24), (unsigned char)(ssrc >> 16), (unsigned char)(ssrc >> 8), (unsigned char)ssrc };
   packet.insert(packet.end(), payload.begin(), payload.end());
   return packet;
}

// The network between A and B: A sends to portFacingA() and B to portFacingB(), and each one's
// packets reach the other from the relay's address facing it
class Relay
{
public:
   Relay(unsigned short portA, unsigned short portB) :
      mToA(localhost, portA),
      mToB(localhost, portB),
      mFacingA(mIo, asio::ip::udp::endpoint(localhost, 0)),
      mFacingB(mIo, asio::ip::udp::endpoint(localhost, 0))
   {
      mFacingA.non_blocking(true);
      mFacingB.non_blocking(true);
      mThread = std::thread([this]() { run(); });
   }

   ~Relay()
   {
      mStop = true;
      mThread.join();
   }

   unsigned short portFacingA() const { return mFacingA.local_endpoint().port(); }
   unsigned short portFacingB() const { return mFacingB.local_endpoint().port(); }

   // Whether text was seen in the clear on the wire, either way
   bool sawInClear(const std::string& text)
   {
      std::lock_guard<std::mutex> lock(mPassedMutex);
      for (const Datagram& d : mPassed)
      {
         if (std::search(d.begin(), d.end(), text.begin(), text.end()) != d.end())
         {
            return true;
         }
      }
      return false;
   }

   // Whether DTLS records were seen on the wire
   bool sawDtls()
   {
      std::lock_guard<std::mutex> lock(mPassedMutex);
      return std::any_of(mPassed.begin(), mPassed.end(), [](const Datagram& d) { return !d.empty() && d[0] >= 20 && d[0] <= 63; });
   }

   // The size of the largest DTLS datagram seen on the wire
   size_t largestDtls()
   {
      std::lock_guard<std::mutex> lock(mPassedMutex);
      size_t largest = 0;
      for (const Datagram& d : mPassed)
      {
         if (!d.empty() && d[0] >= 20 && d[0] <= 63)
         {
            largest = (std::max)(largest, d.size());
         }
      }
      return largest;
   }

   // Alters a byte of the payload of the next RTP packet from A to B
   void tamperWithNextRtpToB() { mTampered = false; mTamper = true; }
   std::atomic<bool> mTampered{false};

   // Sends a datagram to B from the relay's address facing it, where A's packets come from
   void sendToB(const Datagram& d)
   {
      std::lock_guard<std::mutex> lock(mSocketMutex);
      asio::error_code ec;
      mFacingB.send_to(asio::buffer(d), mToB, 0, ec);
   }

private:
   void run()
   {
      while (!mStop)
      {
         const bool passedFromA = pass(mFacingA, mFacingB, mToB, true);
         const bool passedFromB = pass(mFacingB, mFacingA, mToA, false);
         if (!passedFromA && !passedFromB)
         {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
         }
      }
   }

   // Passes on a datagram waiting on in, if there is one
   bool pass(asio::ip::udp::socket& in, asio::ip::udp::socket& out, const asio::ip::udp::endpoint& to, bool fromA)
   {
      std::lock_guard<std::mutex> lock(mSocketMutex);
      // As large as a UDP datagram: DTLS flights can be larger than the MTU, being sent in one
      // datagram (with a 4096 bit RSA certificate, the server's first flight is over 2KB)
      static unsigned char buffer[65536];
      asio::ip::udp::endpoint from;
      asio::error_code ec;
      const size_t size = in.receive_from(asio::buffer(buffer), from, 0, ec);
      if (ec)
      {
         return ec != asio::error::would_block;  // eg. an ICMP error, reported by Windows: read again
      }
      Datagram d(buffer, buffer + size);
      if (fromA && mTamper && isRtp(d) && d.size() > 14)
      {
         d[14] ^= 0xff;  // in the payload, after the 12 byte header
         mTamper = false;
         mTampered = true;
      }
      out.send_to(asio::buffer(d), to, 0, ec);

      std::lock_guard<std::mutex> passedLock(mPassedMutex);
      mPassed.push_back(std::move(d));
      return true;
   }

   const asio::ip::udp::endpoint mToA;
   const asio::ip::udp::endpoint mToB;
   asio::io_context mIo;
   asio::ip::udp::socket mFacingA;
   asio::ip::udp::socket mFacingB;
   std::mutex mSocketMutex;
   std::mutex mPassedMutex;
   std::vector<Datagram> mPassed;
   std::atomic<bool> mTamper{false};
   std::atomic<bool> mStop{false};
   std::thread mThread;
};

// One side of the call: a MediaStream (RTP only) on 127.0.0.1:port
class Endpoint : public MediaStreamHandler
{
public:
   Endpoint(FlowManager& flowManager, unsigned short port, uint32_t ssrc) :
      mSsrc(ssrc)
   {
      mStream = flowManager.createMediaStream(*this, StunTuple(StunTuple::UDP, localhost, port), false /* rtcpEnabled */);
   }

   ~Endpoint()
   {
      delete mStream;
   }

   void onMediaStreamReady(const StunTuple&, const StunTuple&) override { mReady = true; }
   void onMediaStreamError(unsigned int) override { mError = true; }

   MediaStream& stream() { return *mStream; }
   Flow& flow() { return *mStream->getRtpFlow(); }

   void sendRtp(const std::string& payload)
   {
      Datagram packet = rtpPacket(payload, ++mSeq, mSsrc);
      packet.resize(packet.size() + SRTP_MAX_TRAILER_LEN);  // Flow::send appends SRTP's tag in place
      flow().send((char*)packet.data(), (unsigned int)(packet.size() - SRTP_MAX_TRAILER_LEN));
   }

   // Whether an RTP packet reaches the application within timeoutMs: payload is what it carried
   bool receiveRtp(std::string& payload, unsigned int timeoutMs)
   {
      char buffer[2048];
      unsigned int size = sizeof(buffer);
      if (flow().receive(buffer, size, timeoutMs) || size < 12)
      {
         return false;
      }
      payload.assign(buffer + 12, size - 12);
      return true;
   }

   // Throws away whatever has arrived
   void drain()
   {
      std::string payload;
      while (receiveRtp(payload, 200)) {}
   }

   // Whether nothing reaches the application within timeoutMs
   bool nothingArrives(unsigned int timeoutMs = 300)
   {
      std::string payload;
      return !receiveRtp(payload, timeoutMs);
   }

   std::atomic<bool> mReady{false};
   std::atomic<bool> mError{false};

private:
   MediaStream* mStream;
   uint32_t mSsrc;
   uint16_t mSeq = 0;
};

// Whether RTP carrying payload, sent by from, reaches to's application: sent again every 100ms,
// up to tries times, as it can only be sent once SRTP is set up (eg. after a DTLS handshake)
static bool
delivered(Endpoint& from, Endpoint& to, const std::string& payload, int tries = 50)
{
   for (int i = 0; i < tries; ++i)
   {
      from.sendRtp(payload);
      std::string received;
      while (to.receiveRtp(received, 100))
      {
         if (received == payload)
         {
            return true;
         }
      }
   }
   return false;
}

// A and B, each sending to the relay between them
class Call
{
public:
   Call(FlowManager& managerA, FlowManager& managerB, unsigned short portA, unsigned short portB) :
      a(managerA, portA, 0xAAAA),
      b(managerB, portB, 0xBBBB),
      relay(portA, portB)
   {
   }

   // Whether both media streams are ready, then both flows once they're sending to the relay
   bool connect()
   {
      if (!waitFor([this]() { return a.mReady && b.mReady; }))
      {
         return false;
      }
      a.flow().setActiveDestination("127.0.0.1", relay.portFacingA());
      b.flow().setActiveDestination("127.0.0.1", relay.portFacingB());
      return waitFor([this]() { return a.flow().isReady() && b.flow().isReady(); });
   }

   Endpoint a;
   Endpoint b;
   Relay relay;
};

// Checks that only authentic SRTP from A reaches B's application: not a packet altered on the
// way, nor plain RTP from A's address or from anywhere else - and that media still flows after
static void
checkOnlySrtpIsDelivered(const std::string& what, Call& call)
{
   call.b.drain();
   call.relay.tamperWithNextRtpToB();
   call.a.sendRtp(what + " tampered");
   check(what + "a packet altered on the way isn't delivered (SRTP authentication)",
         waitFor([&call]() { return (bool)call.relay.mTampered; }, 1000) && call.b.nothingArrives());

   call.relay.sendToB(rtpPacket(what + " injected", 1000, 0xAAAA));
   check(what + "plain RTP from the peer's address isn't delivered", call.b.nothingArrives());

   asio::io_context io;
   asio::ip::udp::socket other(io, asio::ip::udp::endpoint(localhost, 0));
   other.send_to(asio::buffer(rtpPacket(what + " from elsewhere", 1, 0xCCCC)),
                 asio::ip::udp::endpoint(localhost, call.b.flow().getLocalTuple().getPort()));
   check(what + "plain RTP from another address isn't delivered", call.b.nothingArrives());

   check(what + "media still flows afterwards", delivered(call.a, call.b, what + " afterwards"));
}

// A media stream whose local port another socket has can't work: it reports an error, rather than
// becoming ready with a socket that sends from a port the OS picks
static void
testPortInUse(FlowManager& manager)
{
   std::cerr << "-- a local port another socket has" << std::endl;
   asio::io_context io;
   asio::ip::udp::socket holder(io, asio::ip::udp::endpoint(localhost, 0));
   Endpoint endpoint(manager, holder.local_endpoint().port(), 0x1111);
   check("port in use: the media stream reports an error", waitFor([&endpoint]() { return (bool)endpoint.mError; }, 1000));
   check("port in use: and doesn't become ready", !endpoint.mReady);
}

static void
testPlain(FlowManager& managerA, FlowManager& managerB)
{
   std::cerr << "-- plain RTP" << std::endl;
   Call call(managerA, managerB, 46100, 46102);
   check("plain: both flows are ready", call.connect());
   check("plain: RTP from A reaches B", delivered(call.a, call.b, "plain A to B"));
   check("plain: RTP from B reaches A", delivered(call.b, call.a, "plain B to A"));
   check("plain: the media is in the clear on the wire", call.relay.sawInClear("plain A to B"));
}

static void
testSdes(FlowManager& managerA, FlowManager& managerB)
{
   std::cerr << "-- SRTP keyed from SDP (SDES)" << std::endl;
   const std::string keyAToB = "0123456789abcdefghijklmnopqrst";  // SRTP_MASTER_KEY_LEN: 16 byte key, 14 byte salt
   const std::string keyBToA = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123";
   const resip::MediaConstants::SrtpCryptoSuite suite = resip::MediaConstants::SRTP_AES_CM_128_HMAC_SHA1_80;
   {
      Call call(managerA, managerB, 46110, 46112);
      check("sdes: the SRTP sessions are created",
            call.a.stream().createOutboundSRTPSession(suite, keyAToB.data(), keyAToB.size()) &&
            call.a.stream().createInboundSRTPSession(suite, keyBToA.data(), keyBToA.size()) &&
            call.b.stream().createOutboundSRTPSession(suite, keyBToA.data(), keyBToA.size()) &&
            call.b.stream().createInboundSRTPSession(suite, keyAToB.data(), keyAToB.size()));
      check("sdes: both flows are ready", call.connect());
      check("sdes: SRTP from A reaches B", delivered(call.a, call.b, "sdes A to B"));
      check("sdes: SRTP from B reaches A", delivered(call.b, call.a, "sdes B to A"));
      check("sdes: the media is encrypted on the wire",
            !call.relay.sawInClear("sdes A to B") && !call.relay.sawInClear("sdes B to A"));
      checkOnlySrtpIsDelivered("sdes: ", call);
   }
   {
      Call call(managerA, managerB, 46120, 46122);
      call.a.stream().createOutboundSRTPSession(suite, keyAToB.data(), keyAToB.size());
      call.b.stream().createInboundSRTPSession(suite, keyBToA.data(), keyBToA.size());  // not A's key
      check("sdes, wrong key: both flows are ready", call.connect());
      check("sdes, wrong key: nothing from A is delivered", !delivered(call.a, call.b, "sdes wrong key", 5));
   }
}

#ifdef USE_SSL

static void
testDtls(FlowManager& managerA, FlowManager& managerB)
{
   std::cerr << "-- DTLS-SRTP" << std::endl;
   char fingerprintA[dtls::DtlsSocket::FingerprintSize];
   char fingerprintB[dtls::DtlsSocket::FingerprintSize];
   managerA.getDtlsFactory()->getMyCertFingerprint(fingerprintA);
   managerB.getDtlsFactory()->getMyCertFingerprint(fingerprintB);

   {
      // A takes the active role (a=setup:active), so is the DTLS client
      Call call(managerA, managerB, 46130, 46132);
      check("dtls: both flows are ready", call.connect());
      call.a.flow().setRemoteSDPFingerprint("sha-256", fingerprintB);
      call.b.flow().setRemoteSDPFingerprint("sha-256", fingerprintA);
      call.b.sendRtp("dtls before the handshake");
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      check("dtls: media isn't sent in the clear before the handshake", !call.relay.sawInClear("dtls before the handshake"));
      call.a.flow().startDtlsClient("127.0.0.1", call.relay.portFacingA());
      check("dtls: SRTP from A reaches B, once the handshake is done", delivered(call.a, call.b, "dtls A to B"));
      check("dtls: SRTP from B reaches A", delivered(call.b, call.a, "dtls B to A"));
      check("dtls: the handshake went over the wire", call.relay.sawDtls());
      check("dtls: DTLS packets aren't parsed as STUN (no STUN parser warnings)", stunWarnings->mWarnings == 0);
      // With FlowManager's 4096 bit RSA certificate, the server's first flight is over 2KB
      check("dtls: no DTLS datagram on the wire is larger than the MTU (1200 bytes)",
            call.relay.largestDtls() > 0 && call.relay.largestDtls() <= dtls::DtlsFactory::DefaultMtu);
      check("dtls: the media is encrypted on the wire",
            !call.relay.sawInClear("dtls A to B") && !call.relay.sawInClear("dtls B to A"));
      checkOnlySrtpIsDelivered("dtls: ", call);
   }
   {
      // The client's SDP says the server has another certificate (here the client's own)
      Call call(managerA, managerB, 46140, 46142);
      check("dtls, wrong fingerprint at the client: both flows are ready", call.connect());
      call.a.flow().setRemoteSDPFingerprint("sha-256", fingerprintA);
      call.b.flow().setRemoteSDPFingerprint("sha-256", fingerprintA);
      call.a.flow().startDtlsClient("127.0.0.1", call.relay.portFacingA());
      check("dtls, wrong fingerprint at the client: nothing from A is delivered", !delivered(call.a, call.b, "dtls client mismatch A to B", 20));
      check("dtls, wrong fingerprint at the client: nothing from B is delivered", !delivered(call.b, call.a, "dtls client mismatch B to A", 5));
      check("dtls, wrong fingerprint at the client: the handshake went over the wire", call.relay.sawDtls());
   }
   {
      // The server's SDP says the client has another certificate
      Call call(managerA, managerB, 46150, 46152);
      check("dtls, wrong fingerprint at the server: both flows are ready", call.connect());
      call.a.flow().setRemoteSDPFingerprint("sha-256", fingerprintB);
      call.b.flow().setRemoteSDPFingerprint("sha-256", fingerprintB);
      call.a.flow().startDtlsClient("127.0.0.1", call.relay.portFacingA());
      check("dtls, wrong fingerprint at the server: nothing from A is delivered", !delivered(call.a, call.b, "dtls server mismatch A to B", 20));
      check("dtls, wrong fingerprint at the server: nothing from B is delivered", !delivered(call.b, call.a, "dtls server mismatch B to A", 5));
   }
}

// Keeps the timers it's given, never firing them
class NoTimers : public dtls::DtlsTimerContext
{
public:
   void addTimer(std::shared_ptr<dtls::DtlsTimer> timer, unsigned int) override { mTimers.push_back(std::move(timer)); }
private:
   std::vector<std::shared_ptr<dtls::DtlsTimer>> mTimers;
};

// A peer, on its own UDP port, that completes the DTLS cookie exchange with a flow - so the flow
// starts a server handshake with it - then goes quiet, holding one of the flow's places for DTLS
class StalledDtlsPeer
{
public:
   StalledDtlsPeer(dtls::DtlsFactory& factory, unsigned short flowPort) :
      mSocket(mIo, asio::ip::udp::endpoint(localhost, 0)),
      mFlow(localhost, flowPort)
   {
      mSocket.non_blocking(true);
      mDtls.reset(factory.createClient(std::unique_ptr<dtls::DtlsSocketContext>(new Context(mSocket, mFlow))));
   }

   // Whether the flow started a handshake: the ClientHello is answered with a HelloVerifyRequest,
   // and the ClientHello with its cookie with the flow's first flight - which isn't answered
   bool stall()
   {
      mDtls->startClient();
      Datagram verifyRequest = receive();
      if (verifyRequest.empty())
      {
         return false;
      }
      mDtls->handlePacketMaybe(verifyRequest.data(), verifyRequest.size());
      return !receive().empty();
   }

private:
   // Sends what the DtlsSocket writes from the peer's UDP socket, to the flow
   class Context : public dtls::DtlsSocketContext
   {
   public:
      Context(asio::ip::udp::socket& socket, const asio::ip::udp::endpoint& to) : mSocket(socket), mTo(to) {}
      void write(const unsigned char* data, unsigned int len) override
      {
         asio::error_code ec;
         mSocket.send_to(asio::buffer(data, len), mTo, 0, ec);
      }
      void handshakeCompleted() override {}
      void handshakeFailed(const char*) override {}
   private:
      asio::ip::udp::socket& mSocket;
      const asio::ip::udp::endpoint mTo;
   };

   // What arrives within 2 seconds
   Datagram receive()
   {
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
      unsigned char buffer[2048];
      asio::ip::udp::endpoint from;
      for (;;)
      {
         asio::error_code ec;
         const size_t size = mSocket.receive_from(asio::buffer(buffer), from, 0, ec);
         if (!ec)
         {
            return Datagram(buffer, buffer + size);
         }
         if (std::chrono::steady_clock::now() >= deadline)
         {
            return Datagram();
         }
         std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
   }

   asio::io_context mIo;
   asio::ip::udp::socket mSocket;
   asio::ip::udp::endpoint mFlow;
   std::unique_ptr<dtls::DtlsSocket> mDtls;
};

// A flow allows only so many DTLS associations: peers that start handshakes and go quiet must not
// be able to keep the real peer out
static void
testDtlsStalledHandshakes(FlowManager& managerA, FlowManager& managerB)
{
   std::cerr << "-- DTLS-SRTP, with stalled handshakes" << std::endl;
   char fingerprintA[dtls::DtlsSocket::FingerprintSize];
   char fingerprintB[dtls::DtlsSocket::FingerprintSize];
   managerA.getDtlsFactory()->getMyCertFingerprint(fingerprintA);
   managerB.getDtlsFactory()->getMyCertFingerprint(fingerprintB);

   X509* cert = nullptr;
   EVP_PKEY* key = nullptr;
   FlowManager::createCert(resip::Data("stalled@example.com"), 1, 2048, cert, key);
   const int defaultGrace = Flow::dtlsHandshakeEvictionGrace;
   Flow::dtlsHandshakeEvictionGrace = 2;
   {
      dtls::DtlsFactory stalledFactory(std::unique_ptr<dtls::DtlsTimerContext>(new NoTimers), cert, key);
      Call call(managerA, managerB, 46160, 46162);
      check("stalled: both flows are ready", call.connect());
      call.a.flow().setRemoteSDPFingerprint("sha-256", fingerprintB);
      call.b.flow().setRemoteSDPFingerprint("sha-256", fingerprintA);

      // As many as B allows
      const unsigned short bPort = call.b.flow().getLocalTuple().getPort();
      std::vector<std::unique_ptr<StalledDtlsPeer>> stalled;
      int started = 0;
      for (int i = 0; i < 8; ++i)
      {
         stalled.emplace_back(new StalledDtlsPeer(stalledFactory, bPort));
         started += stalled.back()->stall() ? 1 : 0;
      }
      check("stalled: 8 peers start handshakes with B, then go quiet", started == 8);

      stalled.emplace_back(new StalledDtlsPeer(stalledFactory, bPort));
      check("stalled: while those are recent (within the grace period), another peer isn't let in", !stalled.back()->stall());

      // Beyond the grace period, a stalled handshake makes way for A
      std::this_thread::sleep_for(std::chrono::milliseconds(2200));
      call.a.flow().startDtlsClient("127.0.0.1", call.relay.portFacingA());
      check("stalled: A's handshake takes a stalled one's place, and SRTP from A reaches B",
            delivered(call.a, call.b, "stalled A to B"));
      check("stalled: SRTP from B reaches A", delivered(call.b, call.a, "stalled B to A"));
   }
   Flow::dtlsHandshakeEvictionGrace = defaultGrace;
   X509_free(cert);
   EVP_PKEY_free(key);
}

#endif // USE_SSL

int
main(int argc, char** argv)
{
   // Warnings are always logged, to count the STUN parser's; they're only shown with a log level
   static StunWarningCounter counter(argc > 1);
   stunWarnings = &counter;
   resip::Log::initialize(resip::Log::Cout, argc > 1 ? resip::Log::toLevel(resip::Data(argv[1])) : resip::Log::Warning,
                          "testFlowManager", counter);

   try
   {
      FlowManager managerA;
      FlowManager managerB;
#ifdef USE_SSL
      managerA.initializeDtlsFactory("a@example.com");
      managerB.initializeDtlsFactory("b@example.com");
      check("the DTLS factories are created", managerA.getDtlsFactory() && managerB.getDtlsFactory());
#endif

      testPortInUse(managerA);
      testPlain(managerA, managerB);
      testSdes(managerA, managerB);
#ifdef USE_SSL
      if (managerA.getDtlsFactory() && managerB.getDtlsFactory())
      {
         testDtls(managerA, managerB);
         testDtlsStalledHandshakes(managerA, managerB);
      }
#endif
   }
   catch (const std::exception& e)
   {
      check(std::string("the tests ran without an exception: ") + e.what(), false);
   }

   std::cerr << (failures == 0 ? "\nall checks passed\n" : "\nFAILURES\n");
   return failures == 0 ? 0 : -1;
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
