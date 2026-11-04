#include <cstdio>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#ifndef WIN32
#include <netinet/in.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
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

// A socket closed before delPollItem() of an item that handles failures:
// the group must carry on, and the next item handed the same descriptor
// must get its events.
static void
deletesClosedSocket(const string& impl)
{
   const string what = impl + ": delPollItem() after close() leaves the group usable";
   unique_ptr<FdPollGrp> group(FdPollGrp::create(impl.c_str()));
   RecordingItem closed;
   const Socket fd = boundUdpSocket();
   FdPollItemHandle handle = group->addPollItemNotifyOnFailure(fd, FPEM_Read, &closed);
   ::close(fd);
   group->delPollItem(handle);

   RecordingItem reused;
   const Socket next = boundUdpSocket();
   handle = group->addPollItem(next, FPEM_Write, &reused);
   group->waitAndProcess(100);
   check(closed.mEvents == 0 && (reused.mMask & FPEM_Write) != 0, what);
   group->delPollItem(handle);
   ::close(next);
}

// A socket closed while registered, then asked for other events: epoll
// rejects the change, and the item must hear of it as an error rather
// than wait for events that will never come.
static void
reportsFailedModifyAsError(const string& impl)
{
   const string what = impl + ": modPollItem() on a closed socket is reported as an error";
   unique_ptr<FdPollGrp> group(FdPollGrp::create(impl.c_str()));
   RecordingItem item;
   const Socket fd = boundUdpSocket();
   FdPollItemHandle handle = group->addPollItemNotifyOnFailure(fd, FPEM_Read, &item);
   group->modPollItem(handle, FPEM_Read);
   ::close(fd);
   group->modPollItem(handle, FPEM_Read|FPEM_Write);
   check(item.mEvents == 0, what + ": delivery is deferred");
   group->waitAndProcess(100);
   check(item.mEvents == 1 && (item.mMask & FPEM_Error) != 0, what);
   group->delPollItem(handle);
}

// A hangup must allow the last bytes to be read before EOF, rather than
// become a fatal error that makes Connection discard those bytes.
static void
reportsHangup(const string& impl)
{
   const string what = impl + ": hangup preserves unread data";
   int pair[2];
   if (::socketpair(AF_UNIX, SOCK_STREAM, 0, pair) < 0)
   {
      cerr << "skip " << what << " (socketpair() failed)" << endl;
      return;
   }
   unique_ptr<FdPollGrp> group(FdPollGrp::create(impl.c_str()));
   RecordingItem item;
   FdPollItemHandle handle = group->addPollItem(pair[0], FPEM_Read, &item);
   const char payload[] = "pending data";
   const ssize_t sent = ::send(pair[1], payload, sizeof(payload), 0);
   ::close(pair[1]);
   group->waitAndProcess(100);
   check(sent == static_cast<ssize_t>(sizeof(payload)) && item.mEvents > 0 && (item.mMask & FPEM_Read) != 0, what);
   check((item.mMask & FPEM_Error) == 0, what + ": HUP is not a fatal error");
   char received[sizeof(payload)];
   const ssize_t bytes = ::recv(pair[0], received, sizeof(received), MSG_DONTWAIT);
   check(bytes == static_cast<ssize_t>(sizeof(payload)) && memcmp(received, payload, sizeof(payload)) == 0,
         what + ": final bytes");
   check(::recv(pair[0], received, sizeof(received), MSG_DONTWAIT) == 0, what + ": EOF");
   group->delPollItem(handle);
   ::close(pair[0]);
}

// epoll refuses regular files; that must reach the item as an error
// instead of taking the process down.
static void
reportsFailedAddAsError()
{
   const string what = "epoll: addPollItem() of a regular file is reported as an error";
   FILE* file = ::tmpfile();
   if (file == NULL)
   {
      cerr << "skip " << what << " (tmpfile() failed)" << endl;
      return;
   }
   unique_ptr<FdPollGrp> group(FdPollGrp::create("epoll"));
   RecordingItem item;
   FdPollItemHandle handle = group->addPollItemNotifyOnFailure(::fileno(file), FPEM_Read, &item);
   group->waitAndProcess(0);
   check(item.mEvents > 0 && (item.mMask & FPEM_Error) != 0, what);
   group->delPollItem(handle);
   ::fclose(file);
}

