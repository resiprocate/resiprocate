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
#include "DnsCnameRecord.hxx"

using namespace resip;

DnsCnameRecord::DnsCnameRecord(const RROverlay& overlay)
{
   char* name = 0;
   int len = 0;
   if (ARES_SUCCESS != ares_expand_name(overlay.data(), overlay.msg(), overlay.msgLength(), &name, &len))
   {
      throw CnameException("Failed parse of CNAME record", __FILE__, __LINE__);
   }
   
   mName = name;
   free(name);
}
