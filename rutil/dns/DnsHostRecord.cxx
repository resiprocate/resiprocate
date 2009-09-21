#if defined(HAVE_CONFIG_H)
#include "rutil/config.hxx"
#endif

#include <stdlib.h>

#include "AresCompat.hxx"

#ifndef __CYGWIN__
#ifndef RRFIXEDSZ
#define RRFIXEDSZ 10
#endif
#ifndef NS_RRFIXEDSZ
#define NS_RRFIXEDSZ 10
#endif
#endif

#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/BaseException.hxx"
#include "RROverlay.hxx"
#include "DnsResourceRecord.hxx"
#include "DnsHostRecord.hxx"

using namespace resip;

DnsHostRecord::DnsHostRecord(const RROverlay& overlay)
{
   char* name = 0;
   long len = 0;
   ares_expand_name(overlay.data()-overlay.nameLength()-RRFIXEDSZ, overlay.msg(), overlay.msgLength(), &name, &len);
   mName = name;
   free(name);
   memcpy(&mAddr, overlay.data(), sizeof(in_addr));
}

Data DnsHostRecord::host() const
{
   return Data(inet_ntoa(mAddr));
}

bool DnsHostRecord::isSameValue(const Data& value) const
{
   return DnsUtil::inet_ntop(mAddr) == value;
}

EncodeStream&
DnsHostRecord::dump(EncodeStream& strm) const
{
   strm << mName << "(A)--> " << DnsUtil::inet_ntop(mAddr);
   return strm;
}
