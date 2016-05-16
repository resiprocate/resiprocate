#if !defined(RESIP_COMPAT_HXX)
#define RESIP_COMPAT_HXX 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


/**
   @file
   This file is used to handle compatibility fixes/tweaks so reSIProcate can 
   function on multiple platforms.
*/

#if defined(__INTEL_COMPILER ) && defined( __OPTIMIZE__ )
#  undef __OPTIMIZE__ // weird intel bug with ntohs and htons macros
#endif

//#if defined(HAVE_SYS_INT_TYPES_H)
//#include <sys/int_types.h>
//#endif

#include <cstring>

#ifndef WIN32
#  include <netdb.h>
#  include <sys/types.h>
#  include <sys/time.h>
#  include <sys/socket.h>
#  include <sys/select.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <unistd.h>
#  include <pthread.h>
#  include <limits.h>
#endif

#ifdef WIN32
// !cj! TODO would be nice to remove this 
#  ifndef __GNUC__
#    pragma warning(disable : 4996)
#  endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#  include <windows.h>
#  include <winsock2.h>
#undef WIN32_LEAN_AND_MEAN
#  include <errno.h>
#  include <io.h>
#ifdef UNDER_CE
#include "wince/WceCompat.hxx"
#endif // UNDER_CE

#ifdef _MSC_VER
#include <stdio.h>
#ifndef snprintf
#define snprintf c99_snprintf

