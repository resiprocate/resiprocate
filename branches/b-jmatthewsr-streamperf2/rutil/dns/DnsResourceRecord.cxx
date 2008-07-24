#include "rutil/dns/DnsResourceRecord.hxx"
#include <iostream>

EncodeStream& 
resip::operator<<(EncodeStream& strm, DnsResourceRecord& rr)
{
   rr.dump(strm);
   return strm;
}

