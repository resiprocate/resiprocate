#ifndef RESIP_DNS_AAAA_RECORD
#define RESIP_DNS_AAAA_RECORD

namespace resip
{

class DnsResourceRecord;
class RROverlay;

class DnsAAAARecord : public DnsResourceRecord
{
   public:
      DnsAAAARecord(const RROverlay& overlay);
      ~DnsAAAARecord() {}

      const in6_addr& v6Address() const { return mAddr; }
      
   private:
      in6_addr mAddr;
      
};

}


#endif
