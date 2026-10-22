#include <cctype>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#ifdef __linux__
#include <dirent.h>
#endif

#include "rutil/Data.hxx"
#include "rutil/FdPoll.hxx"
#include "rutil/Lock.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Time.hxx"
#include "rutil/Timer.hxx"
#include "rutil/dns/DnsAAAARecord.hxx"
#include "rutil/dns/DnsCnameRecord.hxx"
#include "rutil/dns/DnsHostRecord.hxx"
#include "rutil/dns/DnsNaptrRecord.hxx"
#include "rutil/dns/DnsSrvRecord.hxx"
#include "rutil/dns/DnsStub.hxx"
#include "rutil/dns/DnsThread.hxx"
#include "rutil/dns/QueryTypes.hxx"

// Drives AresDns through every FdPollGrp implementation, and through the
// legacy FdSet loop, without needing a network.  Each arrangement makes three
// lookups:
//
// - one to 127.0.0.1, which refuses it at once (or answers it, if a resolver
//   listens there).  Either way the socket becomes ready, so the lookup must
//   finish well inside the 5 second resolver timeout.  If the resolver's
//   socket never reached the poll group, or the resolver left a refused
//   query to time out, it would wait out that timeout.  Only one query is in
//   flight: a second send on the same socket would consume the refusal
//   before the poll group saw it.
//
// - one to 127.0.0.1 and then 127.0.0.2, both refusing, so the resolver must
//   move on to the second server at once as well.
//
// - one to 192.0.2.1 (TEST-NET-1), which never answers, so the lookup can only
//   finish through the resolver timeout that processTimers() drives.

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

// Descriptors open in this process; -1 where that cannot be counted.
static int
openDescriptors()
{
#ifdef __linux__
   int count = 0;
   DIR* dir = opendir("/proc/self/fd");
   if (!dir)
   {
      return -1;
   }
   while (readdir(dir))
   {
      ++count;
   }
   closedir(dir);
   return count;
#else
   return -1;
#endif
}

class CountingSink : public DnsResultSink
{
   public:
      CountingSink() : mResults(0) {}

      void onDnsResult(const DNSResult<DnsHostRecord>&) override { count(); }
      void onDnsResult(const DNSResult<DnsAAAARecord>&) override { count(); }
      void onDnsResult(const DNSResult<DnsSrvRecord>&) override { count(); }
      void onDnsResult(const DNSResult<DnsNaptrRecord>&) override { count(); }
      void onDnsResult(const DNSResult<DnsCnameRecord>&) override { count(); }

      bool done()
      {
         Lock lock(mMutex);
         return mResults > 0;
      }

   private:
      void count()
      {
         Lock lock(mMutex);
         ++mResults;
      }

      Mutex mMutex;
      int mResults;
};

struct Server
{
   const char* address;
   const char* nextAddress;   // a second name server, or 0
   int timeoutSec;            // resolver timeout, one try
   uint64_t withinMs;         // the lookup must complete inside this
   const char* proves;
};

static const Server Refusing = { "127.0.0.1", 0, 5, 2500, "socket event reaches the resolver" };
static const Server FailingOver = { "127.0.0.1", "127.0.0.2", 5, 2500, "refused server fails over to the next" };
static const Server Silent = { "192.0.2.1", 0, 1, 10000, "resolver timeout fires" };

// Longest any lookup is waited for before it is counted as lost.
static const uint64_t GiveUpMs = 15000;

enum Arrangement
{
   GroupAtInit,      // DnsStub given the group in its constructor
   GroupAfterInit,   // DnsStub::setPollGrp() after AresDns::init()
   DnsThreadGroup,   // DnsThread swaps in its own group with the query open
   NoGroup           // the FdSet select() loop, as resip/stack/test/testDns uses
};

