#ifndef RESIP_DNS_HOST_RECORD
#define RESIP_DNS_HOST_RECORD

namespace resip
{

class DnsResourceRecord;
class RROverlay;

class DnsHostRecord : public DnsResourceRecord
{
   public:
      DnsHostRecord(const RROverlay&);
      ~DnsHostRecord() {}

      // accessors.
      Data host() const;
      in_addr addr() const { return mAddr; }
      
   private:
      in_addr mAddr;
};

}


#endif
