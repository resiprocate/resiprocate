#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include <time.h>

#include "resiprocate/DateCategory.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#if !defined(DISABLE_RESIP_LOG)
#include "resiprocate/os/Logger.hxx"
#endif
#include "resiprocate/os/ParseBuffer.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#if !defined(DISABLE_RESIP_LOG)
#define RESIPROCATE_SUBSYSTEM Subsystem::SIP
#endif
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
#if !defined(DISABLE_RESIP_LOG)
      DebugLog (<< "Failed to get time: " << strerror(e));
#endif
      return;
   }
   
   struct tm gmt;
#if defined(_WIN32) || defined(__sun)
   struct tm *gmtp = gmtime(&now);
   if (gmtp == 0)
   {
      int e = getErrno();
#if !defined(DISABLE_RESIP_LOG)
      DebugLog (<< "Failed to convert to gmt: " << strerror(e));
#endif
      return;
   }
   memcpy(&gmt,gmtp,sizeof(gmt));
#else
  if (gmtime_r(&now, &gmt) == 0)
  {
     int e = getErrno();
     DebugLog (<< "Failed to convert to gmt: " << strerror(e));
     Transport::error(e);
     return;
  }
#endif

   mDayOfWeek = static_cast<DayOfWeek>(gmt.tm_wday);
   mDayOfMonth = gmt.tm_mday;
   mMonth = static_cast<Month>(gmt.tm_mon);
   mYear = gmt.tm_year + 1900;
   mHour = gmt.tm_hour;
   mMin = gmt.tm_min;
   mSec = gmt.tm_sec;
#if !defined(DISABLE_RESIP_LOG)
   DebugLog (<< "Set date: day=" << mDayOfWeek 
             << " month=" << mMonth
             << " year=" << mYear
             << " " << mHour << ":" << mMin << ":" << mSec);
#endif
}

DateCategory::DateCategory(HeaderFieldValue* hfv, Headers::Type type)
   : ParserCategory(hfv, type),
     mDayOfWeek(Sun),
     mDayOfMonth(),
     mMonth(Jan),
     mYear(0),
     mHour(0),
     mMin(0),
     mSec(0)
{}

DateCategory::DateCategory(const DateCategory& rhs)
{
   *this = rhs;
}

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

/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -L ANSI-C -t -k '*' dayofweek.gperf  */
struct days { char name[32]; DayOfWeek day; };

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
dayofweek_hash (register const char *str, register unsigned int len)
{
  static unsigned char asso_values[] =
    {
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
       1, 13, 13, 13, 13, 13, 13,  5, 13, 13,
      13, 13, 13,  2,  0, 13, 13,  7, 13, 13,
      13, 13, 13, 13, 13, 13, 13,  7, 13, 13,
       0,  0, 13, 13,  4,  0, 13, 13, 13, 13,
       0,  0, 13, 13,  0, 13,  0,  0, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
      13, 13, 13, 13, 13, 13
    };
  register int hval = len;

  switch (hval)
    {
      default:
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

#ifdef __GNUC__
__inline
#endif
struct days *
in_dayofweek_word_set (register const char *str, register unsigned int len)
{
   static const unsigned int MIN_WORD_LENGTH = 3;
   static const unsigned int MAX_WORD_LENGTH = 3;
   static const int MAX_HASH_VALUE = 12;
   
   static struct days wordlist[] =
    {
      {""}, {""}, {""},
      {"Tue", Tue},
      {"Fri", Fri},
      {"Sun", Sun},
      {""},
      {"Thu", Thu},
      {"Mon", Mon},
      {""},
      {"Wed", Wed},
      {""},
      {"Sat", Sat}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = dayofweek_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].name;

          if (*str == *s && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}

DayOfWeek
DateCategory::DayOfWeekFromData(const Data& dow)
{
   static const unsigned int MIN_WORD_LENGTH = 3;
   static const unsigned int MAX_WORD_LENGTH = 3;
   static const int MAX_HASH_VALUE = 12;

   register const char *str = dow.data();
   register Data::size_type len = dow.size();

   static struct days wordlist[] =
      {
         {""}, {""}, {""},
         {"Tue", Tue},
         {"Fri", Fri},
         {"Sun", Sun},
         {""},
         {"Thu", Thu},
         {"Mon", Mon},
         {""},
         {"Wed", Wed},
         {""},
         {"Sat", Sat}
      };

   if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
   {
      register int key = dayofweek_hash (str, len);
      
      if (key <= MAX_HASH_VALUE && key >= 0)
      {
         register const char *s = wordlist[key].name;
         
         if (*str == *s && !strncmp (str+1, s+1, len-1))
         {
            return wordlist[key].day;
         }
      }
   }
   return Sun;
}

/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -L ANSI-C -t -k '*' month.gperf  */
struct months { char name[32]; Month type; };

/* maximum key range = 31, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
month_hash (register const char *str, register unsigned int len)
{
   static unsigned char asso_values[] =
      {
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 15, 34, 34,  8, 34,
         5, 34, 34, 34,  0, 34, 34, 10,  3, 14,
         34, 34, 34,  9, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 10,  0,  0,
         34,  0, 34, 10, 34, 34, 34, 34,  4, 34,
         0,  0,  0, 34,  0, 34,  0,  0,  0, 34,
         34, 10, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
         34, 34, 34, 34, 34, 34
      };
   register int hval = len;

   switch (hval)
   {
      default:
      case 3:
         hval += asso_values[(unsigned char)str[2]];
      case 2:
         hval += asso_values[(unsigned char)str[1]];
      case 1:
         hval += asso_values[(unsigned char)str[0]];
         break;
   }
   return hval;
}

Month
DateCategory::MonthFromData(const Data& mon)
{
   static const unsigned int MIN_WORD_LENGTH = 3;
   static const unsigned int MAX_WORD_LENGTH = 3;
   static const int MAX_HASH_VALUE = 33;

   register const char *str = mon.data();
   register Data::size_type len = mon.size();

   static struct months wordlist[] =
      {
         {""}, {""}, {""},
         {"Jun", Jun},
         {""}, {""},
         {"Nov", Nov},
         {"Jul", Jul},
         {"Feb", Feb},
         {""}, {""},
         {"Dec", Dec},
         {"Sep", Sep},
         {"Jan", Jan},
         {""}, {""}, {""},
         {"Oct", Oct},
         {"Apr", Apr},
         {""}, {""}, {""}, {""},
         {"Mar", Mar},
         {""}, {""}, {""}, {""},
         {"Aug", Aug},
         {""}, {""}, {""}, {""},
         {"May", May}
      };

   if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
   {
      register int key = month_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
      {
         register const char *s = wordlist[key].name;

         if (*str == *s && !strncmp (str + 1, s + 1, mon.size()-1))
            return wordlist[key].type;
      }
   }
   return Jan;
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

static void pad2(const int x, std::ostream& str)
{
   if (x < 10)
   {
      str << Symbols::ZERO[0];
   }
   str << x;
}

std::ostream& 
DateCategory::encodeParsed(std::ostream& str) const
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