enum FatalCase { FatalAdd, FatalModify, FatalDelete, FatalLiveDuplicate };

// Items added without addPollItemNotifyOnFailure() keep the fail-fast
// behavior, and so does adding a descriptor another live item still holds.
static void
retainsFatalControlFailures(FatalCase fatalCase)
{
   const char* names[] = { "epoll: failed ADD without opt-in remains fatal",
                           "epoll: failed MOD without opt-in remains fatal",
                           "epoll: failed DEL without opt-in remains fatal",
                           "epoll: adding a descriptor owned by a live item remains fatal" };
   const string what = names[fatalCase];
   const pid_t child = ::fork();
   if(child == 0)
   {
      const rlimit noCore = {0, 0};
      ::setrlimit(RLIMIT_CORE, &noCore);
      unique_ptr<FdPollGrp> group(FdPollGrp::create("epoll"));
      RecordingItem item;
      RecordingItem other;
      if(fatalCase == FatalAdd)
      {
         FILE* file = ::tmpfile();
         if(file == NULL) ::_exit(2);
         group->addPollItem(::fileno(file), FPEM_Read, &item);
      }
      else
      {
         const Socket fd = boundUdpSocket();
         if(fd == INVALID_SOCKET) ::_exit(2);
         if(fatalCase == FatalLiveDuplicate)
         {
            group->addPollItemNotifyOnFailure(fd, FPEM_Read, &item);
            group->addPollItemNotifyOnFailure(fd, FPEM_Read, &other);
         }
         else
         {
            const FdPollItemHandle handle = group->addPollItem(fd, FPEM_Read, &item);
            ::close(fd);
            if(fatalCase == FatalModify)
            {
               group->modPollItem(handle, FPEM_Read|FPEM_Write);
            }
            else
            {
               group->delPollItem(handle);
            }
         }
      }
      ::_exit(0);
   }
   if(child < 0)
   {
      check(false, what + " (fork() failed)");
      return;
   }
   int status = 0;
   pid_t waited;
   do
   {
      waited = ::waitpid(child, &status, 0);
   } while(waited < 0 && errno == EINTR);
   check(waited == child && WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT, what);
}

// Whether a select() over buildFdSet() would return at once
static bool
selectReady(FdPollGrp& group)
{
   FdSet fdset;
   group.buildFdSet(fdset);
   fdset.selectMilliSeconds(0);
   return fdset.readyToRead(group.getEPollFd());
}

// A select()-driven loop sees only the epoll fd: a queued error must make
// it readable, and stop doing so once the error is delivered or cancelled.
static void
wakesSelectForQueuedErrors()
{
   unique_ptr<FILE, decltype(&::fclose)> file(::tmpfile(), &::fclose);
   if(!file)
   {
      check(false, "epoll: create file for select wakeup test");
      return;
   }
   unique_ptr<FdPollGrp> group(FdPollGrp::create("epoll"));
   check(!selectReady(*group), "epoll: idle group is not readable");

   RecordingItem item;
   FdPollItemHandle handle = group->addPollItemNotifyOnFailure(::fileno(file.get()), FPEM_Read, &item);
   check(selectReady(*group), "epoll: a queued error makes the epoll fd readable");
   FdSet fdset;
   group->buildFdSet(fdset);
   fdset.selectMilliSeconds(0);
   group->processFdSet(fdset);
   check(item.mEvents == 1 && (item.mMask & FPEM_Error) != 0,
         "epoll: processFdSet() delivers the queued error");
   check(!selectReady(*group), "epoll: a delivered error no longer wakes select()");
   group->delPollItem(handle);

   handle = group->addPollItemNotifyOnFailure(::fileno(file.get()), FPEM_Read, &item);
   group->delPollItem(handle);
   check(!selectReady(*group), "epoll: a cancelled error no longer wakes select()");
   group->waitAndProcess(0);
   check(item.mEvents == 1, "epoll: a cancelled error is not delivered");
}

