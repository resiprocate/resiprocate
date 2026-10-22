#include <atomic>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "rutil/Data.hxx"
#include "rutil/FdPoll.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Timer.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/dns/AresDns.hxx"
#include "rutil/dns/DnsAAAARecord.hxx"
#include "rutil/dns/DnsCnameRecord.hxx"
#include "rutil/dns/DnsHostRecord.hxx"
#include "rutil/dns/DnsNaptrRecord.hxx"
#include "rutil/dns/DnsSrvRecord.hxx"
#include "rutil/dns/DnsStub.hxx"
#include "rutil/dns/QueryTypes.hxx"

#if defined(USE_CARES)
#include <ares.h>
#endif

// AresDns configuration, without needing a network: the application's socket
// function, IPv6 name servers, checkDnsChange(), and the ReturnAllResults
// feature.

using namespace std;
using namespace resip;

static int failures = 0;

static void
check(bool condition, const string& what)
{
   cerr << (condition ? "ok   " : "FAIL ") << what << endl;
   if (!condition)
   {
      ++failures;
   }
}

static GenericIPAddress
nameserver(const char* address, unsigned short port = 0)
{
   if (strchr(address, ':'))
   {
      sockaddr_in6 addr;
      memset(&addr, 0, sizeof(addr));
      addr.sin6_family = AF_INET6;
      addr.sin6_port = htons(port);
      inet_pton(AF_INET6, address, &addr.sin6_addr);
      return GenericIPAddress(addr);
   }
   sockaddr_in addr;
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   inet_pton(AF_INET, address, &addr.sin_addr);
   return GenericIPAddress(addr);
}

class ResultSink : public DnsResultSink
{
   public:
      ResultSink() : mDone(false), mStatus(-1), mRecords(0) {}

      void onDnsResult(const DNSResult<DnsHostRecord>& r) override
      {
         mStatus = r.status;
         mRecords = r.records.size();
         mDone = true;
      }
      void onDnsResult(const DNSResult<DnsAAAARecord>&) override { mDone = true; }
      void onDnsResult(const DNSResult<DnsSrvRecord>&) override { mDone = true; }
      void onDnsResult(const DNSResult<DnsNaptrRecord>&) override { mDone = true; }
      void onDnsResult(const DNSResult<DnsCnameRecord>&) override { mDone = true; }

      bool mDone;
      int mStatus;
      size_t mRecords;
};

// One A lookup through a DnsStub on its own poll group; returns how long it
// took, or GiveUpMs if it never completed.
static const uint64_t GiveUpMs = 15000;

static uint64_t
lookupA(const DnsStub::NameserverList& servers, const Data& name, ResultSink& sink,
        AfterSocketCreationFuncPtr socketFunc = 0)
{
   unique_ptr<FdPollGrp> group(FdPollGrp::create());
   DnsStub stub(servers, socketFunc, 0, group.get());
   const uint64_t start = Timer::getTimeMs();
   stub.lookup<RR_A>(name, Protocol::Sip, &sink);
   while (!sink.mDone && Timer::getTimeMs() - start < GiveUpMs)
   {
      stub.processTimers();
      group->waitAndProcess(resipMin(stub.getTimeTillNextProcessMS(), 100u));
   }
   stub.setPollGrp(0);
   return sink.mDone ? Timer::getTimeMs() - start : GiveUpMs;
}

// Each call of the socket function: the transport type it was given, and
// whether the socket was connected yet
struct SocketCall
{
   int transportType;
   bool connected;
};
static vector<SocketCall> socketCalls;

static void
recordSocketCall(Socket s, int transportType, const char*, int)
{
   sockaddr_storage peer;
   socklen_t len = sizeof(peer);
   const bool connected = ::getpeername(s, (sockaddr*)&peer, &len) == 0;
   socketCalls.push_back(SocketCall{transportType, connected});
}

static string
describe(const vector<SocketCall>& calls)
{
   string s;
   for (size_t i = 0; i < calls.size(); ++i)
   {
      s += (i ? ", " : "") + string(calls[i].connected ? "connected" : "unconnected");
   }
   return to_string(calls.size()) + " calls: " + s;
}

// One UDP socket.  Like a SIP TCP connection, c-ares calls the socket
// function before the socket connects, and again once it is connected if
// post-connect calls are enabled.  The bundled ares makes one call, after
// connecting.
static void
callsSocketFunction()
{
   DnsStub::setDnsTimeoutAndTries(5, 1);
   DnsStub::NameserverList servers;
   servers.push_back(nameserver("127.0.0.1"));

   socketCalls.clear();
   ResultSink sink;
   lookupA(servers, "socket-function.invalid", sink, recordSocketCall);
#if defined(USE_CARES)
   check(sink.mDone && socketCalls.size() == 1 && !socketCalls[0].connected &&
         socketCalls[0].transportType == UDP,
         "c-ares calls the socket function once, before connecting, as UDP (" + describe(socketCalls) + ")");
#else
   check(sink.mDone && socketCalls.size() == 1 && socketCalls[0].connected,
         "the bundled ares calls the socket function once, after connecting (" + describe(socketCalls) + ")");
#endif

   ExternalDns::setEnablePostConnectSocketFuncCall(true);
   socketCalls.clear();
   ResultSink sinkPost;
   lookupA(servers, "socket-function-post-connect.invalid", sinkPost, recordSocketCall);
   ExternalDns::setEnablePostConnectSocketFuncCall(false);
#if defined(USE_CARES)
   check(sinkPost.mDone && socketCalls.size() == 2 && !socketCalls[0].connected && socketCalls[1].connected,
         "c-ares with post-connect calls enabled calls it again once connected (" + describe(socketCalls) + ")");
#else
   check(sinkPost.mDone && socketCalls.size() == 1 && socketCalls[0].connected,
         "the bundled ares makes its one call whether or not post-connect calls are enabled (" +
         describe(socketCalls) + ")");
#endif
}

