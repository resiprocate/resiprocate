#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#if defined (HAVE_POPT_H) 
#include <popt.h>
#else
#ifndef WIN32
#warning "will not work very well without libpopt"
#endif
#endif

#include <sys/types.h>
#include <iostream>
#include <memory>

#include <fstream>

#include "rutil/socket.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "resip/stack/DnsInterface.hxx"
#include "rutil/dns/QueryTypes.hxx"
#include "rutil/dns/RROverlay.hxx"
#include "rutil/dns/RRList.hxx"
#include "rutil/dns/RRCache.hxx"
#include "rutil/dns/DnsStub.hxx"

using namespace resip;
using namespace std;

class MyDnsSink : public DnsResultSink
{
   void onDnsResult(const DNSResult<DnsHostRecord>&);
#ifdef USE_IPV6
   void onDnsResult(const DNSResult<DnsAAAARecord>&); 
#endif
   void onDnsResult(const DNSResult<DnsSrvRecord>&);
   void onDnsResult(const DNSResult<DnsNaptrRecord>&) {}
   void onDnsResult(const DNSResult<DnsCnameRecord>&);
};

void MyDnsSink::onDnsResult(const DNSResult<DnsHostRecord>& result)
{
   cout << "A records" << endl;
   cout << "Status: " << result.status << endl;
   cout << "Domain: " << result.domain << endl;
   if (result.status == 0)
   {
      for (vector<DnsHostRecord>::const_iterator it = result.records.begin(); it != result.records.end(); ++it)
      {
         cout << (*it).host() << endl;
      }
   }
   cout << endl;
}

void MyDnsSink::onDnsResult(const DNSResult<DnsCnameRecord>& result)
{
   cout << "CNAME records" << endl;
   cout << "Status: " << result.status << endl;
   cout << "Domain: " << result.domain << endl;
   if (result.status == 0)
   {
      for (vector<DnsCnameRecord>::const_iterator it = result.records.begin(); it != result.records.end(); ++it)
      {
         cout << (*it).cname() << endl;
      }
   }
   cout << endl;
}

void MyDnsSink::onDnsResult(const DNSResult<DnsSrvRecord>& result)
{
   cout << "SRV records" << endl;
   cout << "Status: " << result.status << endl;
   cout << "Domain: " << result.domain << endl;
   if (result.status == 0)
   {
      for (vector<DnsSrvRecord>::const_iterator it = result.records.begin(); it != result.records.end(); ++it)
      {
         cout << "Name: " << (*it).name() << endl;
         cout << "Priority: " << (*it).priority() << endl;
         cout << "Weight: " << (*it).weight() << endl;
         cout << "Port: " << (*it).port() << endl;
         cout << "Target: " << (*it).target() << endl;
      }
   }
   cout << endl;
}


// adopted from DnsUtil.
const int NS_INT16SZ = 2;
const int NS_INADDRSZ = 4;
const int NS_IN6ADDRSZ = 16;

static const char*
MyInet_ntop4(const u_char *src, char *dst, size_t size)
{
   static const char fmt[] = "%u.%u.%u.%u";
#ifdef WIN32
   if ( _snprintf(dst, size, fmt, src[0], src[1], src[2], src[3]) < 0)
#else
   if ( snprintf(dst, size, fmt, src[0], src[1], src[2], src[3]) < 0)
#endif
   {
      errno = ENOSPC;
      dst[size-1] = 0;
      return NULL;
   }
   return (dst);
}

