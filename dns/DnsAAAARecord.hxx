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
      const Data& name() const { return mName; }
      bool isSameValue(const Data& value) const;
      
   private:
      struct in6_addr mAddr;
      Data mName;
      
};

}

#endif


#endif
