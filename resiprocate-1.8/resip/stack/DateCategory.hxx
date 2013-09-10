#if !defined(RESIP_DATE_CATEGORY_HXX)
#define RESIP_DATE_CATEGORY_HXX 

#include <iosfwd>
#include "rutil/Data.hxx"
#include "resip/stack/ParserCategory.hxx"

namespace resip
{

//====================
// DateCategory:
//====================

enum DayOfWeek { 
   Sun = 0,
   Mon,
   Tue,
   Wed,
   Thu,
   Fri,
   Sat
};

extern Data DayOfWeekData[];
extern Data MonthData[];

enum Month {
   Jan = 0,
   Feb,
   Mar,
   Apr,
   May,
   Jun,
   Jul,
   Aug,
   Sep,
   Oct,
   Nov,
   Dec
};

/**
   @ingroup sip_grammar
   @brief Represents the "SIP-date" element in the RFC 3261 grammar.
*/
class DateCategory : public ParserCategory
{
   public:
      enum {commaHandling = NoCommaTokenizing};

      DateCategory();
      DateCategory(time_t datetime);
      DateCategory(const HeaderFieldValue& hfv, 
                     Headers::Type type,
                     PoolBase* pool=0);
      DateCategory(const DateCategory& orig,
                     PoolBase* pool=0);
      DateCategory& operator=(const DateCategory&);

      virtual bool setDatetime(time_t datetime);

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual ParserCategory* clone(void* location) const;
      virtual ParserCategory* clone(PoolBase* pool) const;

      virtual EncodeStream& encodeParsed(EncodeStream& str) const;
      
      static DayOfWeek DayOfWeekFromData(const Data&);
      static Month MonthFromData(const Data&);

      const DayOfWeek& dayOfWeek() const;
      int& dayOfMonth();
      int dayOfMonth() const;
      Month& month();
      Month month() const;
      int& year();
      int year() const;
      int& hour();
      int hour() const;
      int& minute();
      int minute() const;
      int& second();
      int second() const;

   private:
      enum DayOfWeek mDayOfWeek;
      int mDayOfMonth;
      enum Month mMonth;
      int mYear;
      int mHour;
      int mMin;
      int mSec;
};
 
}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
