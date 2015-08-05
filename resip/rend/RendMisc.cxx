/**
**/

#include <fstream>

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include "rutil/ResipAssert.h"
#include <string.h>
#include <time.h>
#ifndef WIN32
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include "rutil/Logger.hxx"
#include "rutil/Time.hxx"

#include "RendMisc.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::APP

RendTimeUs
RendGetTimeUsRel() 
{
   // Note:  _RESIP_MONOTONIC_CLOCK should be defined to be immune from system clock changes
   return resip::ResipClock::getTimeMicroSec();
}

RendOptsBase::~RendOptsBase() 
{ 
}

std::string
RendFmtStr(const char *fmt, ...) 
{
   va_list args;
   char buf[1000];
   va_start(args,fmt);
   vsnprintf(buf, sizeof(buf), fmt, args);
   va_end(args);
   return std::string(buf);
}

#if 0
/**
    Returns negative on error, or number of (positive) allowed fds
**/
int
RendIncreaseLimitFds(unsigned int targetFds) 
{
   struct rlimit lim;

   if ( getrlimit(RLIMIT_NOFILE, &lim)<0 ) 
   {
      CritLog(<<"getrlimit(NOFILE) failed: " << strerror(errno));
      return -1;
   }
   if ( lim.rlim_cur==RLIM_INFINITY || targetFds < lim.rlim_cur )
      return targetFds;
   if ( lim.rlim_max==RLIM_INFINITY || targetFds < lim.rlim_max ) 
   {
      lim.rlim_cur=targetFds;
   } 
   else 
   {
      if ( geteuid()!=0 ) 
      {
         CritLog(<<"Attempting to increase number of fds when not root. This probably wont work");
      }
      lim.rlim_cur=targetFds;
      lim.rlim_max=targetFds;
   }
   if ( setrlimit(RLIMIT_NOFILE, &lim)<0 ) 
   {
      CritLog(<<"setrlimit(NOFILE)=("<<lim.rlim_cur<<","<<lim.rlim_max
         <<")) failed: " << strerror(errno));
      /* There is intermediate: could raise cur to max */
      return -1;
   }
   return targetFds;
}
#endif

char*
RendStatAcc::fmt1(char *buf, size_t buflen, float scale, int prec) const 
{
#ifdef WIN32
   _snprintf(buf, buflen, "[%d:%.*f/%.*f/%.*f]",
      mCnt, prec, mMin*scale, prec, avg()*scale, prec, mMax*scale);
#else
   snprintf(buf, buflen, "[%d:%.*f/%.*f/%.*f]",
      mCnt, prec, mMin*scale, prec, avg()*scale, prec, mMax*scale);
#endif
   return buf;
}



// modified version of resip/stack/ssl/Security.cxx
// Returns negative on error
int
RendReadFileIntoData(const char *filename, resip::Data& contents)
{
   std::ifstream is;
   is.open(filename, std::ios::binary );
   if ( !is.is_open() )
   {
      ErrLog( << "Could not open file " << filename << " for read");
      return -1;
   }
   resip_assert(is.is_open());
   int length = 0;
   is.seekg(0, std::ios::end);
   length = (int)is.tellg();
   is.seekg(0, std::ios::beg);

   // tellg/tell will return -1 if the stream is bad
   if (length == -1)
   {
      ErrLog( << "Could not seek into file " << filename);
      return -2;
   }
   if ( length==0 ) 
   {
      contents.clear();
      return 0;
   }

   char* buffer = new char [length+1]; 
   is.read(buffer,length);
   buffer[length] = 0;

   contents.setBuf(resip::Data::Take, buffer, length);
   is.close();

   return length;
}

/* ====================================================================

 Copyright (c) 2011, Logitech, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Logitech nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
