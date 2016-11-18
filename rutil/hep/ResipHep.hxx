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
*
* Copyright(c) 2010 - 2016 <Alexandr Dubovikov>
*
* All rights reserved.
*
* Redistribution and use in source and binary forms are permitted
* provided that the above copyright notice and this paragraph are
* duplicated in all such forms and that any documentation,
* advertising materials, and other materials related to such
* distribution and use acknowledge that the software was developed
* by the <SIPCAPTURE>.The name of the SIPCAPTURE may not be used to
* endorse or promote products derived from this software without specific
* prior written permission.
*
* THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*
* ====================================================================
*
* Contributor(s):
*
* Dario Bozzali - IFM Infomaster
*
*/
