/*
 * compat.h - Compatibility header
 * 
 * Copyright (C) 2002 Isaac W. Foraker (isaac(at)tazthecat.net)
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Author nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Purpose:
 *	Very few compilers are fully standards compliant.  This header is an
 *	attempt to tweak settings on individual compiler environments to make
 *	standard code more portable.
 *
 */

#ifndef _compat_h_compatability_header_
#define _compat_h_compatability_header_

#ifdef __cplusplus // Only work for C++ sources

// Import common standard library functions into the std namespace on Microsoft
// Visual C++ 6 or earlier, but only if not already running with the STLPort
// fix.
#if defined(_MSC_VER) && _MSC_VER <= 1200 && !defined(_STLP_DO_IMPORT_CSTD_FUNCTIONS)
#	include <cstdio>
#	include <cstdlib>
#	include <cstring>
#	include <cctype>
#	include <ctime>
namespace std {
	using ::atoi;	using ::atof;
	using ::isalpha; using ::isdigit; using ::isalnum; using ::isgraph; using ::isspace;
	using ::islower; using ::isupper; using ::isxdigit; using ::ispunct; using ::isprint;
	using ::iscntrl; using ::toupper; using ::tolower;
	using ::sprintf; using ::printf; using ::sscanf; using ::scanf;
	using ::size_t; using ::time_t;
	using ::strlen;	using ::strcmp; using ::strncmp; using ::strcpy; using ::strncpy;
	using ::memset; using ::memcpy; using ::memcmp;
	using ::malloc; using ::free;
	using ::clock; using ::time; using ::asctime; using ::gmtime; using ::localtime;
}
#endif

// Disable some annoying MSVC++ warnings.
#if defined(_MSC_VER)
#	if _MSC_VER <= 1200
// Older version of MSVC do not support throw() declaration
#		pragma warning(disable : 4290)
#	endif
// Turn off warnings for long symbol names caused by the STL
#	pragma warning(disable : 4786)
#endif


#endif // __cplusplus


#endif // _compat_h__compatability_header