static void
run(const string& impl, Arrangement arrangement, const Server& server)
{
   string scenario;
   switch (arrangement)
   {
      case GroupAtInit:    scenario = impl + ", group at init"; break;
      case GroupAfterInit: scenario = impl + ", group set after init"; break;
      case DnsThreadGroup: scenario = "DnsThread takes over the socket"; break;
      case NoGroup:        scenario = "no group, FdSet loop"; break;
   }
   scenario += string(", ") + server.address;
   if (server.nextAddress)
   {
      scenario += string(" then ") + server.nextAddress;
   }

   // a name of its own, so the stub's cache never answers for this lookup
   string name;
   for (size_t i = 0; i < scenario.size(); ++i)
   {
      const char c = scenario[i];
      name += (isalnum((unsigned char)c) ? c : '-');
   }
   name += ".invalid";

   DnsStub::NameserverList servers;
   const char* addresses[] = { server.address, server.nextAddress };
   for (size_t i = 0; i < 2 && addresses[i]; ++i)
   {
      sockaddr_in addr;
      memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(53);
      inet_pton(AF_INET, addresses[i], &addr.sin_addr);
      servers.push_back(GenericIPAddress(addr));
   }
   DnsStub::setDnsTimeoutAndTries(server.timeoutSec, 1);

   const int before = openDescriptors();
   {
      unique_ptr<FdPollGrp> group;
      if (arrangement != NoGroup)
      {
         group.reset(FdPollGrp::create(arrangement == DnsThreadGroup ? 0 : impl.c_str()));
      }

      CountingSink sink;
      DnsStub stub(servers, 0, 0, arrangement == GroupAtInit || arrangement == DnsThreadGroup ? group.get() : 0);
      if (arrangement == GroupAfterInit)
      {
         stub.setPollGrp(group.get());
      }

      const uint64_t start = Timer::getTimeMs();
      stub.lookup<RR_A>(Data(name), Protocol::Sip, &sink);

      if (arrangement == DnsThreadGroup)
      {
         stub.processTimers();   // sends the query, from the first group
         DnsThread thread(stub);
         thread.run();
         while (!sink.done() && Timer::getTimeMs() - start < GiveUpMs)
         {
            sleepMs(10);
         }
         thread.shutdown();
         thread.join();
      }
      else
      {
         while (!sink.done() && Timer::getTimeMs() - start < GiveUpMs)
         {
            if (group.get())
            {
               stub.processTimers();
               group->waitAndProcess(resipMin(stub.getTimeTillNextProcessMS(), 100u));
            }
            else
            {
               FdSet fdset;
               stub.buildFdSet(fdset);
               fdset.selectMilliSeconds(resipMin(stub.getTimeTillNextProcessMS(), 100u));
               stub.process(fdset);
            }
         }
      }

      const uint64_t tookMs = Timer::getTimeMs() - start;
      check(sink.done() && tookMs < server.withinMs,
            scenario + ": " + server.proves + " (" + (sink.done() ? "done" : "no result") +
            " after " + to_string(tookMs) + " ms, limit " + to_string(server.withinMs) + ")");

      if (group.get())
      {
         stub.setPollGrp(0);
      }
   }

   const int after = openDescriptors();
   if (before >= 0 && after >= 0)
   {
      check(after == before, scenario + ": closes every socket it opened");
   }
}

static void
runAll(const string& impl, Arrangement arrangement)
{
   run(impl, arrangement, Refusing);
   run(impl, arrangement, FailingOver);
   run(impl, arrangement, Silent);
}

int
main()
{
   Log::initialize(Log::Cout, Log::None, Data::Empty);
   initNetwork();

   // getImplList() is "event|epoll|fdset|..."; "event" is an alias for the first
   const string impls(FdPollGrp::getImplList());
   for (size_t start = 0; start < impls.size(); )
   {
      size_t end = impls.find('|', start);
      if (end == string::npos)
      {
         end = impls.size();
      }
      const string impl = impls.substr(start, end - start);
      start = end + 1;
      if (impl == "event")
      {
         continue;
      }
      runAll(impl, GroupAtInit);
      runAll(impl, GroupAfterInit);
   }

   runAll("", DnsThreadGroup);
   runAll("", NoGroup);

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