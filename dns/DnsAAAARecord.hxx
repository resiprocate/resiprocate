#ifndef RESIP_DNS_AAAA_RECORD
#define RESIP_DNS_AAAA_RECORD

#ifdef USE_IPV6
namespace resip
{

class DnsResourceRecord;
class RROverlay;

class DnsAAAARecord : public DnsResourceRecord
{
   public:
      DnsAAAARecord(const RROverlay& overlay);
      ~DnsAAAARecord() {}

      const struct in6_addr& v6Address() const { return mAddr; }
      
   private:
      struct in6_addr mAddr;
      
};

}

#endif


#endif
