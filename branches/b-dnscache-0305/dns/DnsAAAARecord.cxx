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

#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/dns/RROverlay.hxx"
#include "resiprocate/dns/DnsResourceRecord.hxx"
#include "resiprocate/dns/DnsAAAARecord.hxx"

using namespace resip;

#ifdef USE_IPV6

DnsAAAARecord::DnsAAAARecord(const RROverlay& overlay)
{
   char* name = 0;
   int len = 0;
   ares_expand_name(overlay.data()-overlay.nameLength()-RRFIXEDSZ, overlay.msg(), overlay.msgLength(), &name, &len);
   mName = name;
   free(name);
   memcpy(&mAddr, overlay.data(), sizeof(in6_addr));
}

bool DnsAAAARecord::isSameValue(const Data& value) const
{
   return DnsUtil::inet_ntop(mAddr) == value;
}

#endif
