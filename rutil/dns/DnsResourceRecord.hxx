#ifndef _DNS_RESOURCE_RECORD
#define _DNS_RESOURCE_RECORD

#include <iosfwd>
#include "rutil/resipfaststreams.hxx"

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
      virtual EncodeStream& dump(EncodeStream& strm) const = 0;
};

EncodeStream& 
operator<<(EncodeStream& strm, DnsResourceRecord& rr);

}





#endif
