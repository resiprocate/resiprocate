#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdexcept>

#include "rutil/hep/ResipHep.hxx"
#include "rutil/hep/HepAgent.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

HepAgent::HepAgent(const Data &captureHost, int capturePort, int captureAgentID)
   : mCaptureHost(captureHost), mCapturePort(capturePort), mCaptureAgentID(captureAgentID)
{
#ifdef USE_IPV6
   struct sockaddr_in6 myaddr;
   memset(&myaddr, 0, sizeof(myaddr));
   myaddr.sin6_family=AF_INET6;
   myaddr.sin6_addr = in6addr_any;   // FIXME - make it configurable
   myaddr.sin6_port=0;   // FIXME - make it configurable

   mSocket = ::socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

   int no = 0;
#if !defined(WIN32)
   ::setsockopt(mSocket, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no));
#else
   ::setsockopt(mSocket, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&no, sizeof(no));
#endif

#else
   struct sockaddr_in myaddr;
   memset(&myaddr, 0, sizeof(myaddr));
   myaddr.sin_family=AF_INET;
   myaddr.sin_addr.s_addr=INADDR_ANY;   // FIXME - make it configurable
   myaddr.sin_port=0;   // FIXME - make it configurable

   mSocket = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
#endif

   if(mSocket < 0)
   {
      ErrLog(<<"Failed to create socket");
      throw std::runtime_error("Failed to create socket");
   }

   // make it non-blocking
   // FIXME - HEP messages are lost if the transmit buffer fills up
   // Messages should be queued and sent as part of the main loop
   if(!makeSocketNonBlocking(mSocket))
   {
      ErrLog(<<"Failed to set O_NONBLOCK");
      throw std::runtime_error("Failed to set O_NONBLOCK");
   }

   if(::bind(mSocket, ( struct sockaddr *) &myaddr, sizeof(myaddr)) < 0) {
      ErrLog(<<"bind failed");
      throw std::runtime_error("bind failed");
   }

   // create destination Tuple
   struct addrinfo *rset;
   if(getaddrinfo(mCaptureHost.c_str(), 0, 0, &rset) != 0)
   {
      ErrLog(<<"getaddrinfo failed");
      throw std::runtime_error("getaddrinfo failed");
   }
   if(rset == 0)
   {
      ErrLog(<<"no results from getaddrinfo");
      throw std::runtime_error("no results from getaddrinfo");
   }
   const struct addrinfo& dest = rset[0];
   switch(dest.ai_family)
   {
      case AF_INET:
         memcpy(&mDestination.v4Address, dest.ai_addr, dest.ai_addrlen);
         mDestination.v4Address.sin_port = htons(mCapturePort);
         break;
#ifdef IPPROTO_IPV6
      case AF_INET6:
         memcpy(&mDestination.v6Address, dest.ai_addr, dest.ai_addrlen);
         mDestination.v6Address.sin6_port = htons(mCapturePort);
         break;
#endif
      default:
         ErrLog(<<"unsupported address family");
         throw std::runtime_error("unsupported address family");
   }
   freeaddrinfo(rset);
   InfoLog(<<"HEP capture agent ready to send to " << mDestination);
}

HepAgent::~HepAgent()
{
}

/* ====================================================================
 *
 * Copyright 2016 Daniel Pocock http://danielpocock.com  All rights reserved.
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
 *
 */
