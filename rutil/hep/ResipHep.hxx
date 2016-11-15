#if !defined(RESIP_HEP_HXX)
#define RESIP_HEP_HXX

#if defined(_WIN32)
#include "rutil/msvc/stdint.h"
typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#include <netinet/in.h>
#ifdef USE_IPV6
#include <netinet/ip6.h>
#endif /* USE_IPV6 */
#endif

#include "rutil/compat.hxx"

#ifdef _MSC_VER
#define PACKED_STRUCT( __StructDeclaration__ ) __pragma( pack(push, 1) ) __StructDeclaration__ __pragma( pack(pop) )
#elif defined(__GNUC__)
#define PACKED_STRUCT( __StructDeclaration__ ) __StructDeclaration__ __attribute__((__packed__))
#else
#define PACKED_STRUCT( __StructDeclaration__ ) __StructDeclaration__
#endif

/* HEPv3 types */

PACKED_STRUCT(struct hep_chunk{
   u_int16_t vendor_id;
   u_int16_t type_id;
   u_int16_t length;
});

typedef struct hep_chunk hep_chunk_t;

PACKED_STRUCT(struct hep_chunk_uint8 {
   hep_chunk_t chunk;
   u_int8_t data;
});

typedef struct hep_chunk_uint8 hep_chunk_uint8_t;

PACKED_STRUCT(struct hep_chunk_uint16 {
   hep_chunk_t chunk;
   u_int16_t data;
});

typedef struct hep_chunk_uint16 hep_chunk_uint16_t;

PACKED_STRUCT(struct hep_chunk_uint32 {
   hep_chunk_t chunk;
   u_int32_t data;
});

typedef struct hep_chunk_uint32 hep_chunk_uint32_t;

PACKED_STRUCT(struct hep_chunk_str {
   hep_chunk_t chunk;
   char *data;
});

typedef struct hep_chunk_str hep_chunk_str_t;

PACKED_STRUCT(struct hep_chunk_ip4 {
   hep_chunk_t chunk;
   struct in_addr data;
});

typedef struct hep_chunk_ip4 hep_chunk_ip4_t;

#ifdef USE_IPV6
PACKED_STRUCT(struct hep_chunk_ip6 {
   hep_chunk_t chunk;
   struct in6_addr data;
});

typedef struct hep_chunk_ip6 hep_chunk_ip6_t;
#endif // USE_IPV6

PACKED_STRUCT(struct hep_ctrl {
   char id[4];
   u_int16_t length;
});

typedef struct hep_ctrl hep_ctrl_t;

PACKED_STRUCT(struct hep_chunk_payload {
   hep_chunk_t chunk;
   char *data;
});

typedef struct hep_chunk_payload hep_chunk_payload_t;

/* Structure of HEP */

PACKED_STRUCT(struct hep_generic {
   hep_ctrl_t         header;
   hep_chunk_uint8_t  ip_family;
   hep_chunk_uint8_t  ip_proto;
   hep_chunk_uint16_t src_port;
   hep_chunk_uint16_t dst_port;
   hep_chunk_uint32_t time_sec;
   hep_chunk_uint32_t time_usec;
   hep_chunk_uint8_t  proto_t;
   hep_chunk_uint32_t capt_id;
});

typedef struct hep_generic hep_generic_t;

/* Ethernet / IP / UDP header IPv4 */
const int udp_payload_offset = 14 + 20 + 8;

struct hep_hdr {
   u_int8_t hp_v;            /* version */
   u_int8_t hp_l;            /* length */
   u_int8_t hp_f;            /* family */
   u_int8_t hp_p;            /* protocol */
   u_int16_t hp_sport;       /* source port */
   u_int16_t hp_dport;       /* destination port */
};

struct hep_timehdr {
   u_int32_t tv_sec;         /* seconds */
   u_int32_t tv_usec;        /* useconds */
   u_int16_t captid;         /* Capture ID node */
};

struct hep_iphdr {
   struct in_addr hp_src;
   struct in_addr hp_dst;    /* source and dest address */
};

#ifdef USE_IPV6
struct hep_ip6hdr {
   struct in6_addr hp6_src;  /* source address */
   struct in6_addr hp6_dst;  /* destination address */
};
#endif // USE_IPV6

UInt64 hepUnixTimestamp();

#endif // RESIP_HEP_HXX

/* ====================================================================
* The Vovida Software License, Version 1.0
*
* Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
* 3. The names "VOCAL", "Vovida Open Communication Application Library",
*    and "Vovida Open Communication Application Library (VOCAL)" must
*    not be used to endorse or promote products derived from this
*    software without prior written permission. For written
*    permission, please contact vocal@vovida.org.
*
* 4. Products derived from this software may not be called "VOCAL", nor
*    may "VOCAL" appear in their name, without prior written
*    permission of Vovida Networks, Inc.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
* NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
* NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
* IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
* DAMAGE.
*
* ====================================================================
*
* This software consists of voluntary contributions made by Vovida
* Networks, Inc. and many individuals on behalf of Vovida Networks,
* Inc.  For more information on Vovida Networks, Inc., please see
* <http://www.vovida.org/>.
*
*/
