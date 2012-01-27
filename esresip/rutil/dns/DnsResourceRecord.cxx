/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#include "rutil/dns/DnsResourceRecord.hxx"
#include <iostream>

std::ostream& 
resip::operator<<(std::ostream& strm, DnsResourceRecord& rr)
{
   rr.dump(strm);
   return strm;
}

