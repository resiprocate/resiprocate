#ifndef RESIP_DNS_AAAA_RECORD
#define RESIP_DNS_AAAA_RECORD

#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "rutil/dns/DnsResourceRecord.hxx"

#ifndef WIN32
#include <netinet/in.h>
#else
#include <Ws2tcpip.h>
#endif

namespace resip
{

class RROverlay;

class DnsAAAARecord : public DnsResourceRecord
{
   public:
      DnsAAAARecord(const RROverlay& overlay);
      virtual ~DnsAAAARecord() {}

#ifdef IPPROTO_IPV6
      const struct in6_addr& v6Address() const { return mAddr; }
#endif

      virtual const Data& name() const { return mName; }
      virtual bool isSameValue(const Data& value) const;
      EncodeStream& dump(EncodeStream& strm) const;
      
   private:
      union
      {
#ifdef IPPROTO_IPV6
         struct in6_addr mAddr;
#endif
         char pad[28]; // this make union same size if v6 is in or out
      };
      Data mName;
};

}

#endif
