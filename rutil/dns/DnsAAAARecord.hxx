#ifndef RESIP_DNS_AAAA_RECORD
#define RESIP_DNS_AAAA_RECORD

#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "rutil/dns/DnsResourceRecord.hxx"

#ifdef USE_IPV6

namespace resip
{

class RROverlay;

class DnsAAAARecord : public DnsResourceRecord
{
   public:
      DnsAAAARecord(const RROverlay& overlay);
      virtual ~DnsAAAARecord() {}

      const struct in6_addr& v6Address() const { return mAddr; }
      virtual const Data& name() const { return mName; }
      virtual bool isSameValue(const Data& value) const;
      ostream& operator<<(ostream& strm) const;
      
   private:
      struct in6_addr mAddr;
      Data mName;
      
};

}

#endif


#endif
