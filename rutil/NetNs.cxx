#ifdef USE_NETNS

#define _GNU_SOURCE
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

///////////////////////////////////////////////////////////////////////////////
// Note: valgrind v3.7.0 does not support the setns() call, so we use the
// following to check for valgrind and do a crude fix-up.
///////////////////////////////////////////////////////////////////////////////
#ifdef BUILD_FOR_VALGRIND
#include <valgrind/valgrind.h>
#endif
#ifndef RUNNING_ON_VALGRIND
#define RUNNING_ON_VALGRIND 0
#warning Defaulting RUNNING_ON_VALGRIND to 0
#endif

#ifndef HAVE_SETNS
#    include <linux/unistd.h>
#endif

#include "rutil/NetNs.hxx"
#include "rutil/FileSystem.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT
#define DEFAULT_NET_NAMESPACE "/proc/1/ns/net"
#define NET_NAMESPACE_DIR "/var/run/netns/"

using namespace resip;

// Static declarations
HashMap<int, Data> NetNs::sIdDictionaryToNetNs;
HashMap<Data, int> NetNs::sNetNsDictionaryToId;

// Methods
int NetNs::setNs(const Data& netNs)
{
   int netNsFd = openNetNs(netNs);
   if(netNsFd < 0)
   {
      Data deviceName;
      makeNetNsDeviceName(netNs, deviceName);

      if(errno == EACCES)
      {
         // This is a bit heuristic, but let it go if we did not have permission to open the global
         // netns.  This is probably because we are not running as root/privileged user and cannot
         // open the global net namespace file.  If thats the case, odds are that we were also not
         // able to change to a non-global/default net namespaces.  So we should still be in the
         // global net namespace.  Let this be a no-op
         if(netNs.empty())
         {
            WarningLog(<< "Permission denied opening global netns.  Assuming already using global netns.");
         }
         else
         {
            throw Exception("Permission denied opening \"" + deviceName + "\" to set setns, may need to run as root",
                  __FILE__,__LINE__);
         }
      }
      else
      {
         throw Exception("Can't open name space \"" + deviceName + "\" errno: " + Data((int) errno), __FILE__,__LINE__);
      }
   }

   // Should only get here with an invalid netNsFd when we don't have permission
   // to open global netns file.  As described above.  So no-op if we have an invalid netNsFd for
   // the global netns namespace
   if(netNsFd >= 0)
   {
      DebugLog(<< "setns(\"" << netNs << "\"(" << netNsFd << "))");
      int error;

#ifdef HAVE_SETNS
#define DO_SETNS(fd, flag) setns(fd, flag)
#else
// Make system call directly
#define DO_SETNS(fd, flag)  syscall(__NR_setns, fd, flag)
#endif

   ///////////////////////////////////////////////////////////////////////////////
   // Note: valgrind v3.7.0 does not support the setns() call.  In order to work
   // around this, we make the syscall to setns, indicate that an "error" occurred, and
   // then suppress throwing the exception.  It's kind of an ugly hack, but it
   // does let us run valgrind on our code.
   ///////////////////////////////////////////////////////////////////////////////

      error = DO_SETNS(netNsFd, CLONE_NEWNET);

      if(error)
      {
         // setns normally closes the file descriptor when it is done, but because
         // it failed close the FD here just in case.
         close(netNsFd);

         if (!(RUNNING_ON_VALGRIND)) {
            throw Exception("Can't change to name space \"" + netNs + "\"(fd:" +
                  Data((int) netNsFd) + "), error: " + Data((int) error) +
                  " errno: " + Data((int) errno), __FILE__,__LINE__);
         }
      }

   // Add name to the dictionary if it is not already in there
   }
   return(getNetNsId(netNs, true));
}

int NetNs::getPublicNetNs(std::vector<Data>& netNsNames)
{
   // Add global netns name
   int nameCount = 1;
   netNsNames.push_back("");

   FileSystem::Directory namespaceDir(NET_NAMESPACE_DIR);
   for(FileSystem::Directory::iterator dirItr = namespaceDir.begin(); dirItr != namespaceDir.end(); ++dirItr)
   {
      if(!dirItr.is_directory())
      {
         netNsNames.push_back(*dirItr);
         nameCount++;
      }
   }
   return(nameCount);
}

