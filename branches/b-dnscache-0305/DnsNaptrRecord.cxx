#if defined(USE_ARES)
extern "C"
{
#include "ares.h"
#include "ares_dns.h"
}
#endif

#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "RROverlay.hxx"
#include "DnsResourceRecord.hxx"
#include "DnsNaptrRecord.hxx"

using namespace resip;

DnsNaptrRecord::DnsNaptrRecord(const RROverlay& overlay)
{
   char* name = 0;
   int len = 0;
   ares_expand_name(overlay.data(), overlay.msg(), overlay.msgLength(), &name, &len);
   mName = name;
   free(name);

   mOrder = DNS__16BIT(overlay.data());
   mPreference = DNS__16BIT(overlay.data() + 2);
   const unsigned char* pPos = overlay.data() + 4;
   len = *pPos;

   if (pPos + len + 1 > overlay.data() + overlay.dataLength())
   {
      throw NaptrException("Failed parse of NAPTR record", __FILE__, __LINE__);
   }

   mFlags = Data(pPos + 1, len);
   pPos += len + 1;
   len = *pPos;

   if (pPos + len + 1 > overlay.data() + overlay.dataLength())
   {
      throw NaptrException("Failed parse of NAPTR record", __FILE__, __LINE__);
   }
   mService = Data(pPos + 1, len);
   pPos += len + 1;
   len = *pPos;

   if (pPos + len + 1 > overlay.data() + overlay.dataLength())
   {
      throw NaptrException("Failed parse of NAPTR record", __FILE__, __LINE__);
   }
   mRegexp = Data(pPos + 1, len);
   pPos += len + 1;

   if (ARES_SUCCESS != ares_expand_name(pPos, overlay.msg(), overlay.msgLength(), &name, &len))
   {
      throw NaptrException("Failed parse of NAPTR record", __FILE__, __LINE__);
   }
   mReplacement = name;
   free(name);
}