inline int c99_vsnprintf(char* str, size_t size, const char* format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

inline int c99_snprintf(char* str, size_t size, const char* format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(str, size, format, ap);
    va_end(ap);

    return count;
}
#endif
#endif // _MSC_VER

#endif

#define RESIP_MAX_SOCKADDR_SIZE 28

#if defined(__APPLE__)
   // .amr. If you get linker or type conflicts around UInt32, then use this define
#  if defined(RESIP_APPLE_USE_SYSTEM_TYPES)
#     include <TargetConditionals.h>
#     include <CoreServices/CoreServices.h>
#  endif
#  if !defined(MAC_OS_X_VERSION_MIN_REQUIRED) || MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_2
      // you don't include the SDK or you're running 10.3 or above
      // note: this will fail on 10.2 if you don't include the SDK
#     include <arpa/nameser_compat.h>
#  else
      // you include the SDK and you're running Mac OS 10.2 or below
      typedef int socklen_t;
#  endif
#  ifdef __MWERKS__ /* this is a <limits.h> bug filed with Apple, Radar# 3657629. */
#    ifndef __SCHAR_MAX__ 
#      define __SCHAR_MAX__ 127
#    endif
#  endif
#endif

#if defined(__SUNPRO_CC)
#  if defined(_TIME_T)
    using std::time_t;
#  endif
#  include <time.h>
#  include <memory.h>
#  include <string.h>
#endif

#if !defined(T_NAPTR)
#  define T_NAPTR 35
#endif

#if !defined(T_SRV)
#  define T_SRV 33
#endif

#if !defined(T_AAAA)
#  define T_AAAA 28
#endif

#if !defined(T_A)
#  define T_A 1
#endif

// Mac OS X: UInt32 definition conflicts with the Mac OS or iPhone OS SDK.
// If you've included either SDK then these will be defined.
// We could also check that __MACTYPES__ is not defined
#if !defined(TARGET_OS_MAC) && !defined(TARGET_OS_IPHONE)
typedef unsigned char  UInt8;
typedef unsigned short UInt16;
typedef unsigned int   UInt32;
typedef char           Int8;
typedef short          Int16;
typedef int            Int32;
#else
// On Apple platforms, MacTypes.h should provide the types:
#include <MacTypes.h>
typedef SInt8          Int8;
typedef SInt16         Int16;
typedef SInt32         Int32;
#endif

#if defined( TARGET_OS_IPHONE )
// TARGET_OS_IPHONE can be 0 or 1, so must also check the value
#if TARGET_OS_IPHONE
#define REQUIRE_SO_NOSIGPIPE
#endif
#endif

#if defined( WIN32 )
  typedef signed __int64   Int64;
  typedef unsigned __int64 UInt64;
#else
  typedef signed long long   Int64;
  typedef unsigned long long UInt64;
#endif
//typedef struct { unsigned char octet[16]; }  UInt128;

namespace resip
{

#if defined(WIN32) || defined(__QNX__)
#ifndef strcasecmp
#  define strcasecmp(a,b)    stricmp(a,b)
#endif
#ifndef strncasecmp
#  define strncasecmp(a,b,c) strnicmp(a,b,c)
#endif
#endif

#if defined(__QNX__) || defined(__sun) || defined(WIN32)
  typedef unsigned int u_int32_t;
#endif

template<typename _Tp>
inline const _Tp&
resipMin(const _Tp& __a, const _Tp& __b)
{
   if (__b < __a) return __b; return __a;
}

template<typename _Tp>
inline const _Tp&
resipMax(const _Tp& __a, const _Tp& __b) 
{
   if (__a < __b) return __b; return __a;
}

template<typename _Tp1, typename _Tp2>
inline const _Tp1
resipIntDiv(const _Tp1& __a, const _Tp2& __b)
{
   // .bwc. Divide-round-nearest without using any floating-point.
   if(__a%__b > __b/2)
   {
      return __a/__b+1;
   }

   return __a/__b;
}

#if defined(WORDS_BIGENDIAN) || defined(_BIG_ENDIAN) || defined( __BIG_ENDIAN__ ) || (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)) || defined(RESIP_BIG_ENDIAN)

inline UInt64
ntoh64(const UInt64 input)
{
   return input;
}

inline UInt64
hton64(const UInt64 input)
{
   return input;
}

#else

inline UInt64
ntoh64(const UInt64 input)
{
   UInt64 rval;
   UInt8 *data = (UInt8 *)&rval;

   data[0] = (UInt8)((input >> 56) & 0xFF);
   data[1] = (UInt8)((input >> 48) & 0xFF);
   data[2] = (UInt8)((input >> 40) & 0xFF);
   data[3] = (UInt8)((input >> 32) & 0xFF);
   data[4] = (UInt8)((input >> 24) & 0xFF);
   data[5] = (UInt8)((input >> 16) & 0xFF);
   data[6] = (UInt8)((input >> 8) & 0xFF);
   data[7] = (UInt8)((input >> 0) & 0xFF);

   return rval;
}

inline UInt64
hton64(const UInt64 input)
{
   return (ntoh64(input));
}

#endif

}

//template "levels; ie REASONABLE and COMPLETE
//reasonable allows most things such as partial template specialization,
//etc...like most compilers and VC++2003+.
//COMPLETE would allow template metaprogramming, template< template< > > tricks,
//etc...REASONABLE should always be defined when COMPLETE is defined.

//#if !defined(__SUNPRO_CC) && !defined(__INTEL_COMPILER)
#if !defined(__INTEL_COMPILER)
#  define REASONABLE_TEMPLATES
#endif

// .bwc. This is the only place we check for USE_IPV6 in a header file. This 
// code has no effect if USE_IPV6 is not set, so this should only kick in when
// we're building the resip libs. If someone sets USE_IPV6 while building
// against the resip libs, no resip header file will care. 
#ifdef USE_IPV6
#ifndef IPPROTO_IPV6
#if(_WIN32_WINNT >= 0x0501)   // Some versions of the windows SDK define IPPROTO_IPV6 differently - always enable IP v6 if USE_IPV6 and _WIN32_WINNT >= 0x0501
#define IPPROTO_IPV6 ::IPPROTO_IPV6  
#else
#ifdef _MSC_VER
#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)"): "
#pragma message (__LOC__ " IPv6 support requested, but IPPROTO_V6 undefined; this platform does not appear to support IPv6 ")
#else
#warning IPv6 support requested, but IPPROTO_IPV6 undefined; this platform does not appear to support IPv6
#endif
// .bwc. Don't do this; someone might have defined it for their own code.
// #undef USE_IPV6
#endif
#endif
#endif

// !bwc! Some poking around seems to indicate that icc supports gcc's function 
// attributes, at least as far back as version 8. I have no idea what support is 
// like prior to that. As for SUNPRO, it uses gcc's frontend, so I would expect 
// gnu-c function attributes to work, but does it define __GNUC__?
#if defined(__GNUC__) || (__INTEL_COMPILER > 800)
#define RESIP_DEPRECATED(x) x __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define RESIP_DEPRECATED(x) __declspec(deprecated) x
#else
#define RESIP_DEPRECATED(x) x
#endif

// These used to live in resipfaststreams.hxx and may only be used there
#if (defined(WIN32) || defined(_WIN32_WCE))

#if (defined(_MSC_VER) && _MSC_VER >= 1400 )
#define SNPRINTF_1(buffer,sizeofBuffer,count,format,var1) _snprintf_s(buffer,sizeofBuffer,_TRUNCATE,format,var1)
#define LTOA(value,string,sizeofstring,radix) _ltoa_s(value,string,sizeofstring,radix)
#define ULTOA(value,string,sizeofstring,radix) _ultoa_s(value,string,sizeofstring,radix)
#define I64TOA(value,string,sizeofstring,radix) _i64toa_s(value,string,sizeofstring,radix)
#define UI64TOA(value,string,sizeofstring,radix) _ui64toa_s(value,string,sizeofstring,radix)
#define GCVT(val,num,buffer,buffersize) _gcvt_s(buffer,buffersize,val,num)
#else
#define _TRUNCATE -1
#define SNPRINTF_1(buffer,sizeofBuffer,count,format,var1) _snprintf(buffer,count,format,var1)
#define LTOA(value,string,sizeofstring,radix) _ltoa(value,string,radix)
#define ULTOA(value,string,sizeofstring,radix) _ultoa(value,string,radix)
#define I64TOA(value,string,sizeofstring,radix) _i64toa(value,string,radix)
#define UI64TOA(value,string,sizeofstring,radix) _ui64toa(value,string,radix)
#define GCVT(val,sigdigits,buffer,buffersize) _gcvt(val,sigdigits,buffer)
#endif

#else //non-windows
#define _TRUNCATE -1
#define SNPRINTF_1(buffer,sizeofBuffer,count,format,var1) snprintf(buffer,sizeofBuffer,format,var1)
#define LTOA(l,buffer,bufferlen,radix) SNPRINTF_1(buffer,bufferlen,bufferlen,"%li",l)
#define ULTOA(ul,buffer,bufferlen,radix) SNPRINTF_1(buffer,bufferlen,bufferlen,"%lu",ul)
#define I64TOA(value,string,sizeofstring,radix) SNPRINTF_1(string,sizeofstring,sizeofstring,"%lli",value)
#define UI64TOA(value,string,sizeofstring,radix) SNPRINTF_1(string,sizeofstring,sizeofstring,"%llu",value)
#define GCVT(f,sigdigits,buffer,bufferlen) SNPRINTF_1(buffer,bufferlen,bufferlen,"%f",f)
#define _CVTBUFSIZE 309+40
#endif

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
 */
