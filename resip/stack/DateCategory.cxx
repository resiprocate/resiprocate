#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <string.h>
#include <ctype.h>
#include <time.h>

#include "resip/stack/DateCategory.hxx"

#include "resip/stack/Transport.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Socket.hxx"
//#include "rutil/WinLeakCheck.hxx"  // not compatible with placement new used below

using namespace resip;
using namespace std;

namespace resip
{
// Implemented in gen/DayOfWeekHash.cxx
struct days { const char *name; DayOfWeek type; };
class DayOfWeekHash
{
private:
  static inline unsigned int hash (const char *str, unsigned int len);
public:
  static const struct days *in_word_set (const char *str, unsigned int len);
};

// Implemented in gen/MonthHash.cxx
struct months { const char *name; Month type; };
class MonthHash
{
private:
  static inline unsigned int hash (const char *str, unsigned int len);
public:
  static const struct months *in_word_set (const char *str, unsigned int len);
};
}

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

//====================
// Date
//====================

Data resip::DayOfWeekData[] =
{
   "Sun",
   "Mon",
   "Tue",
   "Wed",
   "Thu",
   "Fri",
   "Sat"
};

Data resip::MonthData[] =
{
   "Jan",
   "Feb",
   "Mar",
   "Apr",
   "May",
   "Jun",
   "Jul",
   "Aug",
   "Sep",
   "Oct",
   "Nov",
   "Dec"
};


DateCategory::DateCategory()
   : ParserCategory(),
     mDayOfWeek(Sun),
     mDayOfMonth(),
     mMonth(Jan),
     mYear(0),
     mHour(0),
     mMin(0),
     mSec(0)
{
   time_t now;
   time(&now);
   if (now == ((time_t)-1))
   {
      int e = getErrno();
      DebugLog (<< "Failed to get time: " << strerror(e));
      Transport::error(e);
      return;
   }
   
   setDatetime(now);
}

DateCategory::DateCategory(time_t datetime)
   : ParserCategory(),
     mDayOfWeek(Sun),
     mDayOfMonth(),
     mMonth(Jan),
     mYear(0),
     mHour(0),
     mMin(0),
     mSec(0)
{
   setDatetime(datetime);
}

DateCategory::DateCategory(const HeaderFieldValue& hfv, 
                           Headers::Type type,
                           PoolBase* pool)
   : ParserCategory(hfv, type, pool),
     mDayOfWeek(Sun),
     mDayOfMonth(),
     mMonth(Jan),
     mYear(0),
     mHour(0),
     mMin(0),
     mSec(0)
{}

DateCategory::DateCategory(const DateCategory& rhs,
                           PoolBase* pool)
   : ParserCategory(rhs, pool),
     mDayOfWeek(rhs.mDayOfWeek),
     mDayOfMonth(rhs.mDayOfMonth),
     mMonth(rhs.mMonth),
     mYear(rhs.mYear),
     mHour(rhs.mHour),
     mMin(rhs.mMin),
     mSec(rhs.mSec)
{}

DateCategory&
DateCategory::operator=(const DateCategory& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mDayOfWeek = rhs.mDayOfWeek;
      mDayOfMonth = rhs.mDayOfMonth;
      mMonth = rhs.mMonth;
      mYear = rhs.mYear;
      mHour = rhs.mHour;
      mMin = rhs.mMin;
      mSec = rhs.mSec;
   }
   return *this;
}

bool 
DateCategory::setDatetime(time_t datetime)
{
   struct tm gmt;
#if defined(WIN32) || defined(__sun)
   struct tm *gmtp = gmtime(&datetime);
   if (gmtp == 0)
   {
        int e = getErrno();
        DebugLog (<< "Failed to convert to gmt: " << strerror(e));
        Transport::error(e);
        return false;
   }
   memcpy(&gmt,gmtp,sizeof(gmt));
#else
  if (gmtime_r(&datetime, &gmt) == 0)
  {
     int e = getErrno();
     DebugLog (<< "Failed to convert to gmt: " << strerror(e));
     Transport::error(e);
     return false;
  }
#endif

   mDayOfWeek = static_cast<DayOfWeek>(gmt.tm_wday);
   mDayOfMonth = gmt.tm_mday;
   mMonth = static_cast<Month>(gmt.tm_mon);
   mYear = gmt.tm_year + 1900;
   mHour = gmt.tm_hour;
   mMin = gmt.tm_min;
   mSec = gmt.tm_sec;
   DebugLog (<< "Set date: day=" << mDayOfWeek 
             << " month=" << mMonth
             << " year=" << mYear
             << " " << mHour << ":" << mMin << ":" << mSec);
   return true;
}