class BoundedWritableItem : public RecordingItem
{
   public:
      BoundedWritableItem(FdPollGrp& group, Socket fd, bool edge)
         : mGroup(group), mFd(fd),
           mHandle(group.addPollItem(fd, FPEM_Write | (edge ? FPEM_Edge : 0), this))
      {
      }

      ~BoundedWritableItem() override
      {
         mGroup.delPollItem(mHandle);
         ::close(mFd);
      }

      void processPollEvent(FdPollEventMask mask) override
      {
         RecordingItem::processPollEvent(mask);
         // Let the old unbounded loop finish, so this test fails instead of hanging.
         if(mEvents >= 2)
         {
            mGroup.modPollItem(mHandle, FPEM_Read);
         }
      }

   private:
      FdPollGrp& mGroup;
      Socket mFd;
      FdPollItemHandle mHandle;
};

static void
dispatchesReadyDescriptorsFairly(bool edge)
{
   const string what = edge ? "epoll ET: ready descriptors span multiple waits" :
                              "epoll LT: continuously ready descriptors yield to the caller";
   unique_ptr<FdPollGrp> group(FdPollGrp::create("epoll"));
   vector<unique_ptr<BoundedWritableItem>> items;
   // One more descriptor than a single wait dispatches
   const unsigned count = FdPollGrp::EpollMaxEventsPerWait + 1;
   items.reserve(count);
   for(unsigned i = 0; i < count; ++i)
   {
      const Socket fd = boundUdpSocket();
      if(fd == INVALID_SOCKET)
      {
         check(false, what + " (socket creation failed)");
         return;
      }
      items.emplace_back(new BoundedWritableItem(*group, fd, edge));
   }

   group->waitAndProcess(0);
   unsigned callbacks = 0;
   bool noErrors = true;
   for(const auto& item : items)
   {
      callbacks += item->mEvents;
      noErrors = noErrors && (item->mMask & FPEM_Error) == 0;
   }
   check(noErrors && callbacks > 0 && callbacks < count, what + ": bounded dispatch per call");

   for(unsigned i = 0; i < 3; ++i)
   {
      group->waitAndProcess(0);
   }
   unsigned serviced = 0;
   for(const auto& item : items)
   {
      serviced += (item->mMask & FPEM_Write) != 0;
   }
   check(serviced == count, what + ": every descriptor is serviced");
}

class RetryingErrorItem : public RecordingItem
{
   public:
      explicit RetryingErrorItem(FdPollGrp& group) : mGroup(group), mHandle(nullptr) {}

      void processPollEvent(FdPollEventMask mask) override
      {
         RecordingItem::processPollEvent(mask);
         mGroup.modPollItem(mHandle, FPEM_Read);
      }

      FdPollGrp& mGroup;
      FdPollItemHandle mHandle;
};

static void
defersErrorsQueuedByCallbacks()
{
   unique_ptr<FdPollGrp> group(FdPollGrp::create("epoll"));
   RecordingItem ready;
   const Socket readyFd = boundUdpSocket();
   const FdPollItemHandle readyHandle = group->addPollItem(readyFd, FPEM_Write, &ready);
   RetryingErrorItem failed(*group);
   const Socket failedFd = boundUdpSocket();
   failed.mHandle = group->addPollItemNotifyOnFailure(failedFd, FPEM_Read, &failed);
   ::close(failedFd);
   group->modPollItem(failed.mHandle, FPEM_Read);

   FdSet fdset;
   group->buildFdSet(fdset);
   fdset.selectMilliSeconds(0);
   group->processFdSet(fdset);
   check(failed.mEvents == 1 && (failed.mMask & FPEM_Error) != 0 && ready.mEvents == 1,
         "epoll: processFdSet() defers errors queued by an error callback");

   // The error queued by the callback must wake the next select() too.
   // Stop the writable socket from making the epoll fd readable instead.
   group->modPollItem(readyHandle, FPEM_Read);
   check(selectReady(*group), "epoll: an error queued by a callback keeps the epoll fd readable");
   FdSet next;
   group->buildFdSet(next);
   next.selectMilliSeconds(0);
   group->processFdSet(next);
   check(failed.mEvents == 2, "epoll: the next processFdSet() delivers the newly queued error");
   group->waitAndProcess(0);
   check(failed.mEvents == 3, "epoll: so does the next wait");
   group->delPollItem(failed.mHandle);
   group->delPollItem(readyHandle);
   ::close(readyFd);
}

