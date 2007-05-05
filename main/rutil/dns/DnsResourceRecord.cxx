#include "rutil/dns/DnsResourceRecord.hxx"

std::ostream& 
resip::operator<<(std::ostream& strm, DnsResourceRecord& rr)
{
   rr.dump(strm);
   return strm;
}

