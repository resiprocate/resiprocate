#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <cassert>

#if !defined (WIN32)
#include <arpa/inet.h>
#endif

#include "resiprocate/os/Tuple.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/HashMap.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::UTIL

Tuple::Tuple() : 
   transport(0),
   connectionId(0),
   mTransportType(UNKNOWN_TRANSPORT)
{
   sockaddr_in* addr4 = (sockaddr_in*)&mSockaddr;
   memset(addr4, 0, sizeof(sockaddr_in));
   mSockaddr.sa_family = AF_INET;
}

Tuple::Tuple(const in_addr& ipv4,
             int port,
             TransportType ptype)
   : transport(0),
     connectionId(0),
     mTransportType(ptype)
{
   sockaddr_in* addr4 = (sockaddr_in*)&mSockaddr;
   memset(addr4, 0, sizeof(sockaddr_in));
   addr4->sin_addr = ipv4;
   addr4->sin_port = htons(port);
   addr4->sin_family = AF_INET;
}

#ifdef USE_IPV6
Tuple::Tuple(const in6_addr& ipv6,
             int port,
             TransportType ptype)
   : transport(0),
     connectionId(0),
     mTransportType(ptype)
{
   sockaddr_in6* addr6 = (sockaddr_in6*)&mSockaddr;
   memset(addr6, 0, sizeof(sockaddr_in6));
   addr6->sin6_addr = ipv6;
   addr6->sin6_port = htons(port);
   addr6->sin6_family = AF_INET6;
}
#endif

Tuple::Tuple(const struct sockaddr& addr, TransportType ptype) : 
   transport(0),
   connectionId(0),
   mSockaddr(addr),
   mTransportType(ptype)
{
}

Tuple::Tuple(const Data& printableAddr, int port, TransportType ptype) : 
   transport(0),
   connectionId(0),
   mTransportType(ptype)
{
   if (DnsUtil::isIpV4Address(printableAddr))
   {
      sockaddr_in& addr = reinterpret_cast<sockaddr_in&>(mSockaddr);
      memset(&addr, 0, sizeof(addr));
      
      DnsUtil::inet_pton( printableAddr, addr.sin_addr);
      addr.sin_family = AF_INET;
      addr.sin_port = port;
   }
   else
   {
      sockaddr_in6& addr = reinterpret_cast<sockaddr_in6&>(mSockaddr);
      memset(&addr, 0, sizeof(addr));
      DnsUtil::inet_pton( printableAddr, addr.sin6_addr);
      addr.sin6_family = AF_INET6;
      addr.sin6_port = port;
   }
}


void
Tuple::setPort(int port)
{
   if (mSockaddr.sa_family == AF_INET) // v4   
   {
      sockaddr_in* addr = (sockaddr_in*)&mSockaddr;      
      addr->sin_port = htons(port);
   }
   else
   {
      sockaddr_in6* addr = (sockaddr_in6*)&mSockaddr;      
      addr->sin6_port = htons(port);
   }
}

int 
Tuple::getPort() const
{
   if (mSockaddr.sa_family == AF_INET) // v4   
   {
      sockaddr_in* addr = (sockaddr_in*)&mSockaddr;      
      return ntohs(addr->sin_port);
   }
   else
   {
      sockaddr_in6* addr = (sockaddr_in6*)&mSockaddr;      
      return ntohs(addr->sin6_port);
   }
}


bool 
Tuple::isV4() const
{
   return mSockaddr.sa_family == AF_INET;
}


bool Tuple::operator==(const Tuple& rhs) const
{
   if (mSockaddr.sa_family == rhs.mSockaddr.sa_family)
   {
      if (mSockaddr.sa_family == AF_INET) // v4
      {
         const sockaddr_in& addr1 = reinterpret_cast<const sockaddr_in&>(mSockaddr);
         const sockaddr_in& addr2 = reinterpret_cast<const sockaddr_in&>(rhs.mSockaddr);
         return (addr1.sin_port == addr2.sin_port &&
                 mTransportType == rhs.mTransportType &&
                 memcmp(&addr1.sin_addr, &addr2.sin_addr, sizeof(in_addr)) == 0);
      }
      else // v6
      {
#if USE_IPV6
         const sockaddr_in6& addr1 = reinterpret_cast<const sockaddr_in6&>(mSockaddr);
         const sockaddr_in6& addr2 = reinterpret_cast<const sockaddr_in6&>(rhs.mSockaddr);
         return (addr1.sin6_port == addr2.sin6_port &&
                 mTransportType == rhs.mTransportType &&
                 memcmp(&addr1.sin6_addr, &addr2.sin6_addr, sizeof(in6_addr)) == 0);
#else
         assert(0);
		return false;
#endif
      }
   }
   else
   {
      return false;
   }

   // !dlb! don't include connection 
}