class ReplacingErrorItem : public RecordingItem
{
   public:
      ReplacingErrorItem(FdPollGrp& group, int fd, FdPollItemHandle& handle,
                         RecordingItem& replacement)
         : mGroup(group), mFd(fd), mHandle(handle), mReplacement(replacement) {}

      void processPollEvent(FdPollEventMask mask) override
      {
         RecordingItem::processPollEvent(mask);
         mGroup.delPollItem(mHandle);
         mHandle = mGroup.addPollItemNotifyOnFailure(mFd, FPEM_Read, &mReplacement);
         // Repeated failures must not duplicate the newly queued callback.
         mGroup.modPollItem(mHandle, FPEM_Read);
         mGroup.modPollItem(mHandle, FPEM_Write);
      }

   private:
      FdPollGrp& mGroup;
      int mFd;
      FdPollItemHandle& mHandle;
      RecordingItem& mReplacement;
};

static void
defersErrorsAfterRegistrationReuse()
{
   unique_ptr<FILE, decltype(&::fclose)> first(::tmpfile(), &::fclose);
   unique_ptr<FILE, decltype(&::fclose)> second(::tmpfile(), &::fclose);
   if(!first || !second)
   {
      check(false, "epoll: create files for registration reuse test");
      return;
   }
   unique_ptr<FdPollGrp> group(FdPollGrp::create("epoll"));
   RecordingItem original;
   RecordingItem replacement;
   const int fd = ::fileno(second.get());

   // Leave an obsolete error before the trigger's entry. The latest error
   // for this fd must retain its position after the trigger, not replace it.
   FdPollItemHandle handle = group->addPollItemNotifyOnFailure(fd, FPEM_Read, &original);
   group->delPollItem(handle);
   ReplacingErrorItem trigger(*group, fd, handle, replacement);
   const FdPollItemHandle triggerHandle =
      group->addPollItemNotifyOnFailure(::fileno(first.get()), FPEM_Read, &trigger);
   handle = group->addPollItemNotifyOnFailure(fd, FPEM_Read, &original);

   group->waitAndProcess(0);
   check(trigger.mEvents == 1 && original.mEvents == 0 && replacement.mEvents == 0,
         "epoll: callback fd reuse cancels old errors and defers the replacement's failure");
   group->waitAndProcess(0);
   check(replacement.mEvents == 1 && (replacement.mMask & FPEM_Error) != 0,
         "epoll: repeated ctl failures deliver one error on the next wait");
   group->waitAndProcess(0);
   check(replacement.mEvents == 1, "epoll: no duplicate error remains after delivery");

   group->modPollItem(handle, FPEM_Read);
   group->delPollItem(handle);
   group->delPollItem(triggerHandle);
   group->waitAndProcess(0);
   check(replacement.mEvents == 1, "epoll: removal cancels an undelivered error");
   check(!selectReady(*group), "epoll: no wakeup remains after cancellation");
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
      deletesClosedSocket(impl);
      // select() fails outright on a closed descriptor, and has no hangup
      if (impl == "epoll" || impl == "poll")
      {
         reportsFailedModifyAsError(impl);
         reportsHangup(impl);
      }
      if (impl == "epoll")
      {
         reportsFailedAddAsError();
         retainsFatalControlFailures(FatalAdd);
         retainsFatalControlFailures(FatalModify);
         retainsFatalControlFailures(FatalDelete);
         retainsFatalControlFailures(FatalLiveDuplicate);
         wakesSelectForQueuedErrors();
         dispatchesReadyDescriptorsFairly(false);
         dispatchesReadyDescriptorsFairly(true);
         defersErrorsQueuedByCallbacks();
         defersErrorsAfterRegistrationReuse();
      }
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