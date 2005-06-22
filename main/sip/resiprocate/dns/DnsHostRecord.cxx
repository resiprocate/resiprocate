#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#if defined(USE_ARES)
extern "C"
{
#include "ares.h"
#include "ares_dns.h"
}
#endif

#ifndef __CYGWIN__
#ifndef RRFIXEDSZ
#define RRFIXEDSZ 10
#endif
#endif

#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "RROverlay.hxx"
#include "DnsResourceRecord.hxx"
#include "DnsHostRecord.hxx"

using namespace resip;

DnsHostRecord::DnsHostRecord(const RROverlay& overlay)
{
   char* name = 0;
   int len = 0;
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

