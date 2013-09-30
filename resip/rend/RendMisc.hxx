#if !defined(REND_MISC_HXX)
#define REND_MISC_HXX

#include <string>
#include <stdarg.h>

#include "rutil/compat.hxx"

/**
    2^32/3600/24/365 = 136 years in 32-bits
    2^64/3600/24/265/1000000 = 584 years in 64-bits w micro-sec resolution
    Conclusion: do everything in usec and we will never run out of time
**/

template<typename _Tp>
inline const _Tp&
rendMIN(const _Tp& __a, const _Tp& __b) 
{
   return (__a < __b) ? __a : __b;
}

template<typename _Tp>
inline const _Tp&
rendMAX(const _Tp& __a, const _Tp& __b) 
{
   return (__a < __b) ? __b : __a;
}

// NumberOfElements
#define REND_NELTS(tbl)   ( sizeof(tbl)/sizeof((tbl)[0]) )

#ifndef WIN32
#include <unistd.h>
#define sleepMs sleep
#else
#define sleepMs Sleep
#endif

typedef UInt64 RendTimeUs; // usec
#define REND_S2US(secs) (((UInt64)(secs))*1000000)
#define REND_US2S(usecs) ((usecs)/1000000)
#define REND_S2MS(secs) ((secs)*1000)
#define REND_MS2S(msecs) ((msecs)/1000)

#define REND_MS2US(msecs) (((UInt64)(msecs))*1000)
#define REND_US2MS(usecs) ((usecs)/1000)

typedef unsigned int RendLocalKey;
#define REND_LocalKey_MinVal 1000

extern long RendGetTimeMS();

#define RendGetTimeUsRel RendGetTimeUsRel
extern RendTimeUs RendGetTimeUsRel();	// relative time in micro-sec (us)

extern int RendReadFileIntoData(const char *filename, resip::Data& contents);


struct poptOption;
class RendOptsBase 
{
public:
   RendOptsBase() { }
   virtual ~RendOptsBase();

   virtual struct poptOption* getPoptTbl() = 0;
   virtual const char* getPoptDesc() = 0;
};


extern std::string RendFmtStr(const char *fmt, ...);

// extern int RendIncreaseLimitFds(unsigned int targetFds);

struct RendStatAcc 
{
   RendStatAcc() { clear(); }
   unsigned mCnt;
   double mMin;
   double mMax;
   double mSum;

   void clear() 
   {
      mCnt = 0;
      mMin = mMax = 0.0;
      mSum = 0.0;
   }

   void add(double val) 
   {
      if ( mCnt==0 || val<mMin )	mMin = val;
      if ( mCnt==0 || val>mMax )	mMax = val;
      mSum += val;
      ++mCnt;
   }
   double avg() const 
   {
      return mCnt==0 ? 0.0 : mSum/mCnt;
   }

   char* fmt1(char *buf, size_t buflen, float scale, int prec=3) const;

};

#endif

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
