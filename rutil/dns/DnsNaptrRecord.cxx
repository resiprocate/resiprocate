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
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/dns/RROverlay.hxx"
#include "rutil/dns/DnsResourceRecord.hxx"
#include "rutil/dns/DnsNaptrRecord.hxx"

using namespace resip;
#define RESIPROCATE_SUBSYSTEM resip::Subsystem::DNS

DnsNaptrRecord::RegExp::RegExp()
{
}

DnsNaptrRecord::RegExp::RegExp(const Data& data)  
{
   if (data.size() > 1)
   {
      ParseBuffer pb(const_cast<Data&>(data), "DnsNaptrRecord::RegExp parser");

      const char delim = data[0];
      const char* start = pb.skipChar(delim);
      pb.skipToChar(delim);

      pb.data(mRegexp, start);
      start = pb.skipChar(delim);
      pb.skipToChar(delim);
      pb.data(mReplacement, start);
      start = pb.skipChar(delim);
      
#if 0
      //pb.data(mFlags, start);

      if (regcomp(&mRe, mRegexp.c_str(), REG_EXTENDED) != 0)
      {
         // couldn't parse input regexp so ignore it
         mRegexp.clear();
      }
#endif
   }
}

DnsNaptrRecord::RegExp::~RegExp()
{
   //regfree(&mRe);
}


bool
DnsNaptrRecord::RegExp::empty() const
{
   return mRegexp.empty();
}

const Data&
DnsNaptrRecord::RegExp::regexp() const
{
   return mRegexp;
}

const Data&
DnsNaptrRecord::RegExp::replacement() const
{
   return mReplacement;
}

Data
DnsNaptrRecord::RegExp::apply(const Data& input) const
{
   // !jf! should be doing a real regexp here
   //regmatch_t matches[10];
   //regexec(&mRe, input.c_str(), 10, matches, 0);
   
   return mReplacement;
}


DnsNaptrRecord::DnsNaptrRecord(const RROverlay& overlay)
{
   char* name = 0;
   int len = 0;
   if (ARES_SUCCESS != ares_expand_name(overlay.data()-overlay.nameLength()-RRFIXEDSZ, overlay.msg(), overlay.msgLength(), &name, &len))
   {
      throw NaptrException("Failed parse of NAPTR record", __FILE__, __LINE__);
   }
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
   Data regexp(pPos + 1, len);
   pPos += len + 1;
   mRegexp = DnsNaptrRecord::RegExp(regexp);
   InfoLog (<< "regexp=" << mRegexp.regexp() << " rep=" << mRegexp.replacement());

   if (pPos[0] != 0)
   {
      if (ARES_SUCCESS != ares_expand_name(pPos, overlay.msg(), overlay.msgLength(), &name, &len))
      {
         throw NaptrException("Failed parse of NAPTR record", __FILE__, __LINE__);
      }
      mReplacement = name;
      free(name);
   }
}

bool DnsNaptrRecord::isSameValue(const Data& value) const
{
   return mReplacement == value;
}
