#ifndef _DNS_RESOURCE_RECORD
#define _DNS_RESOURCE_RECORD

namespace resip
{

class Data;

class DnsResourceRecord
{
   public:
      DnsResourceRecord() {}
      virtual ~DnsResourceRecord()
      {
      }
      virtual const Data& name() const = 0;
      virtual bool isSameValue(const Data& valueToCompare) const = 0;

protected:

};

}

#endif
