
#include <rutil/Logger.hxx>
#include <rutil/ResipAssert.h>
#include <rutil/Subsystem.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/Socket.hxx>
#include "RTPPortManager.hxx"

#include <cstring>

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::MEDIA

// We are only handing out even RTP ports; the next higher odd port is used for RTCP
RTPPortManager::RTPPortManager(int portRangeMin, int portRangeMax)
 : mPortRangeMin(portRangeMin),
   mPortRangeMax(portRangeMax),
   mTotalPortPairs(0) // set below
{
   // Ensure we start on an even port
   if(mPortRangeMin & 0x1)
   {
      mPortRangeMin++;
   }
   // Ensure we end on an even port, even though mPortMax+1 
   // is considered valid for RTCP stream of mPortMax port
   if(mPortRangeMax & 0x1)
   {
      mPortRangeMax--;
   }
   resip_assert(mPortRangeMax >= mPortRangeMin);
   for(unsigned int i = mPortRangeMin; i <= mPortRangeMax;)
   {
      mRTPPortFreeList.push_back(i);
      i=i+2;  // only add even ports
   }
   mTotalPortPairs = (unsigned int)mRTPPortFreeList.size();
   StackLog(<< "RTPPortManager: min even port=" << mPortRangeMin << ", max even port=" << mPortRangeMax);
}

unsigned int
RTPPortManager::allocateRTPPort()
{
   unsigned int port = 0;
   if(!mRTPPortFreeList.empty())
   {
      port = mRTPPortFreeList.front();
      mRTPPortFreeList.pop_front();
   }
   StackLog(<<"allocateRTPPort: port == " << port);
   return port;
}

// Whether a UDP socket can be bound to address and port, without sharing it with another socket:
// on Windows, SO_EXCLUSIVEADDRUSE, so it's not given a port another socket has bound with
// SO_REUSEADDR; elsewhere, no SO_REUSEADDR.  Only the port being in use (or reserved: access
// denied) makes it false.  If address isn't an IP address, no socket can be created, or the bind
// fails for another reason - eg. address isn't one of this host's - there's nothing to say about
// the port: true, and the media stream's socket reports the error.
static bool
canBindUdp(const Data& address, unsigned int port)
{
   sockaddr_storage storage;
   memset(&storage, 0, sizeof(storage));
   socklen_t length = 0;
   if(DnsUtil::isIpV4Address(address))
   {
      sockaddr_in& v4 = reinterpret_cast<sockaddr_in&>(storage);
      v4.sin_family = AF_INET;
      v4.sin_port = htons((unsigned short)port);
      DnsUtil::inet_pton(address, v4.sin_addr);
      length = sizeof(v4);
   }
#ifdef USE_IPV6
   else if(DnsUtil::isIpV6Address(address))
   {
      sockaddr_in6& v6 = reinterpret_cast<sockaddr_in6&>(storage);
      v6.sin6_family = AF_INET6;
      v6.sin6_port = htons((unsigned short)port);
      DnsUtil::inet_pton(address, v6.sin6_addr);
      length = sizeof(v6);
   }
#endif
   else
   {
      return true;
   }

   Socket fd = ::socket(storage.ss_family, SOCK_DGRAM, IPPROTO_UDP);
   if(fd == INVALID_SOCKET)
   {
      return true;
   }
#ifdef _WIN32
   BOOL on = TRUE;
   ::setsockopt(fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&on, sizeof(on));
#elif defined(USE_IPV6) && defined(__linux__)
   if(storage.ss_family == AF_INET6)
   {
      int on = 1;
      ::setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on));
   }
#endif
   bool usable = true;
   if(::bind(fd, reinterpret_cast<sockaddr*>(&storage), length) != 0)
   {
      const int e = getErrno();
#ifdef _WIN32
      usable = !(e == EADDRINUSE || e == WSAEACCES);
#else
      usable = !(e == EADDRINUSE || e == EACCES);
#endif
   }
   closeSocket(fd);
   return usable;
}

unsigned int
RTPPortManager::allocateRTPPort(const Data& address)
{
   for(size_t tries = mRTPPortFreeList.size(); tries > 0; --tries)  // each free port once
   {
      const unsigned int port = allocateRTPPort();
      if(canBindUdp(address, port) && canBindUdp(address, port + 1))
      {
         return port;
      }
      InfoLog(<< "allocateRTPPort: RTP port " << port << ", or the RTCP port after it, can't be bound on " << address
              << " (eg. another socket has it): trying the next port");
      mRTPPortFreeList.push_back(port);
   }
   WarningLog(<< "allocateRTPPort: no free RTP port can be bound on " << address);
   return 0;
}

void
RTPPortManager::freeRTPPort(unsigned int port)
{
   StackLog(<<"freeRTPPort: port == " << port);
   resip_assert(port >= mPortRangeMin && port <= mPortRangeMax);
   resip_assert((port & 0x1) == 0);
   mRTPPortFreeList.push_back(port);
}


/* ====================================================================

 Copyright (c) 2010-2026, SIP Spectrum, Inc. http://www.sipspectrum.com
 Copyright (c) 2014 Daniel Pocock http://danielpocock.com
 Copyright (c) 2007-2008, Plantronics, Inc.
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