DayOfWeek
DateCategory::DayOfWeekFromData(const Data& dow)
{
   register const char *str = dow.data();
   register Data::size_type len = dow.size();

   const struct days* _day = DayOfWeekHash::in_word_set(str, len);
   if(_day != 0)
   {
      return _day->type;
   }
   else
   {
      return Sun;
   }
}

Month
DateCategory::MonthFromData(const Data& mon)
{
   register const char *str = mon.data();
   register Data::size_type len = mon.size();

   const struct months* _month = MonthHash::in_word_set(str, len);
   if(_month != 0)
   {
      return _month->type;
   }
   else
   {
      return Jan;
   }
}

const DayOfWeek& 
DateCategory::dayOfWeek() const 
{
   checkParsed();
   return mDayOfWeek;
}

int&
DateCategory::dayOfMonth() 
{
   checkParsed();
   return mDayOfMonth;
}

int 
DateCategory::dayOfMonth() const 
{
   checkParsed();
   return mDayOfMonth;
}

Month& 
DateCategory::month() 
{
   checkParsed();
   return mMonth;
}

Month
DateCategory::month() const
{
   checkParsed();
   return mMonth;
}

int& 
DateCategory::year() 
{
   checkParsed();
   return mYear;
}

int
DateCategory::year() const 
{
   checkParsed();
   return mYear;
}

int&
DateCategory::hour() 
{
   checkParsed();
   return mHour;
}

int
DateCategory::hour() const 
{
   checkParsed();
   return mHour;
}

int&
DateCategory::minute() 
{
   checkParsed();
   return mMin;
}

int
DateCategory::minute() const 
{
   checkParsed();
   return mMin;
}

int&
DateCategory::second() 
{
   checkParsed();
   return mSec;
}

int
DateCategory::second() const 
{
   checkParsed();
   return mSec;
}

void
DateCategory::parse(ParseBuffer& pb)
{
   // Mon, 04 Nov 2002 17:34:15 GMT

   const char* anchor = pb.skipWhitespace();

   pb.skipToChar(Symbols::COMMA[0]);
   Data dayOfWeek;
   pb.data(dayOfWeek, anchor);
   mDayOfWeek = DateCategory::DayOfWeekFromData(dayOfWeek);

   pb.skipChar(Symbols::COMMA[0]);

   pb.skipWhitespace();

   mDayOfMonth = pb.integer();

   anchor = pb.skipWhitespace();
   pb.skipNonWhitespace();

   Data month;
   pb.data(month, anchor);
   mMonth = DateCategory::MonthFromData(month);

   pb.skipWhitespace();
   mYear = pb.integer();

   pb.skipWhitespace();

   mHour = pb.integer();
   pb.skipChar(Symbols::COLON[0]);
   mMin = pb.integer();
   pb.skipChar(Symbols::COLON[0]);
   mSec = pb.integer();

   pb.skipWhitespace();
   pb.skipChar('G');
   pb.skipChar('M');
   pb.skipChar('T');

   pb.skipWhitespace();
   pb.assertEof();
}

ParserCategory* 
DateCategory::clone() const
{
   return new DateCategory(*this);
}

ParserCategory* 
DateCategory::clone(void* location) const
{
   return new (location) DateCategory(*this);
}

ParserCategory* 
DateCategory::clone(PoolBase* pool) const
{
   return new (pool) DateCategory(*this, pool);
}

static void pad2(const int x, EncodeStream& str)
{
   if (x < 10)
   {
      str << Symbols::ZERO[0];
   }
   str << x;
}

EncodeStream& 
DateCategory::encodeParsed(EncodeStream& str) const
{
   str << DayOfWeekData[mDayOfWeek] // Mon
       << Symbols::COMMA[0] << Symbols::SPACE[0];
   
   pad2(mDayOfMonth, str);  //  04

   str << Symbols::SPACE[0]
       << MonthData[mMonth] << Symbols::SPACE[0] // Nov
       << mYear << Symbols::SPACE[0]; // 2002

   pad2(mHour, str);
   str << Symbols::COLON[0];
   pad2(mMin, str);
   str << Symbols::COLON[0];
   pad2(mSec, str);
   str << " GMT";

   return str;
}


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
