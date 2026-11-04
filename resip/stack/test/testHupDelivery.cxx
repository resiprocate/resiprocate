#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

// A TCP peer that sends a request and closes at once makes WSAPoll report
// the data together with POLLHUP. The stack must read the request and then
// close the connection on EOF, rather than treat the hangup as a socket
// error and drop the unread request.
//
//  1. Raw WSAPoll revents, and the FdPollGrp("poll") callback mask, for an
//     accepted socket whose peer sent data and closed.
//  2. A SipStack on the poll group, driven by SipStack::process(), so the
//     data and the FIN are both queued before the stack polls.
//  3. The same through EventStackThread, repeated.
//
// testFdPollGrp covers the hangup mapping for poll() and epoll on POSIX.

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "resip/stack/EventStackThread.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/Data.hxx"
#include "rutil/FdPoll.hxx"
#include "rutil/Log.hxx"
#include "rutil/Socket.hxx"

using namespace resip;
using namespace std;

#if defined(WIN32) && defined(RESIP_POLL_IMPL_POLL)

static int failures = 0;

static void
check(bool ok, const string& what)
{
   cout << (ok ? "ok   " : "FAIL ") << what << endl;
   if (!ok)
   {
      ++failures;
   }
}

// Counts the log lines that show how a connection was closed
class CloseLogWatcher : public ExternalLogger
{
   public:
      virtual bool operator()(Log::Level level, const Subsystem& subsystem,
                              const Data& appName, const char* file, int line,
                              const Data& message, const Data& messageWithHeaders,
                              const Data& instanceName)
      {
         if (message.find("Exception on socket") != Data::npos &&
             message.find("; closing connection") != Data::npos)
         {
            ++mErrorCloses;      // Connection::processPollEvent() got FPEM_Error
         }
         if (message.find("Closing connection bytesRead") != Data::npos)
         {
            ++mReadCloses;       // Connection::performReads() got EOF
         }
         return level <= Log::Warning;  // keep the console readable
      }

      void reset()
      {
         mErrorCloses = 0;
         mReadCloses = 0;
      }

      std::atomic<int> mErrorCloses{0};
      std::atomic<int> mReadCloses{0};
};

static CloseLogWatcher watcher;

static string
maskString(unsigned mask)
{
   string s;
   if (mask & FPEM_Read)  s += "Read ";
   if (mask & FPEM_Write) s += "Write ";
   if (mask & FPEM_Error) s += "Error ";
   return s.empty() ? "none" : s;
}

static string
reventsString(short revents)
{
   string s;
   if (revents & POLLRDNORM) s += "POLLRDNORM ";
   if (revents & POLLRDBAND) s += "POLLRDBAND ";
   if (revents & POLLWRNORM) s += "POLLWRNORM ";
   if (revents & POLLHUP)    s += "POLLHUP ";
   if (revents & POLLERR)    s += "POLLERR ";
   if (revents & POLLNVAL)   s += "POLLNVAL ";
   return s.empty() ? "none" : s;
}

static Socket
connectTo(int port)
{
   Socket s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   sockaddr_in addr;
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons((u_short)port);
   addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   if (::connect(s, (sockaddr*)&addr, sizeof(addr)) != 0)
   {
      cerr << "connect() to port " << port << " failed: " << WSAGetLastError() << endl;
      closeSocket(s);
      return INVALID_SOCKET;
   }
   return s;
}

static string
optionsRequest(int port, int n)
{
   const string id = to_string(n);
   return "OPTIONS sip:test@127.0.0.1:" + to_string(port) + ";transport=tcp SIP/2.0\r\n"
          "Via: SIP/2.0/TCP 127.0.0.1:5999;branch=z9hG4bK-hup-" + id + ";rport\r\n"
          "Max-Forwards: 70\r\n"
          "From: <sip:client@127.0.0.1>;tag=hup" + id + "\r\n"
          "To: <sip:test@127.0.0.1>\r\n"
          "Call-ID: hup-test-" + id + "\r\n"
          "CSeq: 1 OPTIONS\r\n"
          "Content-Length: 0\r\n"
          "\r\n";
}

// Sends msg and closes the socket straight away
static bool
sendAndClose(Socket s, const string& msg)
{
   int sent = ::send(s, msg.data(), (int)msg.size(), 0);
   closeSocket(s);
   return sent == (int)msg.size();
}

static bool
isExpectedRequest(SipMessage* msg, int n)
{
   return msg && msg->isRequest() && msg->method() == OPTIONS &&
          msg->header(h_CallId).value() == Data("hup-test-" + to_string(n));
}

class RecordingItem : public FdPollItemIf
{
   public:
      virtual void processPollEvent(FdPollEventMask mask)
      {
         ++mEvents;
         mMask |= mask;
      }
      int mEvents = 0;
      unsigned mMask = 0;
};

