#include <fstream>

#include "rutil/DnsUtil.hxx"
#include "TfmDns.hxx"
#include "rutil/WinLeakCheck.hxx"

#include "rutil/dns/ares/ares.h"
#include "rutil/dns/ares/ares_dns.h"

#if !defined WIN32 && !defined __CYGWIN__
#include <resolv.h>
#endif

using namespace resip;
using namespace std;

unsigned short TfmDns::mTransactionId = 0;

int 
TfmDns::init(const std::vector<GenericIPAddress>& additionalNameservers,
             AfterSocketCreationFuncPtr socketfunc,
             int dnsTimeout, 
             int dnsTries,
             unsigned int features)
{
   return Success;
}

TfmDns::TfmDns()
{
   //setup();
}

TfmDns::~TfmDns()
{
}
      
bool 
TfmDns::requiresProcess()
{
   return true; 
}

void 
TfmDns::buildFdSet(fd_set& read, fd_set& write, int& size)
{
}

void 
TfmDns::process(fd_set& read, fd_set& write)
{
}

void
TfmDns::lookup(const char* target, unsigned short type, ExternalDnsHandler* handler, void* userData)
{
   switch (type)
   {
      case 1:
         lookupA(target, handler, userData);
         break;
      case 33:
         lookupSrv(target, handler, userData);
         break;
      default:
         memset(mResponse, 0, sizeof(mResponse)/sizeof(char));
         handler->handleDnsRaw(ExternalDnsRawResult(ARES_ENOTIMP, mResponse, 0, userData));
         break;
   }
}

void
TfmDns::lookupA(const char* target, ExternalDnsHandler* handler, void* userData)
{
   static list<Host> Empty;
   HostMap::iterator it = mHosts.find(target);

   if (it == mHosts.end())
   {
      makeAResponse(target, Empty);
      handler->handleDnsRaw(ExternalDnsRawResult(ARES_ENOTFOUND, mResponse, mResponseLength, userData));
   }
   else
   {
      makeAResponse(target, it->second);
      handler->handleDnsRaw(ExternalDnsRawResult(mResponse, mResponseLength, userData));
   }
}

void 
TfmDns::lookupSrv(const char* target, ExternalDnsHandler* handler, void* userData)
{
   static list<SRV> Empty;
   SRVMap::iterator it = mSRVs.find(target);

   if (it == mSRVs.end())
   {
      makeSrvResponse(target, Empty);
      handler->handleDnsRaw(ExternalDnsRawResult(ARES_ENOTFOUND, mResponse, mResponseLength, userData));
   }
   else
   {
      makeSrvResponse(target, it->second);
      handler->handleDnsRaw(ExternalDnsRawResult(mResponse, mResponseLength, userData));
   }
}

