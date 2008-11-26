#if !defined(RESIP_ARES_COMPAT_HXX)
#define RESIP_ARES_COMPAT_HXX

// This header hides some of the differences between contrib/ares and c-ares
//
// GOTCHA: This must only be included from .cxx files.  Ideally, it would only
// be included from .cxx files inside this directory, but there is some
// leakage at the moment :(

#if defined(USE_ARES)
#  include "ares.h"
#  include "ares_dns.h"
#  include "ares_private.h"
#elif defined(USE_CARES)
#  include "ares.h"
#  include "ares_version.h"
#else
#  error Must have ARES or C-ARES
#endif

// These are not part of the c-ares API, but are used by this library
#if !defined(DNS__16BIT)
#  define DNS__16BIT(p)             (((p)[0] << 8) | (p)[1])   
#  define DNS__32BIT(p)		    (((p)[0] << 24) | ((p)[1] << 16) | \
				      ((p)[2] << 8) | (p)[3])
#  define DNS_HEADER_QDCOUNT(h)     DNS__16BIT((h) + 4)
#  define DNS_HEADER_ANCOUNT(h)     DNS__16BIT((h) + 6)
#  define DNS_HEADER_NSCOUNT(h)     DNS__16BIT((h) + 8)
#  define DNS_HEADER_ARCOUNT(h)     DNS__16BIT((h) + 10)
#  define DNS_RR_TYPE(r)            DNS__16BIT(r)
#  define DNS_RR_LEN(r)             DNS__16BIT((r) + 8)
#  define DNS_RR_TTL(r)             DNS__32BIT((r) + 4)
#endif

namespace resip
{
  // To avoid #ifdefs on every call to ares_expand_name() and so on, we define
  // the type that is used to return lengths from that function.  This can be
  // int or long.
#if defined(USE_ARES)
  typedef int ares_length_type;
#endif

#if defined(USE_CARES)
  typedef long ares_length_type;
#endif
}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