// 1. What WSAPoll reports, and what FdPollGrp passes on
static void
pollMaskForDataThenClose()
{
   Socket listener = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   sockaddr_in addr;
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   ::bind(listener, (sockaddr*)&addr, sizeof(addr));
   ::listen(listener, 1);
   int len = sizeof(addr);
   ::getsockname(listener, (sockaddr*)&addr, &len);
   Socket client = connectTo(ntohs(addr.sin_port));
   Socket server = ::accept(listener, NULL, NULL);
   closeSocket(listener);
   if (client == INVALID_SOCKET || server == INVALID_SOCKET)
   {
      check(false, "poll: socket setup");
      return;
   }
   makeSocketNonBlocking(server);

   const string payload = "final data before FIN";
   check(sendAndClose(client, payload), "poll: peer sends data and closes");
   this_thread::sleep_for(chrono::milliseconds(200));

   WSAPOLLFD pfd;
   pfd.fd = server;
   pfd.events = POLLRDNORM;
   pfd.revents = 0;
   int n = WSAPoll(&pfd, 1, 0);
   cout << "     raw WSAPoll returned " << n << ", revents = " << reventsString(pfd.revents) << endl;

   unique_ptr<FdPollGrp> group(FdPollGrp::create("poll"));
   RecordingItem item;
   FdPollItemHandle handle = group->addPollItem(server, FPEM_Read, &item);
   group->waitAndProcess(100);
   cout << "     FdPollGrp(poll) callback mask = " << maskString(item.mMask) << endl;
   check(item.mEvents > 0 && (item.mMask & FPEM_Read) != 0, "poll: hangup with data is reported as readable");
   check((item.mMask & FPEM_Error) == 0, "poll: hangup with data is not reported as an error");

   char buf[128];
   int got = ::recv(server, buf, sizeof(buf), 0);
   check(got == (int)payload.size(), "poll: the final data is still readable");
   check(::recv(server, buf, sizeof(buf), 0) == 0, "poll: then EOF");
   group->delPollItem(handle);
   closeSocket(server);
}

// 2. SipStack on the poll group, driven from this thread
static void
stackDeliversFinalRequest(int port)
{
   FdPollGrp* pollGrp = FdPollGrp::create("poll");
   SipStackOptions options;
   options.mPollGrp = pollGrp;
   SipStack* stack = new SipStack(options);
   stack->addTransport(TCP, port, V4, StunDisabled, "127.0.0.1");

   Socket client = connectTo(port);
   if (client == INVALID_SOCKET)
   {
      check(false, "stack: connect to the stack");
   }
   else
   {
      // Let the stack accept and register the connection
      for (int i = 0; i < 10; ++i)
      {
         stack->process(20);
      }
      watcher.reset();
      check(sendAndClose(client, optionsRequest(port, 0)), "stack: peer sends OPTIONS and closes");
      // Queue both the request and the FIN before the stack polls again
      this_thread::sleep_for(chrono::milliseconds(200));

      SipMessage* msg = NULL;
      for (int i = 0; i < 100 && !msg; ++i)
      {
         stack->process(20);
         msg = stack->receive();
      }
      for (int i = 0; i < 10; ++i)
      {
         stack->process(20);
      }
      check(isExpectedRequest(msg, 0), "stack: the request sent just before the close is delivered");
      cout << "     closes via error path " << watcher.mErrorCloses
           << ", via EOF " << watcher.mReadCloses << endl;
      check(watcher.mErrorCloses == 0 && watcher.mReadCloses == 1,
            "stack: the connection closes on EOF, not as a socket exception");
      delete msg;
   }
   delete stack;
   delete pollGrp;
}

// 3. EventStackThread, repeated
static void
eventThreadDeliversFinalRequests(int port, int runs)
{
   FdPollGrp* pollGrp = FdPollGrp::create("poll");
   EventThreadInterruptor* intr = new EventThreadInterruptor(*pollGrp);
   SipStackOptions options;
   options.mPollGrp = pollGrp;
   options.mAsyncProcessHandler = intr;
   SipStack* stack = new SipStack(options);
   stack->addTransport(TCP, port, V4, StunDisabled, "127.0.0.1");
   EventStackThread* thread = new EventStackThread(*stack, *intr, *pollGrp);
   thread->run();

   watcher.reset();
   int delivered = 0;
   for (int n = 1; n <= runs; ++n)
   {
      Socket client = connectTo(port);
      if (client == INVALID_SOCKET)
      {
         continue;
      }
      this_thread::sleep_for(chrono::milliseconds(50));  // accepted and registered
      sendAndClose(client, optionsRequest(port, n));
      SipMessage* msg = NULL;
      for (int i = 0; i < 200 && !msg; ++i)
      {
         msg = stack->receive();
         if (!msg)
         {
            this_thread::sleep_for(chrono::milliseconds(10));
         }
      }
      if (isExpectedRequest(msg, n))
      {
         ++delivered;
      }
      delete msg;
   }
   this_thread::sleep_for(chrono::milliseconds(200));
   cout << "     delivered " << delivered << " of " << runs
        << "; closes via error path " << watcher.mErrorCloses
        << ", via EOF " << watcher.mReadCloses << endl;
   check(delivered == runs, "event thread: every request sent just before the close is delivered");
   check(watcher.mErrorCloses == 0, "event thread: no connection closes as a socket exception");

   thread->shutdown();
   thread->join();
   delete thread;
   delete stack;
   delete intr;
   delete pollGrp;
}

int
main(int argc, char** argv)
{
   initNetwork();
   Log::initialize(Log::Cout, Log::Debug, argv[0], watcher);

   const int basePort = argc > 1 ? atoi(argv[1]) : 25060;
   pollMaskForDataThenClose();
   stackDeliversFinalRequest(basePort);
   eventThreadDeliversFinalRequests(basePort + 1, 20);

   cerr << (failures == 0 ? "PASS" : "FAILED") << endl;
   return failures == 0 ? 0 : 1;
}

#else

int
main()
{
   cerr << "SKIP: needs WSAPoll (the hangup mapping on POSIX is in testFdPollGrp)" << endl;
   return 0;
}

#endif


/* ====================================================================

 Copyright (c) 2026, SIP Spectrum, Inc. https://www.sipspectrum.com
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