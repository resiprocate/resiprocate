#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#ifndef WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "rutil/Data.hxx"
#include "rutil/FdPoll.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Socket.hxx"

// Socket events through each FdPollGrp implementation the build has.

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

class RecordingItem : public FdPollItemIf
{
   public:
      RecordingItem() : mEvents(0), mMask(0) {}

      void processPollEvent(FdPollEventMask mask) override
      {
         ++mEvents;
         mMask |= mask;
      }

      int mEvents;
      FdPollEventMask mMask;
};

#ifndef WIN32
static Socket
boundUdpSocket()
{
   Socket fd = ::socket(AF_INET, SOCK_DGRAM, 0);
   sockaddr_in addr;
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   ::bind(fd, (sockaddr*)&addr, sizeof(addr));
   return fd;
}

// A daemon that has closed stdin can be handed a socket as descriptor 0.
// Asking for write events on it must work like on any other descriptor; an
// idle UDP socket is always writable.
static void
modifiesDescriptorZero(const string& impl)
{
   const string what = impl + ": modPollItem() applies to descriptor 0";
   const int savedStdin = ::dup(0);
   ::close(0);
   const Socket fd = boundUdpSocket();
   if (fd != 0)
   {
      cerr << "skip " << what << " (socket got descriptor " << fd << ")" << endl;
   }
   else
   {
      unique_ptr<FdPollGrp> group(FdPollGrp::create(impl.c_str()));
      RecordingItem item;
      FdPollItemHandle handle = group->addPollItem(fd, FPEM_Read, &item);
      group->waitAndProcess(0);
      const bool quietWhenReadOnly = (item.mEvents == 0);

      group->modPollItem(handle, FPEM_Read | FPEM_Write);
      group->waitAndProcess(100);
      check(quietWhenReadOnly && (item.mMask & FPEM_Write) != 0, what);
      group->delPollItem(handle);
   }
   ::close(fd);
   ::dup2(savedStdin, 0);
   ::close(savedStdin);
}

// A socket closed while still registered: poll() flags it POLLNVAL, which
// must reach the item as an error rather than as an empty event mask.
// Only the poll group can be asked this; epoll drops a closed descriptor
// silently, and select() fails outright.
static void
reportsClosedSocketAsError()
{
   const string what = "poll: a socket closed while registered is reported as an error";
   unique_ptr<FdPollGrp> group(FdPollGrp::create("poll"));
   RecordingItem item;
   const Socket fd = boundUdpSocket();
   FdPollItemHandle handle = group->addPollItem(fd, FPEM_Read, &item);
   ::close(fd);
   group->waitAndProcess(100);
   check(item.mEvents > 0 && (item.mMask & FPEM_Error) != 0, what);
   group->delPollItem(handle);
}
#endif

int
main()
{
   Log::initialize(Log::Cout, Log::None, Data::Empty);
   initNetwork();

#ifdef WIN32
   cerr << "skip: these checks need POSIX descriptors" << endl;
#else
   // getImplList() is "event|epoll|fdset|..."; "event" is an alias for the first
   const string impls(FdPollGrp::getImplList());
   bool havePoll = false;
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
      havePoll = havePoll || impl == "poll";
      modifiesDescriptorZero(impl);
   }

   if (havePoll)
   {
      reportsClosedSocketAsError();
   }
   else
   {
      cerr << "skip poll: not in this build" << endl;
   }
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