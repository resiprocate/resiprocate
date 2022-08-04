#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdexcept>
#include <utility>

#include "rutil/rtcp/re_rtp.h"

#include "rutil/hep/ResipHep.hxx"
#include "rutil/hep/HepAgent.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::EEP

HepAgent::HepAgent(const Data &captureHost, int capturePort, int captureAgentID)
   : mCaptureHost(captureHost), mCapturePort(capturePort), mCaptureAgentID(captureAgentID)
{
   bool initV4Socket = true;
   struct sockaddr myaddr;
   memset(&myaddr, 0, sizeof(myaddr));
   int sockaddrlen;
#ifdef USE_IPV6
   if (!DnsUtil::isIpV4Address(captureHost))
   {
      initV4Socket = false;

      struct sockaddr_in6* myaddrv6 = (struct sockaddr_in6*)&myaddr;
      sockaddrlen = sizeof(struct sockaddr_in6);
      myaddrv6->sin6_family = AF_INET6;
      myaddrv6->sin6_addr = in6addr_any;   // FIXME - make it configurable
      myaddrv6->sin6_port = 0;   // FIXME - make it configurable
      mSocket = ::socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

      int no = 0;
#if !defined(WIN32)
      ::setsockopt(mSocket, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no));
#else
      // Note:  On linux this allows the IPV6 socket to send to V4 destination, however this doesn't 
      //        appear to work on windows.  The sendTo call in sendToHOMER ends up return 10014 
      //        (The system detected an invalid pointer address in attempting to use a pointer argument in a call.)
      //        The above call to isIpV4Address was added to address this.
      ::setsockopt(mSocket, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&no, sizeof(no));
#endif
   }

#endif // USE_IPV6

   if (initV4Socket)
   {
      struct sockaddr_in* myaddrv4 = (struct sockaddr_in*)&myaddr;
      sockaddrlen = sizeof(struct sockaddr_in);
      myaddrv4->sin_family = AF_INET;
      myaddrv4->sin_addr.s_addr = INADDR_ANY;   // FIXME - make it configurable
      myaddrv4->sin_port = 0;   // FIXME - make it configurable

      mSocket = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
   }

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

   if(::bind(mSocket, &myaddr, sockaddrlen) < 0) 
   {
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

inline int32_t
ntoh_cpl(const void *x)
{
   unsigned char c[4];
   uint32_t *v = reinterpret_cast<uint32_t *>(c);

   memcpy(c, x, 4);

   // replace the fraction lost (8 bits) by sign of the 24 bit value
   c[0] = (c[1] & 0x80) ? 0xff : 0;

   return (int32_t)(ntohl(*v));
}

void
HepAgent::sendRTCP(const TransportType type, const GenericIPAddress& source, const GenericIPAddress& destination, const Data& rtcpRaw, const Data& correlationId)
{
   const struct rtcp_msg* msg = reinterpret_cast<const struct rtcp_msg*>(rtcpRaw.data());

   Data json;
   DataStream stream(json);

   StackLog(<<"RTCP packet type: " << msg->hdr.pt << " len " << (ntohs(msg->hdr.length)*2) << " bytes");
  
   stream << "{";

   switch (msg->hdr.pt)
   {
      case RTCP_SR:
         stream << "\"sender_information\":{"
                << "\"ntp_timestamp_sec\":" << ntohl(msg->r.sr.ntp_sec) << ","
                << "\"ntp_timestamp_usec\":" << ntohl(msg->r.sr.ntp_frac) << ","
                << "\"octets\":" << ntohl(msg->r.sr.osent) << ","
                << "\"rtp_timestamp\":" << ntohl(msg->r.sr.rtp_ts) << ","
                << "\"packets\":" << ntohl(msg->r.sr.psent)
                << "},";
         if(msg->hdr.count > 0)
         {
            const struct rtcp_rr *rr = reinterpret_cast<const struct rtcp_rr*>(&msg->r.sr.rrv);
            stream << "\"ssrc\":" << ntohl(msg->r.sr.ssrc) << ","
                   << "\"type\":" << msg->hdr.pt << ","
                   << "\"report_blocks\":["
                   << "{"
                      << "\"source_ssrc\":" << ntohl(rr->ssrc) << ","
                      << "\"highest_seq_no\":" << ntohl(rr->last_seq) << ","
                      << "\"fraction_lost\":" << +(reinterpret_cast<const uint8_t *>(&rr->fraction_lost_32))[0] << ","  // 8 bits
                      << "\"ia_jitter\":" << ntohl(rr->jitter) << ","
                      << "\"packets_lost\":" << ntoh_cpl(&rr->fraction_lost_32) << ","
                      << "\"lsr\":" << ntohl(rr->lsr) << ","
                      << "\"dlsr\":" << ntohl(rr->dlsr)
                   << "}"
                   << "],\"report_count\":1";
         }
         break;

      case RTCP_RR:
         if(msg->hdr.count > 0)
         {
            const struct rtcp_rr *rr = reinterpret_cast<const struct rtcp_rr*>(&msg->r.rr.rrv);
            stream << "\"ssrc\":" << ntohl(msg->r.rr.ssrc) << ","
                   << "\"type\":" << msg->hdr.pt << ","
                   << "\"report_blocks\":["
                   << "{"
                      << "\"source_ssrc\":" << ntohl(rr->ssrc) << ","
                      << "\"highest_seq_no\":" << ntohl(rr->last_seq) << ","
                      << "\"fraction_lost\":" << +(reinterpret_cast<const uint8_t *>(&rr->fraction_lost_32))[0] << "," // 8 bits
                      << "\"ia_jitter\":" << ntohl(rr->jitter) << ","
                      << "\"packets_lost\":" << ntoh_cpl(&rr->fraction_lost_32) << ","
                      << "\"lsr\":" << ntohl(rr->lsr) << ","
                      << "\"dlsr\":" << ntohl(rr->dlsr)
                   << "}"
                   << "],\"report_count\":1";
         }
         break;

      default:
         DebugLog(<<"unhandled RTCP packet type: " << msg->hdr.pt);
   }

   stream << "}";
   stream.flush();
   StackLog(<<"constructed RTCP JSON: " << json);

   sendToHOMER<Data>(resip::UDP,
      source, destination,
      HepAgent::RTCP_JSON, json,
      correlationId);
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