bool Tuple::operator<(const Tuple& rhs) const
{
   if (mTransportType < rhs.mTransportType)
   {
      return true;
   }
   else if (mTransportType > rhs.mTransportType)
   {
      return false;
   }
   else if (mSockaddr.sa_family == AF_INET && rhs.mSockaddr.sa_family == AF_INET)
   {
      sockaddr_in* addr1 = (sockaddr_in*)&mSockaddr;
      sockaddr_in* addr2 = (sockaddr_in*)&rhs.mSockaddr;
      int c=memcmp(&addr1->sin_addr, &addr2->sin_addr, sizeof(in_addr));

      if (c < 0)
      {
         return true;
      }
      else if (c > 0)
      {
         return false;
      }
      else if (addr1->sin_port < addr2->sin_port)
      {
         return true;
      }
      else
      {
         return false;
      }
   }
   else if (mSockaddr.sa_family == AF_INET6 && rhs.mSockaddr.sa_family == AF_INET6)
   {
      sockaddr_in6* addr1 = (sockaddr_in6*)&mSockaddr;
      sockaddr_in6* addr2 = (sockaddr_in6*)&rhs.mSockaddr;
      int c = memcmp(&addr1->sin6_addr, &addr2->sin6_addr, sizeof(in6_addr));
      if (c < 0)
      {
         return true;
      }
      else if (c > 0)
      {
         return false;
      }
      else if (addr1->sin6_port < addr2->sin6_port)
      {
         return true;
      }
      else
      {
         return false;
      }
   }
   else if (mSockaddr.sa_family == AF_INET6 && rhs.mSockaddr.sa_family == AF_INET)
   {
      return true;
   }
   else if (mSockaddr.sa_family == AF_INET && rhs.mSockaddr.sa_family == AF_INET6)
   {
      return false;
   }
   else
   {
      assert(0);
      return false;
   }
}

std::ostream&
resip::operator<<(std::ostream& ostrm, const Tuple& tuple)
{
	ostrm << "[ " ;

#if defined(WIN32) 
#error "fix this"
#else	
	char str[256];
    if (tuple.mSockaddr.sa_family == AF_INET6)
    {
       sockaddr_in6* addr = (sockaddr_in6*)&tuple.mSockaddr;
       ostrm << inet_ntop(AF_INET6, &(addr->sin6_addr), str, sizeof(str));       
       ostrm << ":" << ntohs(addr->sin6_port);
    }
    else
    {
       sockaddr_in* addr = (sockaddr_in*)&tuple.mSockaddr;
       ostrm << inet_ntop(AF_INET, &(addr->sin_addr), str, sizeof(str));
       ostrm << ":" << ntohs(addr->sin_port);
    }
#endif	
	
	ostrm  << " , " 
	       << Tuple::toData(tuple.mTransportType) 
	       << " ,transport="
	       << tuple.transport 
           << " ,connectionId=" 
           << tuple.connectionId
	       << " ]";
	
	return ostrm;
}


#if ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )

size_t 
__gnu_cxx::hash<resip::Tuple>::operator()(const resip::Tuple& tuple) const
{
   // !dlb! do not include the connection
   const sockaddr& sockaddr = tuple.getSockaddr();
   if (sockaddr.sa_family == AF_INET6)
   {
      const sockaddr_in6& addr = reinterpret_cast<const sockaddr_in6&>(sockaddr);
      return size_t(addr.sin6_addr.s6_addr + 5*addr.sin6_port + 25*tuple.getType());
   }
   else
   {
      const sockaddr_in& addr = reinterpret_cast<const sockaddr_in&>(sockaddr);
      return size_t(addr.sin_addr.s_addr + 5*addr.sin_port + 25*tuple.getType());
   }
}

#elif  defined(__INTEL_COMPILER )
size_t 
std::hash_value(const resip::Tuple& tuple) 
{
   // !dlb! do not include the connection
   const sockaddr& sockaddr = tuple.getSockaddr();
   if (sockaddr.sa_family == AF_INET6)
   {
      const sockaddr_in6& addr = reinterpret_cast<const sockaddr_in6&>(sockaddr);
      return size_t(addr.sin6_addr.s6_addr + 5*addr.sin6_port + 25*tuple.getType());
   }
   else
   {
      const sockaddr_in& addr = reinterpret_cast<const sockaddr_in&>(sockaddr);
      return size_t(addr.sin_addr.s_addr + 5*addr.sin_port + 25*tuple.getType());
   }
}

#endif

static const Data transportNames[MAX_TRANSPORT] =
{
   Data("UNKNOWN_TRANSPORT"),
   Data("UDP"),
   Data("TCP"),
   Data("TLS"),
   Data("SCTP"),
   Data("DCCP")
};

TransportType
Tuple::toTransport(const Data& type)
{
   for (TransportType i = UNKNOWN_TRANSPORT; i < MAX_TRANSPORT; 
        i = static_cast<TransportType>(i + 1))
   {
      if (isEqualNoCase(type, transportNames[i]))
      {
         return i;
      }
   }
   return UNKNOWN_TRANSPORT;
};

const Data&
Tuple::toData(TransportType type)
{
   assert(type >= UNKNOWN_TRANSPORT && type < MAX_TRANSPORT);
   return transportNames[type];
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
