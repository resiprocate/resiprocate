#if defined(HAVE_CONFIG_H)
#include "config.h"
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
      ParseBuffer pb(data, "DnsNaptrRecord::RegExp parser");

      const char delim = data[0];
      const char* start = pb.skipChar(delim);
      pb.skipToChar(delim);

      pb.data(mRegexp, start);
      start = pb.skipChar(delim);
      pb.skipToChar(delim);
      pb.data(mReplacement, start);
      start = pb.skipChar(delim);
      // .kw. start above is not used -- what is going on here?
      // should above code be in #if block below?
      
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
   long len = 0;
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

EncodeStream&
DnsNaptrRecord::dump(EncodeStream& strm) const
{
   strm << mName << " (NAPTR)--> o=" << mOrder << " p=" << mPreference;
   return strm;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
 