static bool
haveIpv6Loopback()
{
   Socket s = ::socket(AF_INET6, SOCK_DGRAM, 0);
   if (s == INVALID_SOCKET)
   {
      return false;
   }
   sockaddr_in6 addr;
   memset(&addr, 0, sizeof(addr));
   addr.sin6_family = AF_INET6;
   addr.sin6_addr = in6addr_loopback;
   const bool ok = ::bind(s, (sockaddr*)&addr, sizeof(addr)) == 0;
   closeSocket(s);
   return ok;
}

// ::1 refuses the query, so the lookup finishes well inside the 5 second
// timeout, but only if the resolver really sends it to ::1.
static void
usesIpv6Nameserver()
{
#if !defined(USE_IPV6)
   cerr << "skip IPv6 name server (built without USE_IPV6)" << endl;
#else
   if (!haveIpv6Loopback())
   {
      cerr << "skip IPv6 name server (no IPv6 loopback here)" << endl;
      return;
   }
   DnsStub::setDnsTimeoutAndTries(5, 1);
   DnsStub::NameserverList servers;
   servers.push_back(nameserver("::1"));
   ResultSink sink;
   const uint64_t tookMs = lookupA(servers, "ipv6-nameserver.invalid", sink);
   check(sink.mDone && tookMs < 2500,
         "an IPv6 name server (::1) is used (" + to_string(tookMs) + " ms, limit 2500)");
#endif
}

static bool
changeDetected(const DnsStub::NameserverList& before, const DnsStub::NameserverList& after)
{
   AresDns dns;
   dns.init(before, 0, 0, 0, 0);
   dns.changeNameServers(after);
   return dns.checkDnsChange();
}

static void
checksDnsChange()
{
   DnsStub::NameserverList v4a, v4b, v6a, v6b, port53, port5353, mixedA, mixedB;
   v4a.push_back(nameserver("127.0.0.1"));
   v4b.push_back(nameserver("127.0.0.2"));
   v6a.push_back(nameserver("::1"));
   v6b.push_back(nameserver("::2"));
   port53.push_back(nameserver("127.0.0.1", 53));
   port5353.push_back(nameserver("127.0.0.1", 5353));
   mixedA.push_back(nameserver("127.0.0.1"));
   mixedA.push_back(nameserver("::1"));
   mixedB.push_back(nameserver("127.0.0.1"));
   mixedB.push_back(nameserver("::2"));

   check(!changeDetected(v4a, v4a), "checkDnsChange: same IPv4 server is no change");
   check(changeDetected(v4a, v4b), "checkDnsChange: different IPv4 server is a change");
#if !defined(USE_IPV6)
   // A build without IPv6 ignores IPv6 name servers, so changing one is no
   // change at all
   check(!changeDetected(mixedA, mixedB), "checkDnsChange: IPv6 servers are ignored without USE_IPV6");
#else
   check(!changeDetected(v6a, v6a), "checkDnsChange: same IPv6 server is no change");
   check(changeDetected(v6a, v6b), "checkDnsChange: different IPv6 server is a change");
   check(changeDetected(mixedA, mixedB), "checkDnsChange: a changed IPv6 server among IPv4 ones is a change");
#endif
#if defined(USE_CARES)
   check(changeDetected(port53, port5353), "checkDnsChange: different port is a change");
#else
   cerr << "skip checkDnsChange port (the bundled ares always uses port 53)" << endl;
#endif
}

#if defined(USE_CARES)
// Answers every query on a loopback UDP port: REFUSED, with one A record
// (127.0.0.9), or truncated, which sends the resolver to TCP.
class FakeNameserver
{
   public:
      enum Reply { Refuse, Answer, Truncate };

      explicit FakeNameserver(Reply reply) : mReply(reply), mStop(false)
      {
         mSocket = ::socket(AF_INET, SOCK_DGRAM, 0);
         sockaddr_in addr;
         memset(&addr, 0, sizeof(addr));
         addr.sin_family = AF_INET;
         addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
         ::bind(mSocket, (sockaddr*)&addr, sizeof(addr));
         socklen_t len = sizeof(addr);
         ::getsockname(mSocket, (sockaddr*)&addr, &len);
         mPort = ntohs(addr.sin_port);
         mThread = thread(&FakeNameserver::serve, this);
      }

