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
   struct sockaddr_storage myaddr;
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

   if(::bind(mSocket, (struct sockaddr*)&myaddr, sockaddrlen) < 0)
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

// Reads a 32-bit field in network byte order from any offset in the packet
static uint32_t
readUInt32(const uint8_t* p)
{
   uint32_t v;
   memcpy(&v, p, sizeof(v));
   return ntohl(v);
}

// Offsets and sizes on the wire (RFC 3550 section 6.4).  The packet is read
// from these rather than through struct rtcp_msg, whose layout only holds a
// decoded packet and doesn't match the wire on every compiler (MSVC pads its
// header to 8 bytes after the bit-fields).
static const size_t RTCP_SSRC_OFFSET = 4;
static const size_t RTCP_SR_REPORTS_OFFSET = 28;   // header, SSRC, sender info
static const size_t RTCP_RR_REPORTS_OFFSET = 8;    // header, SSRC
static const size_t RTCP_REPORT_BLOCK_SIZE = 24;

void
encodeRRvec(DataStream& stream, const uint8_t *rr_vec, int rrCount)
{
   stream << "\"report_blocks\":[";

   for(int i = 0 ; i < rrCount; i++)
   {
      const uint8_t *rr = rr_vec + i * RTCP_REPORT_BLOCK_SIZE;
      stream << ((i>0) ? "," : "")
             << "{"
             << "\"source_ssrc\":" << readUInt32(rr) << ","
             << "\"highest_seq_no\":" << readUInt32(rr + 8) << ","
             << "\"fraction_lost\":" << +rr[4] << ","  // 8 bits
             << "\"ia_jitter\":" << readUInt32(rr + 12) << ","
             << "\"packets_lost\":" << ntoh_cpl(rr + 4) << ","
             << "\"lsr\":" << readUInt32(rr + 16) << ","
             << "\"dlsr\":" << readUInt32(rr + 20)
             << "}";
   }

   stream << "],\"report_count\":" << rrCount;
}

Data
HepAgent::convertRTCPtoJSON(const Data& rtcpRaw)
{
   const uint8_t* raw = reinterpret_cast<const uint8_t*>(rtcpRaw.data());

   StackLog(<< "buffer size: " << rtcpRaw.size());

   if(rtcpRaw.size() < 8)
   {
      ErrLog(<<"too small to be a valid RTCP packet");
      return "";
   }

   // FIXME - check for multiple RTCP packets in a single UDP datagram
   //       - does HOMER require these to be submitted as separate HEP messages?

   Data json;
   DataStream stream(json);

   // we can't trust the order of bit-fields in a struct
   // on all compilers and platforms to match the order
   // on the wire
   uint8_t byte0 = raw[0];
   unsigned int version = (byte0 & 0xc0) >> 6;
   unsigned int rrCount = (byte0 & 0x1f);
   bool p = (byte0 & 0x20) >> 5;
   unsigned int pt = raw[1];
   unsigned int length = (raw[2] << 8) | raw[3];

   StackLog(<<"RTCP version: " << version << " p " << p
      << " RR count: " << rrCount
      << " packet type: " << pt
      << " length: " << (length*2) << " bytes");

   if(version != 2)
   {
      ErrLog(<<"unsupported RTCP version: " << version);
      return "";
   }
  
   // The report blocks follow the SSRC in an RR, and the SSRC and 20 bytes of
   // sender info in an SR.  The header gives their number, but they all have
   // to be in the packet.
   if(pt == RTCP_SR || pt == RTCP_RR)
   {
      const size_t reportsOffset = (pt == RTCP_SR) ? RTCP_SR_REPORTS_OFFSET : RTCP_RR_REPORTS_OFFSET;
      if(rtcpRaw.size() < reportsOffset + rrCount * RTCP_REPORT_BLOCK_SIZE)
      {
         ErrLog(<<"RTCP packet too small for its " << rrCount << " report blocks: " << rtcpRaw.size() << " bytes");
         return "";
      }
   }

   stream << "{" << "\"type\":" << pt << ",";

   switch (pt)
   {
      case RTCP_SR:
         // the sender info follows the SSRC: NTP timestamp (seconds and
         // fraction), RTP timestamp, packet count, octet count
         stream << "\"sender_information\":{"
                << "\"ntp_timestamp_sec\":" << readUInt32(raw + 8) << ","
                << "\"ntp_timestamp_usec\":" << readUInt32(raw + 12) << ","
                << "\"octets\":" << readUInt32(raw + 24) << ","
                << "\"rtp_timestamp\":" << readUInt32(raw + 16) << ","
                << "\"packets\":" << readUInt32(raw + 20)
                << "},"
                << "\"ssrc\":" << readUInt32(raw + RTCP_SSRC_OFFSET) << ",";
         encodeRRvec(stream, raw + RTCP_SR_REPORTS_OFFSET, rrCount);
         break;

      case RTCP_RR:
         stream << "\"ssrc\":" << readUInt32(raw + RTCP_SSRC_OFFSET) << ",";
         encodeRRvec(stream, raw + RTCP_RR_REPORTS_OFFSET, rrCount);
         break;

      default:
         WarningLog(<<"unhandled RTCP packet type: " << pt);
   }

   stream << "}";
   stream.flush();

   return json;
}

void
HepAgent::sendRTCP(const TransportType type, const GenericIPAddress& source, const GenericIPAddress& destination, const Data& rtcpRaw, const Data& correlationId)
{
   Data json(convertRTCPtoJSON(rtcpRaw));
   if(json.empty())
   {
      // convertRTCPtoJSON has already logged why
      return;
   }

   StackLog(<< "source: " << source
      << " destination: " << destination
      << " constructed RTCP JSON: " << json);

   sendToHOMER<Data>(resip::UDP,
      source, destination,
      HepAgent::RTCP_JSON, json,
      correlationId);
}

bool
HepAgent::sendToWire(const Data& buf) const
{
   return sendto(mSocket, buf.data(), (int)buf.size(), 0, &mDestination.address, (int)mDestination.length()) < 0;
}

/* ====================================================================
 *
 * Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 * Copyright (c) 2021-2022, Daniel Pocock https://danielpocock.com
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
