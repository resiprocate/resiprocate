#include "rutil/dns/DnsResourceRecord.hxx"
#include <iostream>

resip::EncodeStream& 
resip::operator<<(resip::EncodeStream& strm, DnsResourceRecord& rr)
{
   rr.dump(strm);
   return strm;
}

