#ifndef WIN32   

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>

#endif

#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Tuple.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::UTIL

using namespace resip;
using namespace std;

Data 
DnsUtil::getLocalHostName()
{
   char buffer[256];
   if (int e = gethostname(buffer,256) == -1)
   {
       if ( e != 0 )
       {
           int err = getErrno();
           switch (err)
           {
               case WSANOTINITIALISED:
                   CritLog( << "could not find local hostname because netwrok not initialized:" << strerror(err) );
                   break;
               default:
                   CritLog( << "could not find local hostname:" << strerror(err) );
                   break;
           }
           throw Exception("could not find local hostname",__FILE__,__LINE__);
       }
   }
   return buffer;
}


Data 
DnsUtil::getLocalDomainName()
{
#if defined( __MACH__ ) || defined( WIN32 ) || defined(__SUNPRO_CC)
   assert(0);
 // !cj! TODO 
   return NULL;
#else
   char buffer[1024];
   if (int e = getdomainname(buffer,sizeof(buffer)) == -1)
   {
       if ( e != 0 )
       {
           int err = getErrno();
           CritLog(<< "Couldn't find domainname: " << strerror(err));
           throw Exception(strerror(err), __FILE__,__LINE__);
       }
   }
   return buffer;
#endif
}

Data
DnsUtil::getLocalIpAddress(const Data& myInterface)
{
   std::list<std::pair<Data,Data> > ifs = DnsUtil::getInterfaces(myInterface);
   if (ifs.empty())
   {
      throw Exception("No matching interface", __FILE__,__LINE__);
   }
   
   if (ifs.size() > 1)
   {
      assert(0);
   }
  
   return ifs.front().second;
}

Data
DnsUtil::inet_ntop(const Tuple& tuple)
{
#if USE_IPV6
	if (!tuple.isV4())
   {
      const sockaddr_in6& addr = reinterpret_cast<const sockaddr_in6&>(tuple.getSockaddr());
      return DnsUtil::inet_ntop(addr.sin6_addr);
   }
   else
#endif
   {
      const sockaddr_in& addr = reinterpret_cast<const sockaddr_in&>(tuple.getSockaddr());
      return DnsUtil::inet_ntop(addr.sin_addr);
   }
}

Data
DnsUtil::inet_ntop(const struct in_addr& addr)
{
	  char str[256];
#if !defined(WIN32)
    ::inet_ntop(AF_INET, (u_int32_t*)(&addr), str, sizeof(str));
#else
   // !cj! TODO 
	u_int32_t i = *((u_int32_t*)(&addr));
	sprintf(str,"%d.%d.%d.%d\0",(i>>0)&0xFF,(i>>8)&0xFF,(i>>16)&0xFF,(i>>24)&0xFF);
#endif
	return Data(str);
}

Data
DnsUtil::inet_ntop(const struct in6_addr& addr)
{
	 char str[256];
#if !defined(WIN32)
    ::inet_ntop(AF_INET6, (u_int32_t*)(&addr), str, sizeof(str));
#else
	 u_int32_t s[8];
	 u_int32_t* p = (u_int32_t*)(&addr);
	 for (int i=0;i<8;i++)
	 {
		 s[i] = *(p++);
	 }
	 sprintf(str,"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",s[7],s[6],s[5],s[4],s[3],s[2],s[1],s[0]);
#endif
	   return Data(str);
}

Data
DnsUtil::inet_ntop(const struct sockaddr& addr)
{
#ifdef USE_IPV6
	if (addr.sa_family == AF_INET6)
   {
      const struct sockaddr_in6* addr6 = reinterpret_cast<const sockaddr_in6*>(&addr);
      return DnsUtil::inet_ntop(addr6->sin6_addr);
   }
   else
#endif
   {
      const struct sockaddr_in* addr4 = reinterpret_cast<const sockaddr_in*>(&addr);
      return DnsUtil::inet_ntop(addr4->sin_addr);
   }
}

int
DnsUtil::inet_pton(const Data& printableIp, struct in_addr& dst)
{
#if !defined(WIN32)
   return ::inet_pton(AF_INET, printableIp.c_str(), &dst);
#else
   // !cj! TODO
	unsigned long i = inet_addr(printableIp.c_str());
	if ( i == INADDR_NONE )
	{
		return 0;
	}
	u_int32_t* d = reinterpret_cast<u_int32_t*>( &dst );
    *d = i;
   return 1;
#endif   
}

