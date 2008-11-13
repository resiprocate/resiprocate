#if defined(HAVE_CONFIG_H)
#include "rutil/config.hxx"
#endif

#include <stdlib.h>

#if defined(USE_ARES)
#include "ares.h"
#include "ares_dns.h"
#endif

#ifndef __CYGWIN__
#ifndef RRFIXEDSZ
#define RRFIXEDSZ 10
#endif
#endif

#include "rutil/Data.hxx"
#include "rutil/BaseException.hxx"
#include "RROverlay.hxx"
#include "DnsResourceRecord.hxx"
#include "DnsSrvRecord.hxx"

using namespace resip;

DnsSrvRecord::DnsSrvRecord(const RROverlay& overlay)
{
   char* name = 0;
   int len = 0;
   if (RARES_SUCCESS != rares_expand_name(overlay.data()-overlay.nameLength()-RRFIXEDSZ, overlay.msg(), overlay.msgLength(), &name, &len))
   {
      throw SrvException("Failed parse of SRV record", __FILE__, __LINE__);
   }
   mName = name;
   free(name);

   mPriority = RARES_DNS__16BIT(overlay.data());
   mWeight = RARES_DNS__16BIT(overlay.data() + 2);
   mPort = RARES_DNS__16BIT(overlay.data() + 4);

   if (RARES_SUCCESS != rares_expand_name(overlay.data() + 6, overlay.msg(), overlay.msgLength(), &name, &len))
   {
      throw SrvException("Failed parse of SRV record", __FILE__, __LINE__);
   }
   mTarget = name;
   free(name);
}

bool DnsSrvRecord::isSameValue(const Data& value) const
{
   return value == (mTarget + ":" + Data(mPort));
}

EncodeStream&
DnsSrvRecord::dump(EncodeStream& strm) const
{
   strm << mName << " (SRV) --> p=" << mPriority << " w=" << mWeight << " " << mTarget << ":" << mPort;
   return strm;
}
