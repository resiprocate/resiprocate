#if defined(HAVE_CONFIG_H)
#include "rutil/config.hxx"
#endif

#include "AresCompat.hxx"

#ifndef __CYGWIN__
#ifndef RRFIXEDSZ
#define RRFIXEDSZ 10
#endif
#ifndef NS_RRFIXEDSZ
#define NS_RRFIXEDSZ 10
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

DnsAAAARecord::DnsAAAARecord(const RROverlay& overlay)
{
#ifdef USE_IPV6
   char* name = 0;
   long len = 0;
   ares_expand_name(overlay.data()-overlay.nameLength()-RRFIXEDSZ, overlay.msg(), overlay.msgLength(), &name, &len);
   mName = name;
   free(name);
   memcpy(&mAddr, overlay.data(), sizeof(in6_addr));
#else
   assert(0);
#endif
}

bool DnsAAAARecord::isSameValue(const Data& value) const
{
#ifdef USE_IPV6
   return DnsUtil::inet_ntop(mAddr) == value;
#else
   assert(0);
   return false;
#endif
}

EncodeStream&
DnsAAAARecord::dump(EncodeStream& strm) const
{
#ifdef USE_IPV6
   strm << mName << " (AAAA) --> " << DnsUtil::inet_ntop(mAddr);
   return strm;
#else
   assert(0);
   strm <<  " (AAAA) --> ? (IPV6 is disabled in this library, what "
                                    "happened here?)";
   return strm;
#endif
}
