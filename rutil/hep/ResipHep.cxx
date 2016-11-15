#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rutil/hep/ResipHep.hxx"

#include "rutil/ResipAssert.h"

// Under Windows we use FILETIME, whose epoch is 1601-01-01 00:00:00 UTC.
// To obtain Unix timestamp we must calculate difference with Unix epoc,
// that is 1970-01-01 00:00:00 UTC.
// In this way we obtain: ((1970 - 1601) * 365 + 89) * 24 * 60 * 60 * 1000 * 1000,
// where 89 is the number of leap year days between 1601 and 1970:
// (1970 - 1601) / 4 excluding 1700, 1800, and 1900.
#define EPOCH_DIFFERENCE 11644473600000000LL

// Returns microseconds since Unix epoch time (UTC)
UInt64 hepUnixTimestamp()
{
   resip_assert(sizeof(UInt64) == 64/8);

   UInt64 time = 0;

#if defined(WIN32) || defined(UNDER_CE)
   FILETIME ft;

#ifdef UNDER_CE
   SYSTEMTIME st;
   ::GetSystemTime(&st);
   ::SystemTimeToFileTime(&st, &ft);
#else // UNDER_CE
   ::GetSystemTimeAsFileTime(&ft);
#endif // UNDER_CE

   ULARGE_INTEGER li;
   li.LowPart = ft.dwLowDateTime;
   li.HighPart = ft.dwHighDateTime;
 
   // divide by 10 to convert 100-nanoseconds to microseconds,
   // then subtract EPOCH_DIFFERENCE to obtain microseconds
   // from Unix epoch time
   time = (li.QuadPart / 10) - EPOCH_DIFFERENCE;

#else // #if defined(WIN32) || defined(UNDER_CE)

   struct timeval now;
   gettimeofday(&now , NULL);

   time = now.tv_sec;
   time = time * 1000000;
   time += now.tv_usec;

#endif // #if defined(WIN32) || defined(UNDER_CE)

   return time;
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
