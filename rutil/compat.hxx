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

#ifndef __cplusplus
#error Expecting __cplusplus (mandatory in for all C++ compilers)
#endif

/* Introduction of cstdint types from C++11
 *
 * Traditionally, reSIProcate has used custom types UInt8, UInt32, ...
 * For the v1.9.x release series, reSIProcate supports both the
 * custom types and the <cstdint> style.  A future release of
 * reSIProcate may deprecate and eventually remove UInt8, UInt32 and friends.
 *
 * C++11 is not required, compat.hxx aims to correctly typedef the
 * expected <cstdint> types on platforms where they don't exist.
 * rutil/test/testCompat.cxx will even test the types using sizeof() to
 * make sure they are always declared correctly.
 *
 *** Known issues ***
 *
 * One side-effect of this change is that code in some environments,
 * unsigned long may be the same as uint32_t or uint64_t.
 * In these cases, overloaded methods will appear to have identical
 * prototypes and the compile will fail with an error.  It is believed
 * that all such issues have been eliminated from the stack itself
 * but such errors may be encountered while re-compiling application
 * code against v1.9 or later.
 *
 * If a build fails like this:
 * resipfaststreams.hxx: In member function 'resip::ResipFastOStream& resip::ResipFastOStream::operator<<(int32_t)':
 * resipfaststreams.hxx:198:30: error: expected ')' before 'PRIi32'
 * resipfaststreams.hxx:198:38: warning: spurious trailing '%' in format [-Wformat]
 * resipfaststreams.hxx:198:38: warning: too many arguments for format [-Wformat-extra-args]
 * resipfaststreams.hxx:198:38: warning: spurious trailing '%' in format [-Wformat]
 * resipfaststreams.hxx:198:38: warning: too many arguments for format [-Wformat-extra-args]
 *
 * it is a sign that some header included inttypes.h without declaring
 * __STDC_FORMAT_MACROS.  You can define __STDC_FORMAT_MACROS in CPPFLAGS
 * or just make sure you always include rutil/compat.hxx ahead of other headers
 * and it will define __STDC_FORMAT_MACROS when required.
 *
 */
#if __cplusplus >= 201103L
#include <cinttypes>
#include <cstdint>
using std::PRId32;
using std::PRIu32;
using std::PRId64;
using std::PRIu64;
using std::uint8_t;
using std::uint8_t;
using std::uint32_t;
using std::uint64_t;
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
#if defined(HAVE_INTTYPES_H)
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#else
#error inttypes is required
#endif
#elif defined(TARGET_OS_MAC) || defined(TARGET_OS_IPHONE)
typedef UInt8 uint8_t;
typedef UInt16 uint16_t;
typedef UInt32 uint32_t;
typedef unsigned long long uint64_t;
#elif defined(__QNX__) || defined(__sun)
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
#elif defined(WIN32)
#include "rutil/msvc/stdint.h"
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include "rutil/msvc/inttypes.h"
#else
#error Failed to find or typedef fixed-size types, please add your platform to rutil/compat.hxx
#endif

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
#define WIN32_LEAN_AND_MEAN
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
#if !defined(TARGET_OS_MAC) && !defined(TARGET_OS_IPHONE)
typedef uint8_t    UInt8;
typedef uint16_t   UInt16;
typedef uint32_t   UInt32;
typedef int32_t    Int32;
#endif

typedef uint64_t UInt64;

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
  // FIXME: look for all uses of u_int32_t and remove them
  typedef uint32_t u_int32_t;
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
