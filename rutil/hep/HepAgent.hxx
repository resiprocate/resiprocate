#if !defined(RESIP_HEPAGENT_HXX)
#define RESIP_HEPAGENT_HXX

#include "rutil/Data.hxx"
#include "rutil/GenericIPAddress.hxx"
#include "rutil/Socket.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/hep/ResipHep.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

namespace resip
{

class HepAgent
{
   public:
      typedef enum
      {
         SIP = 1,
         RTCP_JSON = 5
      } HEPEventType;

      HepAgent(const Data &captureHost, int capturePort, int captureAgentID);
      virtual ~HepAgent();
      template <class T>
      void sendToHOMER(const TransportType type, const GenericIPAddress& source, const GenericIPAddress& destination, const HEPEventType eventType, const T& msg, const Data& correlationId)
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
         unsigned int sourcePort = 0;
         unsigned int destinationPort = 0;
         switch(source.address.sa_family)
         {
            case AF_INET:
            {
               hg->ip_family.data = AF_INET;
               src_ip4.chunk.vendor_id = htons(0x0000);
               src_ip4.chunk.type_id   = htons(0x0003);
               const struct sockaddr_in& src_sa = source.v4Address;
               memcpy(&src_ip4.data, &src_sa.sin_addr.s_addr, sizeof(src_sa.sin_addr.s_addr));
               src_ip4.chunk.length = htons(sizeof(src_ip4));
               chunk = Data(Data::Borrow, (char *)&src_ip4, sizeof(hep_chunk_ip4_t));
               stream << chunk;
               sourcePort = ntohs(src_sa.sin_port);

               dst_ip4.chunk.vendor_id = htons(0x0000);
               dst_ip4.chunk.type_id   = htons(0x0004);
               const struct sockaddr_in& dst_sa = destination.v4Address;
               memcpy(&dst_ip4.data, &dst_sa.sin_addr.s_addr, sizeof(dst_sa.sin_addr.s_addr));
               dst_ip4.chunk.length = htons(sizeof(dst_ip4));
               chunk = Data(Data::Borrow, (char *)&dst_ip4, sizeof(hep_chunk_ip4_t));
               stream << chunk;
               destinationPort = ntohs(dst_sa.sin_port);

               break;
            }
#ifdef USE_IPV6
            case AF_INET6:
            {
               hg->ip_family.data = AF_INET6;
               src_ip6.chunk.vendor_id = htons(0x0000);
               src_ip6.chunk.type_id   = htons(0x0005);
               const struct sockaddr_in6& src_sa6 = source.v6Address;
               memcpy(&src_ip6.data, &src_sa6.sin6_addr, sizeof(src_sa6.sin6_addr));
               src_ip6.chunk.length = htons(sizeof(src_ip6));
               chunk = Data(Data::Borrow, (char *)&src_ip6, sizeof(hep_chunk_ip6_t));
               stream << chunk;
               sourcePort = ntohs(src_sa6.sin6_port);

               dst_ip6.chunk.vendor_id = htons(0x0000);
               dst_ip6.chunk.type_id   = htons(0x0006);
               const struct sockaddr_in6& dst_sa6 = destination.v6Address;
               memcpy(&dst_ip6.data, &dst_sa6.sin6_addr, sizeof(dst_sa6.sin6_addr));
               dst_ip6.chunk.length = htons(sizeof(dst_ip6));
               chunk = Data(Data::Borrow, (char *)&dst_ip6, sizeof(hep_chunk_ip6_t));
               stream << chunk;
               destinationPort = ntohs(dst_sa6.sin6_port);
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
         switch(type)
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
#if !defined(WIN32) || (defined(WIN32) && (_WIN32_WINNT >= 0x0600))
            case SCTP:
               hg->ip_proto.data = IPPROTO_SCTP;
               break;
#endif
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
         hg->src_port.data = htons(sourcePort);
         hg->src_port.chunk.length = htons(sizeof(hg->src_port));

         /* DST PORT */
         hg->dst_port.chunk.vendor_id = htons(0x0000);
         hg->dst_port.chunk.type_id   = htons(0x0008);
         hg->dst_port.data = htons(destinationPort);
         hg->dst_port.chunk.length = htons(sizeof(hg->dst_port));

         UInt64 now = hepUnixTimestamp();

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
         hg->proto_t.data = eventType;
         hg->proto_t.chunk.length = htons(sizeof(hg->proto_t));

         /* Capture ID */
         hg->capt_id.chunk.vendor_id = htons(0x0000);
         hg->capt_id.chunk.type_id   = htons(0x000c);
         hg->capt_id.data = htons(mCaptureAgentID);
         hg->capt_id.chunk.length = htons(sizeof(hg->capt_id));

         stream.flush();

         /* Correlation ID */
         if(!correlationId.empty())
         {
            StackLog(<<"adding correlation ID: " << correlationId);
            hep_chunk_t correlation_chunk;
            correlation_chunk.vendor_id = htons(0x0000);
            correlation_chunk.type_id   = htons(0x0011);
            correlation_chunk.length    = htons(sizeof(correlation_chunk) + correlationId.size());
            chunk = Data(Data::Borrow, (char *)&correlation_chunk, sizeof(correlation_chunk));
            stream << chunk;
            stream << correlationId;
            stream.flush();
         }

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

         if(sendto(mSocket, buf.data(), buf.size(), 0, &mDestination.address, mDestination.length()) < 0)
         {
            int e = getErrno();
#if defined(WIN32)
            ErrLog(<< "sending to HOMER " << mDestination << " failed (" << e << ")");
#else
            ErrLog(<< "sending to HOMER " << mDestination << " failed (" << e << "): " << strerror(e));
#endif
         }
         else
         {
            DebugLog(<< "packet sent to HOMER " << mDestination);
         }
      }

   private:
      Data mCaptureHost;
      int mCapturePort;
      int mCaptureAgentID;
      GenericIPAddress mDestination;
      Socket mSocket;
};


}

#undef RESIPROCATE_SUBSYSTEM

#endif


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
