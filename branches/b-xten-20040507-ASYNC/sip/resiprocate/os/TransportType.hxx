#if !defined(RESIP_TRANSPORTTYPE_HXX)
#define RESIP_TRANSPORTTYPE_HXX

namespace resip
{

typedef enum 
{
   UNKNOWN_TRANSPORT = 0,
   UDP,
   TCP,
   TLS,
   SCTP,
   DCCP,
   MAX_TRANSPORT
} TransportType;

typedef enum 
{
   V4,
   V6
} IpVersion;

}

#endif