static const char *
MyInet_ntop6(const u_char *src, char *dst, size_t size)
{
   /*
    * Note that int32_t and int16_t need only be "at least" large enough
    * to contain a value of the specified size.  On some systems, like
    * Crays, there is no such thing as an integer variable with 16 bits.
    * Keep this in mind if you think this function should have been coded
    * to use pointer overlays.  All the world's not a VAX.
    */
   char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
   struct { int base, len; } best, cur;
   u_int words[NS_IN6ADDRSZ / NS_INT16SZ];
   int i;

   /*
    * Preprocess:
    *	Copy the input (bytewise) array into a wordwise array.
    *	Find the longest run of 0x00's in src[] for :: shorthanding.
    */
   memset(words, '\0', sizeof words);
   for (i = 0; i < NS_IN6ADDRSZ; i++)
      words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
   best.base = -1;
   cur.base = -1;
   for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
      if (words[i] == 0) {
         if (cur.base == -1)
            cur.base = i, cur.len = 1;
         else
            cur.len++;
      } else {
         if (cur.base != -1) {
            if (best.base == -1 || cur.len > best.len)
               best = cur;
            cur.base = -1;
         }
      }
   }
   if (cur.base != -1) {
      if (best.base == -1 || cur.len > best.len)
         best = cur;
   }
   if (best.base != -1 && best.len < 2)
      best.base = -1;

   /*
    * Format the result.
    */
   tp = tmp;
   for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
      /* Are we inside the best run of 0x00's? */
      if (best.base != -1 && i >= best.base &&
          i < (best.base + best.len)) {
         if (i == best.base)
            *tp++ = ':';
         continue;
      }
      /* Are we following an initial run of 0x00s or any real hex? */
      if (i != 0)
         *tp++ = ':';
      /* Is this address an encapsulated IPv4? */
      if (i == 6 && best.base == 0 &&
          (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
         if (!MyInet_ntop4(src+12, tp, sizeof tmp - (tp - tmp)))
            return (NULL);
         tp += strlen(tp);
         break;
      }
      tp += sprintf(tp, "%x", words[i]);
   }
   /* Was it a trailing run of 0x00's? */
   if (best.base != -1 && (best.base + best.len) ==
       (NS_IN6ADDRSZ / NS_INT16SZ))
      *tp++ = ':';
   *tp++ = '\0';

   /*
    * Check for overflow, copy, and we're done.
    */
   if ((size_t)(tp - tmp) > size) {
      errno = ENOSPC;
      return (NULL);
   }
   strcpy(dst, tmp);
   return (dst);
}

#ifdef USE_IPV6
void MyDnsSink::onDnsResult(const DNSResult<DnsAAAARecord>& result)
{
   cout << "AAAA records" << endl;
   cout << "Status: " << result.status << endl;
   cout << "Domain: " << result.domain << endl;
   if (result.status == 0)
   {
      for (vector<DnsAAAARecord>::const_iterator it = result.records.begin(); it != result.records.end(); ++it)
      {
         char str[256];
         cout << MyInet_ntop6((const u_char*)&(*it).v6Address(), str, sizeof(str)) << endl;
      }
   }
   cout << endl;
}
#endif


// NOTE: In order to run this test, you need to uncomment out the USE_LOCAL_DNS define in
// ExternalDnsFactory.cxx.
main(int argc, char* argv[])
{
   {
      const char* const key = "yahoo.com";
      MyDnsSink sink;
      DnsStub stub;
      DnsInterface dns(stub);
      stub.lookup<RR_A>(key, Protocol::Sip, &sink);
   }

   {
      const char* const key = "_ldap._tcp.openldap.org";
      MyDnsSink sink;
      DnsStub stub;
      DnsInterface dns(stub);
      stub.lookup<RR_SRV>(key, Protocol::Sip, &sink);
   }

   {
#ifdef USE_IPV6
      const char* const key = "quartz";
      MyDnsSink sink;
      DnsStub stub;
      DnsInterface dns(stub);
      stub.lookup<RR_AAAA>(key, Protocol::Sip, &sink);
#endif
   }

   {
      const char* const key = "www.google.com";
      MyDnsSink sink;
      DnsStub stub;
      DnsInterface dns(stub);
      stub.lookup<RR_CNAME>(key, Protocol::Sip, &sink);
   }

   return 0;
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