      ~FakeNameserver()
      {
         mStop = true;
         mThread.join();
         closeSocket(mSocket);
      }

      unsigned short port() const { return mPort; }

   private:
      void serve()
      {
         while (!mStop)
         {
            FdSet fdset;
            fdset.setRead(mSocket);
            if (fdset.selectMilliSeconds(50) <= 0)
            {
               continue;
            }
            unsigned char buf[512];
            sockaddr_in from;
            socklen_t fromLen = sizeof(from);
            int len = (int)::recvfrom(mSocket, (char*)buf, sizeof(buf), 0, (sockaddr*)&from, &fromLen);
            if (len < 12)
            {
               continue;
            }
            // Keep the header and the one question; drop anything after it,
            // such as the EDNS OPT record newer c-ares adds to its queries.
            int end = 12;
            while (end < len && buf[end] != 0)
            {
               end += buf[end] + 1;       // a label
            }
            end += 1 + 4;                 // the root label, type and class
            if (end > len)
            {
               continue;
            }
            len = end;
            buf[2] |= 0x80;                       // QR: this is a response
            if (mReply == Truncate)
            {
               buf[2] |= 0x02;                    // TC
            }
            buf[3] = (unsigned char)(0x80 | (mReply == Refuse ? 5 : 0));   // RA, RCODE
            memset(buf + 6, 0, 6);                // no answer, authority or additional records
            if (mReply == Answer)
            {
               const unsigned char answer[] = {
                  0xc0, 0x0c,             // name: the question's
                  0x00, 0x01, 0x00, 0x01, // type A, class IN
                  0x00, 0x00, 0x00, 0x3c, // TTL 60
                  0x00, 0x04, 127, 0, 0, 9 };
               buf[7] = 1;                // ANCOUNT
               memcpy(buf + len, answer, sizeof(answer));
               len += sizeof(answer);
            }
            ::sendto(mSocket, (const char*)buf, len, 0, (sockaddr*)&from, fromLen);
         }
      }

      Reply mReply;
      atomic<bool> mStop;
      Socket mSocket;
      unsigned short mPort;
      thread mThread;
};

// The first server refuses, the second answers.  Normally c-ares skips the
// refusal and returns the second server's answer; with ReturnAllResults it
// returns the refusal, so that DnsStub can see the precise error.
static void
honoursReturnAllResults()
{
   FakeNameserver refusing(FakeNameserver::Refuse);
   FakeNameserver answering(FakeNameserver::Answer);
   DnsStub::NameserverList servers;
   servers.push_back(nameserver("127.0.0.1", refusing.port()));
   servers.push_back(nameserver("127.0.0.1", answering.port()));
   DnsStub::setDnsTimeoutAndTries(5, 1);

   ResultSink normal;
   lookupA(servers, "return-all-results-off.invalid", normal);
   check(normal.mDone && normal.mStatus == 0 && normal.mRecords == 1,
         "without ReturnAllResults the refusal is skipped and the next server answers (status " +
         to_string(normal.mStatus) + ", " + to_string(normal.mRecords) + " records)");

   // enableDnsFeatures() only adds features, so this runs last
   DnsStub::enableDnsFeatures(ExternalDns::ReturnAllResults);
   ResultSink all;
   lookupA(servers, "return-all-results-on.invalid", all);
   check(all.mDone && all.mStatus == ARES_EREFUSED && all.mRecords == 0,
         "with ReturnAllResults the first server's refusal is returned (status " +
         to_string(all.mStatus) + ")");
}

// A truncated answer makes c-ares retry over TCP, on the same port; nothing
// listens there, but the socket function is still called for the TCP socket
// before it connects.
static void
callsSocketFunctionForTcp()
{
   FakeNameserver truncating(FakeNameserver::Truncate);
   DnsStub::NameserverList servers;
   servers.push_back(nameserver("127.0.0.1", truncating.port()));
   DnsStub::setDnsTimeoutAndTries(5, 1);

   socketCalls.clear();
   ResultSink sink;
   lookupA(servers, "socket-function-tcp.invalid", sink, recordSocketCall);
   bool sawTcp = false;
   for (size_t i = 0; i < socketCalls.size(); ++i)
   {
      sawTcp = sawTcp || socketCalls[i].transportType == TCP;
   }
   check(sink.mDone && sawTcp, "c-ares calls the socket function for a TCP socket too, as TCP");
}
#endif

int
main()
{
   Log::initialize(Log::Cout, Log::None, Data::Empty);
   initNetwork();

   callsSocketFunction();
   usesIpv6Nameserver();
   checksDnsChange();
#if defined(USE_CARES)
   callsSocketFunctionForTcp();
   honoursReturnAllResults();
#else
   cerr << "skip ReturnAllResults (needs name server ports, which the bundled ares ignores)" << endl;
#endif

   cerr << (failures == 0 ? "PASS" : "FAILED") << endl;

   return failures == 0 ? 0 : 1;
}


/* ====================================================================

 Copyright (c) 2026, SIP Spectrum, Inc. http://www.sipspectrum.com
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