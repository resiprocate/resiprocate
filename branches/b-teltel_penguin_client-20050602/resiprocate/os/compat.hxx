#if !defined(RESIP_COMPAT_HXX)
#define RESIP_COMPAT_HXX 

#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#if defined(__INTEL_COMPILER ) && defined( __OPTIMIZE__ )
#undef __OPTIMIZE__ // wierd intel bug with ntohs and htons macros
#endif

#if defined(HAVE_SYS_INT_TYPES_H)
#include <sys/int_types.h>
#endif

#ifdef _WIN32
// !cj! TODO would be nice to remove this 
#pragma warning(disable : 4996)
#pragma warning(disable : 4049)
#pragma warning(disable : 4217)
#endif
 
#ifndef _WIN32
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#ifdef _WIN32
# include <winsock2.h>
# include <windows.h>
# include <winbase.h>
# ifndef _WIN32_WCE
# include <errno.h>
# include <io.h>
# endif

#ifndef __BIT_TYPES_DEFINED__ /* sleepcat DB uses this */ 
typedef unsigned long int u_int32_t;
typedef long int ssize_t;
#endif // #ifdef _WIN32

#endif // #ifdef _WIN32

#if defined(TARGET_OS_MAC) /* TARGET_OS_MAC #defined in OS X SDK, "TargetConditionals.h" */
# include <netdb.h>
# include <arpa/nameser_compat.h>
# ifdef __MWERKS__ /* this is a <limits.h> bug filed with Apple, Radar# 3657629. */
#  ifndef __SCHAR_MAX__ 
#   define __SCHAR_MAX__ 127
#  endif // #  ifndef __SCHAR_MAX__ 
# endif // # ifdef __MWERKS__ 
#endif // #if defined(TARGET_OS_MAC)

#if defined(__SUNPRO_CC)
# if defined(_TIME_T)
 using std::time_t;
# endif // # if defined(_TIME_T)
#include <time.h>
#include <memory.h>
#include <string.h>
#endif // #if defined(__SUNPRO_CC)

#include <cstring>

#if defined(_WIN32) || defined(__QNX__)
# ifdef _WIN32_WCE
#  define strcasecmp(a,b) _stricmp(a,b)
#  define strncasecmp(a,b,c) _strnicmp(a,b,c)
# else
#  define strcasecmp(a,b) stricmp(a,b)
#  define strncasecmp(a,b,c) strnicmp(a,b,c)
# endif // # ifdef _WIN32_WCE
#endif

#ifndef _WIN32
#include <pthread.h>
#endif

#if defined(__QNX__) || defined(__sun)
typedef unsigned int u_int32_t;
#endif

#if !defined(T_NAPTR)
#define T_NAPTR 35
#endif

#if !defined(T_SRV)
#define T_SRV 33
#endif

#if !defined(T_AAAA)
#define T_AAAA 28
#endif

namespace resip
{

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

#if defined(__DARWIN__)
typedef size_t socklen_t;
#endif
}

#if defined( _WIN32 )
typedef __int64          Int64;
typedef unsigned __int64 UInt64;
#else
typedef long long        Int64;
typedef unsigned long long UInt64;
#endif


//template "levels; ie REASONABLE and COMPLETE
//reasonable allows most things such as partial template specialization,
//etc...like most compilers and VC++2003+.
//COMPLETE would allow template metaprogramming, template< template< > > tricks,
//etc...REASONABLE should always be defined when COMPLETE is defined.


#if defined(_MSC_VER) && (MSC_VER >= 1200)
#define REASONABLE_TEMPLATES
#endif
#if !defined(__SUNPRO_CC) && !defined(__INTEL_COMPILER)
#define REASONABLE_TEMPLATES
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
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
