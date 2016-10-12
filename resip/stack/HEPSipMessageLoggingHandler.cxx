#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdexcept>

#include "resip/stack/core_hep.h"
#include "resip/stack/HEPSipMessageLoggingHandler.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::NONE

using namespace resip;
using namespace std;

HEPSipMessageLoggingHandler::HEPSipMessageLoggingHandler(Data &captureHost, int capturePort, int captureAgentID)
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
   setsockopt(mSocket, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&no, sizeof(no));

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
   mTuple = Tuple(*(rset[0].ai_addr), UDP);
   mTuple.setPort(mCapturePort);
   freeaddrinfo(rset);
   InfoLog(<<"HEP capture agent ready to send to " << mTuple);
}

HEPSipMessageLoggingHandler::~HEPSipMessageLoggingHandler()
{
}

void
HEPSipMessageLoggingHandler::outboundMessage(const Tuple &source, const Tuple &destination, const SipMessage &msg)
{
   sendToHOMER(source, destination, msg);
}

void
HEPSipMessageLoggingHandler::outboundRetransmit(const Tuple &source, const Tuple &destination, const SendData &data)
{
   // FIXME - implement for SendData
}

void
HEPSipMessageLoggingHandler::inboundMessage(const Tuple& source, const Tuple& destination, const SipMessage &msg)
{
   sendToHOMER(source, destination, msg);
}