int
DnsUtil::inet_pton(const Data& printableIp, struct in6_addr& dst)
{
#if !defined(WIN32)
   return ::inet_pton(AF_INET6, printableIp.c_str(), &dst);
#else
   // !cj! TODO
   assert(0);
   return -1;
#endif   
}


bool 
DnsUtil::isIpV4Address(const Data& ipAddress)
{
   // ok, this is fairly monstrous but it works. 
   unsigned int p1,p2,p3,p4;
   int count=0;
   int result = sscanf( ipAddress.c_str(), 
                        "%u.%u.%u.%u%n",
                        &p1, &p2, &p3, &p4, &count );

   if ( (result == 4) && (p1 <= 255) && (p2 <= 255) && (p3 <= 255) && (p4 <= 255) && (count == int(ipAddress.size())) )
   {
      return true;
   }
   else
   {
      return false;
   }
}

// RFC 1884
bool 
DnsUtil::isIpV6Address(const Data& ipAddress)
{
   if (ipAddress.empty())
   {
      return false;
   }

   // first character must be a hex digit
   if (!isxdigit(*ipAddress.data()))
   {
      return false;
   }

   switch (ipAddress.size())
   {
      case 1:
         return false;
      case 2:
         return (*(ipAddress.data()+2) == ':' ||
                 *(ipAddress.data()+1) == ':');
      case 3:
         return (*(ipAddress.data()+3) == ':' ||
                 *(ipAddress.data()+2) == ':' ||
                 *(ipAddress.data()+1) == ':');
      case 4:
         return (*(ipAddress.data()+4) == ':' ||
                 *(ipAddress.data()+3) == ':' ||
                 *(ipAddress.data()+2) == ':' ||
                 *(ipAddress.data()+1) == ':');
      default:
         return (*(ipAddress.data()+5) == ':' ||
                 *(ipAddress.data()+4) == ':' ||
                 *(ipAddress.data()+3) == ':' ||
                 *(ipAddress.data()+2) == ':' ||
                 *(ipAddress.data()+1) == ':');
   }
}

Data
DnsUtil::canonicalizeIpV6Address(const Data& ipV6Address)
{
#ifdef USE_IPV6
	struct in6_addr dst;
   int res = DnsUtil::inet_pton(ipV6Address, dst);
   if (res <= 0)
   {
      WarningLog(<< ipV6Address << " not well formed IPV6 address");
      assert(0);
   }
   return DnsUtil::inet_ntop(dst);
#else
	assert(0);

	return Data::Empty;
#endif
}

bool 
DnsUtil::isIpAddress(const Data& ipAddress)
{
   return isIpV4Address(ipAddress) || isIpV6Address(ipAddress);
}


std::list<std::pair<Data,Data> > 
DnsUtil::getInterfaces(const Data& matching)
{
   std::list<std::pair<Data,Data> > results;
   
#if !defined(WIN32) && !defined(__SUNPRO_CC)
   struct ifconf ifc;
   
   int s = socket( AF_INET, SOCK_DGRAM, 0 );
   const int len = 100 * sizeof(struct ifreq);
   
   char buf[ len ];
   
   ifc.ifc_len = len;
   ifc.ifc_buf = buf;
   
   int e = ioctl(s,SIOCGIFCONF,&ifc);
   char *ptr = buf;
   int tl = ifc.ifc_len;
   int count=0;
  
   int maxRet = 10;
   while ( (tl > 0) && ( count < maxRet) )
   {
      struct ifreq* ifr = (struct ifreq *)ptr;
      
      int si = sizeof(ifr->ifr_name) + sizeof(struct sockaddr);
      tl -= si;
      ptr += si;
      //char* name = ifr->ifr_ifrn.ifrn_name;
      char* name = ifr->ifr_name;
 
      struct ifreq ifr2;
      ifr2 = *ifr;
      
      e = ioctl(s,SIOCGIFADDR,&ifr2);

      struct sockaddr a = ifr2.ifr_addr;
      Data ip = DnsUtil::inet_ntop(a);
      DebugLog (<< "Considering: " << name << " -> " << ip << " flags=" << ifr2.ifr_flags);
      if (matching == Data::Empty || matching == name)
      {
         results.push_back(std::make_pair(name, ip));
      }
   }
#else // !WIN32
   assert(0);
#endif

   return results;
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
