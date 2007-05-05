#ifndef _DNS_RESOURCE_RECORD
#define _DNS_RESOURCE_RECORD

#include <iosfwd>

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
      virtual std::ostream& dump(std::ostream& strm) const = 0;
};

std::ostream& 
operator<<(std::ostream& strm, DnsResourceRecord& rr);

}





#endif