void
HEPSipMessageLoggingHandler::sendToHOMER(const Tuple& source, const Tuple& destination, const SipMessage &msg)
{
   struct hep_generic *hg;
   hep_chunk_ip4_t src_ip4, dst_ip4;
   hep_chunk_t payload_chunk;

#ifdef USE_IPV6
   hep_chunk_ip6_t src_ip6, dst_ip6;
#endif

   char *data = new char[sizeof(struct hep_generic)];
   Data buf(Data::Take, data, sizeof(struct hep_generic));
   hg = (struct hep_generic *)buf.data();
   DebugLog(<< "buf.size() == " << buf.size());
   DataStream stream(buf);

   memset(hg, 0, sizeof(struct hep_generic));

   /* header set */
   memcpy(hg->header.id, "\x48\x45\x50\x33", 4);

   Data chunk;

   /* IP proto */
   hg->ip_family.chunk.vendor_id = htons(0x0000);
   hg->ip_family.chunk.type_id   = htons(0x0001);
   hg->ip_family.chunk.length = htons(sizeof(hg->ip_family));
   switch(source.ipVersion())
   {
      case V4:
      {
         hg->ip_family.data = AF_INET;
         src_ip4.chunk.vendor_id = htons(0x0000);
         src_ip4.chunk.type_id   = htons(0x0003);
         struct sockaddr_in *sa;
         sa = (struct sockaddr_in *)&source.getSockaddr();
         memcpy(&src_ip4.data, &sa->sin_addr.s_addr, sizeof(sa->sin_addr.s_addr));
         src_ip4.chunk.length = htons(sizeof(src_ip4));
         chunk = Data(Data::Borrow, (char *)&src_ip4, sizeof(hep_chunk_ip4_t));
         stream << chunk;

         dst_ip4.chunk.vendor_id = htons(0x0000);
         dst_ip4.chunk.type_id   = htons(0x0004);
         sa = (struct sockaddr_in *)&destination.getSockaddr();
         memcpy(&dst_ip4.data, &sa->sin_addr.s_addr, sizeof(sa->sin_addr.s_addr));
         dst_ip4.chunk.length = htons(sizeof(dst_ip4));
         chunk = Data(Data::Borrow, (char *)&dst_ip4, sizeof(hep_chunk_ip4_t));
         stream << chunk;

         break;
      }
#ifdef USE_IPV6
      case V6:
      {
         hg->ip_family.data = AF_INET6;
         src_ip6.chunk.vendor_id = htons(0x0000);
         src_ip6.chunk.type_id   = htons(0x0005);
         struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)&source.getSockaddr();
         memcpy(&src_ip6.data, &sa6->sin6_addr, sizeof(sa6->sin6_addr));
         src_ip6.chunk.length = htons(sizeof(src_ip6));
         chunk = Data(Data::Borrow, (char *)&src_ip6, sizeof(hep_chunk_ip6_t));
         stream << chunk;

         dst_ip6.chunk.vendor_id = htons(0x0000);
         dst_ip6.chunk.type_id   = htons(0x0006);
         sa6 = (struct sockaddr_in6 *)&destination.getSockaddr();
         memcpy(&dst_ip6.data, &sa6->sin6_addr, sizeof(sa6->sin6_addr));
         dst_ip6.chunk.length = htons(sizeof(dst_ip6));
         chunk = Data(Data::Borrow, (char *)&dst_ip6, sizeof(hep_chunk_ip6_t));
         stream << chunk;
         break;
      }
#endif
      {
      default:
         ErrLog(<<"unhandled address family");
         return;
      }
   }
   stream.flush();
   DebugLog(<< "buf.size() == " << buf.size());
   hg = (struct hep_generic *)buf.data();

   /* PROTOCOL */
   switch(source.getType())
   {
      case TLS:
         hg->ip_proto.data = IPPROTO_IDP; // FIXME
         break;
      case TCP:
         hg->ip_proto.data = IPPROTO_TCP;
         break;
      case UDP:
         hg->ip_proto.data = IPPROTO_UDP;
         break;
      case SCTP:
         hg->ip_proto.data = IPPROTO_SCTP;
         break;
      case WS:
      case WSS:
         hg->ip_proto.data = IPPROTO_TCP; // FIXME
         break;
      default:
         ErrLog(<<"unhandled TransportType");
         return;
   }
   /* Proto ID */
   hg->ip_proto.chunk.vendor_id = htons(0x0000);
   hg->ip_proto.chunk.type_id   = htons(0x0002);
   hg->ip_proto.chunk.length = htons(sizeof(hg->ip_proto));

   /* SRC PORT */
   hg->src_port.chunk.vendor_id = htons(0x0000);
   hg->src_port.chunk.type_id   = htons(0x0007);
   hg->src_port.data = htons(source.getPort());
   hg->src_port.chunk.length = htons(sizeof(hg->src_port));

   /* DST PORT */
   hg->dst_port.chunk.vendor_id = htons(0x0000);
   hg->dst_port.chunk.type_id   = htons(0x0008);
   hg->dst_port.data = htons(destination.getPort());
   hg->dst_port.chunk.length = htons(sizeof(hg->dst_port));

   UInt64 now = ResipClock::getTimeMicroSec();

   /* TIMESTAMP SEC */
   hg->time_sec.chunk.vendor_id = htons(0x0000);
   hg->time_sec.chunk.type_id   = htons(0x0009);
   hg->time_sec.chunk.length = htons(sizeof(hg->time_sec));
   hg->time_sec.data = htonl(now / 1000000LL);

   /* TIMESTAMP USEC */
   hg->time_usec.chunk.vendor_id = htons(0x0000);
   hg->time_usec.chunk.type_id   = htons(0x000a);
   hg->time_usec.data = htonl(now % 1000000LL);
   hg->time_usec.chunk.length = htons(sizeof(hg->time_usec));

   /* Protocol TYPE */
   hg->proto_t.chunk.vendor_id = htons(0x0000);
   hg->proto_t.chunk.type_id   = htons(0x000b);
   hg->proto_t.data = 0x001; //SIP
   hg->proto_t.chunk.length = htons(sizeof(hg->proto_t));

   /* Capture ID */
   hg->capt_id.chunk.vendor_id = htons(0x0000);
   hg->capt_id.chunk.type_id   = htons(0x000c);
   hg->capt_id.data = htons(mCaptureAgentID);
   hg->capt_id.chunk.length = htons(sizeof(hg->capt_id));

   stream.flush();

   Data::size_type payloadChunkOffset = buf.size();
   payload_chunk.vendor_id = htons(0x0000);
   payload_chunk.type_id   = htons(0x000f);
   chunk = Data(Data::Borrow, (char *)&payload_chunk, sizeof(payload_chunk));
   stream << chunk;
   stream.flush();
   Data::size_type beforePayload = buf.size();
   DebugLog(<< "buf.size() == " << buf.size());
   stream << msg;
   stream.flush();
   Data::size_type afterPayload = buf.size();
   DebugLog(<< "Final buf.size() == " << buf.size());
   hep_chunk_t *_payload_chunk = (hep_chunk_t *)(buf.data() + payloadChunkOffset);
   _payload_chunk->length = htons(sizeof(payload_chunk) + afterPayload - beforePayload);
   hg = (struct hep_generic *)buf.data();
   hg->header.length = htons(afterPayload);

   const sockaddr& addr = mTuple.getSockaddr();
   if(sendto(mSocket, buf.data(), buf.size(), 0, &addr, mTuple.length()) < 0)
   {
      ErrLog(<<"sending to HOMER " << mTuple << " failed: " << strerror(errno));
   }
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
