#include "CDRFile.hxx"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <fstream>
#include <ctime>

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <AppSubsystem.hxx>

#define RESIPROCATE_SUBSYSTEM AppSubsystem::RECONSERVER

using namespace resip;
using namespace recon;
using namespace reconserver;

// FIXME - do all the file IO on another thread so that we
//         don't block the B2BUA
// FIXME - handle any exceptions within this class so that
//         the process isn't impacted
CDRFile::CDRFile(const resip::Data& filename)
    : mSep(','),
      mFilename(filename),
      mRotate(false)
{
   mFile.open(mFilename.c_str(), std::ios::app);
}

CDRFile::~CDRFile()
{
   mFile.close();
}

void
CDRFile::log(SharedPtr <B2BCall> call)
{
   if(mRotate)
   {
      StackLog(<<"rotating the CDR file");
      mFile.close();
      mFile.open(mFilename.c_str(), std::ios::app);
      mRotate = false;
   }
   logString(call->getB2BCallID());
   logString(call->getCaller());
   logString(call->getCallee());
   logString(call->getOriginZone());
   logString(call->getDestinationZone());
   logTimestamp(call->getStart());
   Data disposition;
   if(call->answered())
   {
      logTimestamp(call->getConnect());
      disposition = "ANSWERED";
   }
   else
   {
      logString(Data::Empty);
      switch(call->getResponseCode())
      {
      case 486:
         disposition = "BUSY";
         break;
      case 487:
         disposition = "NO ANSWER";
         break;
      default:
         disposition = "FAILED";
      }
   }
   logTimestamp(call->getFinish());
   logTimediff(call->getFinish() - call->getStart());
   if(call->answered())
   {
      logTimediff(call->getFinish() - call->getConnect());
   }
   else
   {
      logTimediff(0);
   }
   logString(disposition);
   logNumeric(call->getResponseCode(), true);
}

void
CDRFile::rotateLog()
{
   mRotate = true;
}

void
CDRFile::logString(const resip::Data& s, bool last, bool quote)
{
   if(quote)
   {
      mFile << '"' << s << '"';
   }
   else
   {
      mFile << s;
   }
   if(last)
   {
      mFile << std::endl;
   }
   else
   {
      mFile << mSep;
   }
}

void
CDRFile::logTimestamp(const uint64_t& t, bool last)
{
   const time_t timeInSeconds = (time_t)(t / 1000);
   const int millis = t % 1000;

   char datebuf[256];
   const unsigned int datebufSize = 256;
   struct tm localTimeResult;
   strftime (datebuf,
             datebufSize,
             "%Y%m%d-%H%M%S", /* guaranteed to fit in 256 chars,
                                 hence don't check return code */
#ifdef WIN32
             localtime (&timeInSeconds));  // Thread safe call on Windows
#else
             localtime_r (&timeInSeconds, &localTimeResult));  // Thread safe version of localtime on linux
#endif

   char msbuf[5];
   /* Dividing (without remainder) by 1000 rounds the microseconds
      measure to the nearest millisecond. */
   snprintf(msbuf, 5, ".%3.3ld", millis);

   int datebufCharsRemaining = datebufSize - (int)strlen(datebuf);
#if defined(WIN32) && defined(_M_ARM)
   // There is a bug under ARM with strncat - we use strcat instead - buffer is plenty large accomdate our timestamp, no
   // real need to be safe here anyway.
   strcat(datebuf, msbuf);
#else
   strncat (datebuf, msbuf, datebufCharsRemaining - 1);
#endif
   datebuf[datebufSize - 1] = '\0'; /* Just in case strncat truncated msbuf,
                                       thereby leaving its last character at
                                       the end, instead of a null terminator */

   logString(Data(datebuf), last, false);
}

void
CDRFile::logTimediff(const uint64_t& d, bool last)
{
   logString(Data((UInt64)(d / 1000)), last, false);
}

void
CDRFile::logNumeric(int s, bool last)
{
   logString(Data((Int32)s), last, false);
}


/* ====================================================================
 *
 * Copyright 2017 Daniel Pocock http://danielpocock.com  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 */