void NetNs::makeNetNsDeviceName(const Data& netNs, Data& netNsDeviceName)
{
   if(netNs.empty())
   {
      // empty string means the global namespace.  As it does not
      // have a name, we instead open and switch to the namespace used
      // by process 1 which should be the global namespace
      netNsDeviceName = DEFAULT_NET_NAMESPACE;
   }
   else
   {
      netNsDeviceName = NET_NAMESPACE_DIR;
      netNsDeviceName += netNs;
   }
}

int NetNs::openNetNs(const Data& netNs)
{
   Data nameSpacePath;
   makeNetNsDeviceName(netNs, nameSpacePath);
   int netNsFd = open(nameSpacePath.c_str(), O_RDONLY);

   if(netNsFd >= 0)
   {
      DebugLog(<< "Opened netns: " << nameSpacePath << " fd: " << Data((int) netNsFd));
   }

   return(netNsFd);
}

bool NetNs::netNsExists(const Data& netNs)
{
   int netNsFd = openNetNs(netNs);
   if(netNsFd >= 0 )
   {
      DebugLog(<< "Closing netns fd: " << Data((int) netNsFd));
      close(netNsFd);
   }

   return(netNsFd >= 0);
}

int NetNs::getNetNsId(const Data& netNsName, bool shouldAdd)
{
   initDictionary();

   int netNsId = -1;

   HashMap<Data, int>::iterator netNsIterator = sNetNsDictionaryToId.find(netNsName);
   if(netNsIterator != sNetNsDictionaryToId.end())
   {
      netNsId = netNsIterator->second;
   }
   else if(shouldAdd)
   {
      netNsId = netNsName.caseInsensitiveTokenHash();
      int originalId = netNsId;
      // If hash is not unique, find the next available id
      while(sIdDictionaryToNetNs.find(netNsId) != sIdDictionaryToNetNs.end())
      {
         netNsId++;
      }

      // This means another netns name had the same hash (i.e. the hash is
      // not unique.  So there is a likelyhood that in another instance
      // of resip::SipStack (e.g. configured for replicating this instance)
      // we could get a different id for this netns name.
      // That could cause problems if the Tuple binary token was generated on
      // on instance of resip:SipStack for the Record-Route and another instance
      // of resip::SipStack recieved that Route as its Route to pop off.
      if(originalId != netNsId)
      {
         WarningLog(<< "Non-unique netns hashes.  " << netNsName << ": " << netNsId 
               << sIdDictionaryToNetNs[originalId] << ": " << originalId);
      }

      DebugLog(<< "Adding netns to dictionary: " << netNsName << ": " << netNsId);
      sIdDictionaryToNetNs[netNsId] = netNsName;
      sNetNsDictionaryToId[netNsName] = netNsId;
   }

   return(netNsId);
}

const Data& NetNs::getNetNsName(int netNsId)
{
   initDictionary();

   HashMap<int, Data>::iterator idIterator = sIdDictionaryToNetNs.find(netNsId);
   if(idIterator == sIdDictionaryToNetNs.end())
   {
      throw Exception("netns id: " + Data(netNsId) + " does not exist",
                  __FILE__,__LINE__);
   }
   return(idIterator->second);
}

void NetNs::initDictionary()
{
   // Make sure there is an entry in the dictionary for the
   // global namespace "" (empty string)
   if(sIdDictionaryToNetNs.size() == 0 &&
      sNetNsDictionaryToId.size() == 0)
   {
      Data defaultNetNs("");
      sIdDictionaryToNetNs[defaultNetNs.caseInsensitiveTokenHash()] = defaultNetNs;
      sNetNsDictionaryToId[""] = 0;

      // No locking, so test no one else touched 
      // the dictionary
      resip_assert(sIdDictionaryToNetNs.size() == 1);
      resip_assert(sNetNsDictionaryToId.size() == 1);
   }
}

#endif

/* ====================================================================
 *
 * Copyright (c) Daniel Petrie, SIPez LLC.  All rights reserved.
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
 * vi: set shiftwidth=3 expandtab:
 */
