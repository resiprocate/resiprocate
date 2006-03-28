#if defined(HAVE_CONFIG_H)
#include "rutil/config.hxx"
#endif

#if defined(USE_ARES)
#include "ares.h"
#include "ares_dns.h"
#endif

#ifndef __CYGWIN__
#ifndef RRFIXEDSZ
#define RRFIXEDSZ 10
#endif
#endif

#include <stdlib.h>

#include "rutil/Socket.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/dns/RROverlay.hxx"
#include "rutil/dns/DnsResourceRecord.hxx"
#include "rutil/dns/DnsAAAARecord.hxx"

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