// Pre-condiiton: total length doesn't exceed 512 bytes and no bad names.
void 
TfmDns::makeSrvResponse(const char* target, const std::list<SRV>& srvs)
{
   memset(mResponse, 0, sizeof(mResponse)/sizeof(char));   
   int nameLen = computeLength(target);
   mResponseLength = (nameLen * (1 + srvs.size())) + HFIXEDSZ + QFIXEDSZ + (RRFIXEDSZ * srvs.size());

   for (list<SRV>::const_iterator it = srvs.begin(); it != srvs.end(); ++it)
   {
      mResponseLength += computeSrvDataLength(it->mTarget);
   }

  int len;
  unsigned char *q;
  const char *p;

  /* Set up the header. */
  q = mResponse;
  DNS_HEADER_SET_QID(q, getNextTransactionId());
  DNS_HEADER_SET_QR(q, 1);
  DNS_HEADER_SET_OPCODE(q, QUERY);
  DNS_HEADER_SET_TC(q, 0);
  DNS_HEADER_SET_RCODE(q, 0);
  DNS_HEADER_SET_QDCOUNT(q, 1);
  DNS_HEADER_SET_ANCOUNT(q, srvs.size());
  DNS_HEADER_SET_NSCOUNT(q, 0);
  DNS_HEADER_SET_ARCOUNT(q, 0);

  /* Start writing out the name after the header. */
  q += HFIXEDSZ;
  const char* name = target;
  while (*name)
  {
     /* Count the number of bytes in this label. */
     len = 0;
     for (p = name; *p && *p != '.'; p++)
     {
        if (*p == '\\' && *(p + 1) != 0)
           p++;
        len++;
     }

     /* Encode the length and copy the data. */
     *q++ = len;
     for (p = name; *p && *p != '.'; p++)
     {
        if (*p == '\\' && *(p + 1) != 0)
           p++;
        *q++ = *p;
     }

     /* Go to the next label and repeat, unless we hit the end. */
     if (!*p)
        break;

     name = p + 1;
  }

  /* Add the zero-length label at the end. */
  *q++ = 0;

  /* Finish off the question with the type and class. */
  DNS_QUESTION_SET_TYPE(q, 33);
  DNS_QUESTION_SET_CLASS(q, C_IN);

  q += QFIXEDSZ;

  // start writing out the answers
  for (list<SRV>::const_iterator it = srvs.begin(); it != srvs.end(); ++it)
  {
     name = target;
     while (*name)
     {
        /* Count the number of bytes in this label. */
        len = 0;
        for (p = name; *p && *p != '.'; p++)
        {
           if (*p == '\\' && *(p + 1) != 0)
              p++;
           len++;
        }

        /* Encode the length and copy the data. */
        *q++ = len;
        for (p = name; *p && *p != '.'; p++)
        {
           if (*p == '\\' && *(p + 1) != 0)
              p++;
           *q++ = *p;
        }

        /* Go to the next label and repeat, unless we hit the end. */
        if (!*p)
           break;

        name = p + 1;
     }

     /* Add the zero-length label at the end. */
     *q++ = 0;

     DNS_RR_SET_TYPE(q, 33);
     DNS_RR_SET_CLASS(q, C_IN);
     DNS_RR_SET_TTL(q, 3600);
     int dataLen = computeSrvDataLength(it->mTarget);
     DNS_RR_SET_LEN(q, dataLen);
     q += RRFIXEDSZ;

     // encode priority, weight, and prot.
     DNS__SET16BIT(q, it->mPriority);
     DNS__SET16BIT(q+2, it->mWeight);
     DNS__SET16BIT(q+4, it->mPort);
     q += 6;

     const char* data = it->mTarget.c_str();
     while (*data)
     {
        /* Count the number of bytes in this label. */
        len = 0;
        for (p = data; *p && *p != '.'; p++)
        {
           if (*p == '\\' && *(p + 1) != 0)
              p++;
           len++;
        }

        /* Encode the length and copy the data. */
        *q++ = len;
        for (p = data; *p && *p != '.'; p++)
        {
           if (*p == '\\' && *(p + 1) != 0)
              p++;
           *q++ = *p;
        }

        /* Go to the next label and repeat, unless we hit the end. */
        if (!*p)
           break;

        data = p + 1;
     }

     /* Add the zero-length label at the end. */
     *q++ = 0;

  }
}

int
TfmDns::computeLength(const Data& name)
{
   const char *p;
   int len = 1;
   for (p = name.c_str(); *p; ++p)
   {
      if (*p == '\\' && *(p + 1) != 0)
      {
         ++p;
      }
      ++len;
   }

   /* If there are n periods in the name, there are n + 1 labels, and
    * thus n + 1 length fields, unless the name is empty or ends with a
    * period.  So add 1 unless name is empty or ends with a period.
    */
   if (name[0] && *(p - 1) != '.')
   {
      ++len;
   }

   return len;
}

void
TfmDns::makeAResponse(const char* target, const std::list<Host>& hosts)
{
   memset(mResponse, 0, sizeof(mResponse)/sizeof(char));   
   int nameLen = computeLength(target);   
   static int addrLength = 4;

   mResponseLength = (nameLen * (1 + hosts.size())) + HFIXEDSZ + QFIXEDSZ + (RRFIXEDSZ * hosts.size()) + hosts.size() * addrLength;

   int len;
   unsigned char *q;
   const char *p;

   /* Set up the header. */
   q = mResponse;
   DNS_HEADER_SET_QID(q, getNextTransactionId());
   DNS_HEADER_SET_QR(q, 1);
   DNS_HEADER_SET_OPCODE(q, QUERY);
   DNS_HEADER_SET_TC(q, 0);
   DNS_HEADER_SET_RCODE(q, 0);
   DNS_HEADER_SET_QDCOUNT(q, 1);
   DNS_HEADER_SET_ANCOUNT(q, hosts.size());
   DNS_HEADER_SET_NSCOUNT(q, 0);
   DNS_HEADER_SET_ARCOUNT(q, 0);

   /* Start writing out the name after the header. */
   q += HFIXEDSZ;
   const char* name = target;
   while (*name)
   {
      /* Count the number of bytes in this label. */
      len = 0;
      for (p = name; *p && *p != '.'; p++)
      {
         if (*p == '\\' && *(p + 1) != 0)
            p++;
         len++;
      }

      /* Encode the length and copy the data. */
      *q++ = len;
      for (p = name; *p && *p != '.'; p++)
      {
         if (*p == '\\' && *(p + 1) != 0)
            p++;
         *q++ = *p;
      }

      /* Go to the next label and repeat, unless we hit the end. */
      if (!*p)
         break;

      name = p + 1;
   }

   /* Add the zero-length label at the end. */
   *q++ = 0;

   /* Finish off the question with the type and class. */
   DNS_QUESTION_SET_TYPE(q, 1);
   DNS_QUESTION_SET_CLASS(q, C_IN);

   q += QFIXEDSZ;

   // start writing out the answers
   for (list<Host>::const_iterator it = hosts.begin(); it != hosts.end(); ++it)
   {
      name = target;
      while (*name)
      {
         /* Count the number of bytes in this label. */
         len = 0;
         for (p = name; *p && *p != '.'; p++)
         {
            if (*p == '\\' && *(p + 1) != 0)
               p++;
            len++;
         }

         /* Encode the length and copy the data. */
         *q++ = len;
         for (p = name; *p && *p != '.'; p++)
         {
            if (*p == '\\' && *(p + 1) != 0)
               p++;
            *q++ = *p;
         }

         /* Go to the next label and repeat, unless we hit the end. */
         if (!*p)
            break;

         name = p + 1;
      }

      /* Add the zero-length label at the end. */
      *q++ = 0;

      DNS_RR_SET_TYPE(q, 1);
      DNS_RR_SET_CLASS(q, C_IN);
      DNS_RR_SET_TTL(q, 3600);
      DNS_RR_SET_LEN(q, addrLength);
      q += RRFIXEDSZ;

      // encode the address.
      in_addr addr;
      DnsUtil::inet_pton(it->mIp, addr);
      memcpy(q, &addr, addrLength);
      q += addrLength;
   }
}

int
TfmDns::computeSrvDataLength(const Data& target)
{   
   return computeLength(target) + 6; // 6 bytes for priority, weight, and port.
}

void
TfmDns::setup()
{
   // add some SRV records
   list<SRV> srvs;
   SRV srv1(0, 10, 5060, "proxy1.localhost");
   SRV srv2(10, 20, 5060, "proxy2.localhost");
   srvs.push_back(srv1);
   srvs.push_back(srv2);
   mSRVs.insert(SRVMap::value_type("_sip._udp.localhost", srvs));
   mSRVs.insert(SRVMap::value_type("_sip._tcp.localhost", srvs));

   list<Host> hosts;
   Host host1("192.168.0.8");   
   hosts.push_back(host1);
   mHosts.insert(HostMap::value_type("proxy1.localhost", hosts));

   hosts.clear();
   Host host2("127.0.0.1");
   hosts.push_back(host2);
   mHosts.insert(HostMap::value_type("proxy2.localhost", hosts));
}

void
TfmDns::addSrvs(const Data& query, const list<SRV>& srvs)
{
   SRVMap::iterator it = mSRVs.find(query);
   if (it == mSRVs.end())
   {
      mSRVs.insert(SRVMap::value_type(query, srvs));
   }
   else
   {
      // no duplication check
      it->second.insert(it->second.end(), srvs.begin(), srvs.end());
   }
}

void
TfmDns::addHosts(const Data& domain, const list<Host>& hosts)
{
   HostMap::iterator it = mHosts.find(domain);
   if (it == mHosts.end())
   {
      mHosts.insert(HostMap::value_type(domain, hosts));
   }
   else
   {
      // no duplication check
      it->second.insert(it->second.end(), hosts.begin(), hosts.end());
   }
}

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
