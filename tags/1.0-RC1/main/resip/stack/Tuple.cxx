#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "rutil/compat.hxx"

#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <cassert>

#if !defined (WIN32)
#include <arpa/inet.h>
#include <netinet/in.h>
#if defined(__APPLE__) && !defined(s6_addr16)
#define s6_addr16 __u6_addr.__u6_addr16
#endif
#endif

#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/GenericIPAddress.hxx"
#include "rutil/HashMap.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/Transport.hxx"
#include "resip/stack/Tuple.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DNS

Tuple::Tuple() : 
   transport(0),
   connectionId(0),
   onlyUseExistingConnection(false),
   mTransportType(UNKNOWN_TRANSPORT)
{
   sockaddr_in* addr4 = (sockaddr_in*)&mSockaddr;
   memset(addr4, 0, sizeof(sockaddr_in));
   mSockaddr.sa_family = AF_INET;
}

Tuple::Tuple(const GenericIPAddress& genericAddress, TransportType type, 
             const Data& targetDomain) : 
   transport(0),
   connectionId(0),
   onlyUseExistingConnection(false),
   mTransportType(type),
   mTargetDomain(targetDomain)
{
  if (genericAddress.isVersion4())
  {
     m_anonv4 = genericAddress.v4Address;
  }
  else
#ifdef USE_IPV6
  {
     m_anonv6 = genericAddress.v6Address;
  }
#else
  {
     assert(0);
  }
#endif
}


Tuple::Tuple(const Data& printableAddr, 
             int port,
             IpVersion ipVer,
             TransportType type,
             const Data& targetDomain) :
   transport(0),
   connectionId(0),
   onlyUseExistingConnection(false),
   mTransportType(type),
   mTargetDomain(targetDomain)
{
   if (ipVer == V4)
   {
      memset(&m_anonv4, 0, sizeof(m_anonv4));
      m_anonv4.sin_family = AF_INET;
      m_anonv4.sin_port = htons(port);

      if (printableAddr.empty())
      {
         m_anonv4.sin_addr.s_addr = htonl(INADDR_ANY); 
      }
      else
      {
         DnsUtil::inet_pton( printableAddr, m_anonv4.sin_addr);
      }
   }
   else
   {
#ifdef USE_IPV6
      memset(&m_anonv6, 0, sizeof(m_anonv6));
      m_anonv6.sin6_family = AF_INET6;
      m_anonv6.sin6_port = htons(port);
      if (printableAddr.empty())
      {
         m_anonv6.sin6_addr = in6addr_any;
      }
      else
      {
         DnsUtil::inet_pton( printableAddr, m_anonv6.sin6_addr);
      }
#else
	  assert(0);
#endif
   }
}

Tuple::Tuple(const Data& printableAddr, 
             int port,
             TransportType ptype,
             const Data& targetDomain) : 
   transport(0),
   connectionId(0),
   onlyUseExistingConnection(false),
   mTransportType(ptype),
   mTargetDomain(targetDomain)
{
   if (DnsUtil::isIpV4Address(printableAddr))
   {
      memset(&m_anonv4, 0, sizeof(m_anonv4));
      
      DnsUtil::inet_pton( printableAddr, m_anonv4.sin_addr);
      m_anonv4.sin_family = AF_INET;
      m_anonv4.sin_port = htons(port);
   }
   else
   {
#ifdef USE_IPV6
      memset(&m_anonv6, 0, sizeof(m_anonv6));
      DnsUtil::inet_pton( printableAddr, m_anonv6.sin6_addr);
      m_anonv6.sin6_family = AF_INET6;
      m_anonv6.sin6_port = htons(port);
#else
	  assert(0);
#endif
   }
}

Tuple::Tuple(const in_addr& ipv4,
             int port,
             TransportType ptype,
             const Data& targetDomain)
   : transport(0),
     connectionId(0),
     onlyUseExistingConnection(false),
     mTransportType(ptype),
     mTargetDomain(targetDomain)
{
   memset(&m_anonv4, 0, sizeof(sockaddr_in));
   m_anonv4.sin_addr = ipv4;
   m_anonv4.sin_port = htons(port);
   m_anonv4.sin_family = AF_INET;
}

#ifdef USE_IPV6
Tuple::Tuple(const in6_addr& ipv6,
             int port,
             TransportType ptype,
             const Data& targetDomaina)
   : transport(0),
     connectionId(0),
     onlyUseExistingConnection(false),
     mTransportType(ptype),
     mTargetDomain(targetDomaina)
{
   memset(&m_anonv6, 0, sizeof(sockaddr_in6));
   m_anonv6.sin6_addr = ipv6;
   m_anonv6.sin6_port = htons(port);
   m_anonv6.sin6_family = AF_INET6;
}
#endif

Tuple::Tuple(const struct sockaddr& addr, 
             TransportType ptype,
             const Data& targetDomain) : 
   transport(0),
   connectionId(0),
   onlyUseExistingConnection(false),
   mSockaddr(addr),
   mTransportType(ptype),
   mTargetDomain(targetDomain)
{
   if (addr.sa_family == AF_INET)   
   {
      m_anonv4 = (sockaddr_in&)(addr);
   }
#ifdef USE_IPV6
   else if (addr.sa_family == AF_INET6)
   {
      m_anonv6 = (sockaddr_in6&)(addr);
   }
#endif
   else
   {
      assert(0);
   }
}

Data 
Tuple::presentationFormat() const
{
#ifdef USE_IPV6
   if (isV4())
   {
      return Tuple::inet_ntop(*this);
   }
   else if (IN6_IS_ADDR_V4MAPPED(&m_anonv6.sin6_addr))
   {
      return DnsUtil::inet_ntop(*(reinterpret_cast<const in_addr*>(
                                   (reinterpret_cast<const unsigned char*>(&m_anonv6.sin6_addr) + 12))));
   }
   else
   {
      return Tuple::inet_ntop(*this);
   }
#else
      return Tuple::inet_ntop(*this);
#endif

}

void
Tuple::setPort(int port)
{
   if (mSockaddr.sa_family == AF_INET) // v4   
   {
      m_anonv4.sin_port = htons(port);
   }
   else
   {
#ifdef USE_IPV6
      m_anonv6.sin6_port = htons(port);
#else
	  assert(0);
#endif
   }
}

int 
Tuple::getPort() const
{
   if (mSockaddr.sa_family == AF_INET) // v4   
   {
      return ntohs(m_anonv4.sin_port);
   }
   else
   {
#ifdef USE_IPV6
      return ntohs(m_anonv6.sin6_port);
#else
	  assert(0);
#endif
   }
   
   return -1;
}

bool
Tuple::isAnyInterface() const
{
   if (isV4())
   {
      return m_anonv4.sin_addr.s_addr == htonl(INADDR_ANY); 
   }
#if defined (USE_IPV6)
   else
   {
      return memcmp(&m_anonv6.sin6_addr, &in6addr_any, sizeof(in6_addr)) == 0;
   }
#else
   return false;
#endif
}

bool
Tuple::isLoopback() const
{
   
   if(ipVersion()==V4)
   {
      static Tuple loopbackv4("127.0.0.1",0,UNKNOWN_TRANSPORT);
      return isEqualWithMask(loopbackv4,8,true,true);
   }
   else if (ipVersion()==V6)
   {
#ifdef USE_IPV6
#if defined(__linux__) || defined(__APPLE__) || defined(WIN32)
      return IN6_IS_ADDR_LOOPBACK(&(m_anonv6.sin6_addr)) != 0;
#else
      return ((*(const __uint32_t *)(const void *)(&(m_anonv6.sin6_addr.s6_addr[0])) == 0) && 
             (*(const __uint32_t *)(const void *)(&(m_anonv6.sin6_addr.s6_addr[4])) == 0) && 
             (*(const __uint32_t *)(const void *)(&(m_anonv6.sin6_addr.s6_addr[8])) == 0) && 
             (*(const __uint32_t *)(const void *)(&(m_anonv6.sin6_addr.s6_addr[12])) == ntohl(1)));
#endif
#endif
   }
   else
   {
      assert(0);
   }
   
   return false;
}

bool 
Tuple::isV4() const
{
   return mSockaddr.sa_family == AF_INET;
}

IpVersion 
Tuple::ipVersion() const
{
   return mSockaddr.sa_family == AF_INET ? V4 : V6;
}

socklen_t
Tuple::length() const
{
   if (mSockaddr.sa_family == AF_INET) // v4
   {
      return sizeof(sockaddr_in);
   }
#ifdef USE_IPV6
   else  if (mSockaddr.sa_family == AF_INET6) // v6
   {
      return sizeof(sockaddr_in6);
   }
#endif

   assert(0);
   return 0;
}


bool Tuple::operator==(const Tuple& rhs) const
{
   if (mSockaddr.sa_family == rhs.mSockaddr.sa_family)
   {
      if (mSockaddr.sa_family == AF_INET) // v4
      {
         return (m_anonv4.sin_port == rhs.m_anonv4.sin_port &&
                 mTransportType == rhs.mTransportType &&
                 memcmp(&m_anonv4.sin_addr, &rhs.m_anonv4.sin_addr, sizeof(in_addr)) == 0);
      }
      else // v6
      {
#ifdef USE_IPV6
         return (m_anonv6.sin6_port == rhs.m_anonv6.sin6_port &&
                 mTransportType == rhs.mTransportType &&
                 memcmp(&m_anonv6.sin6_addr, &rhs.m_anonv6.sin6_addr, sizeof(in6_addr)) == 0);
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

bool
Tuple::operator<(const Tuple& rhs) const
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
      int c=memcmp(&m_anonv4.sin_addr,
                   &rhs.m_anonv4.sin_addr,
                   sizeof(in_addr));

      if (c < 0)
      {
         return true;
      }
      else if (c > 0)
      {
         return false;
      }
      else if (m_anonv4.sin_port < rhs.m_anonv4.sin_port)
      {
         return true;
      }
      else
      {
         return false;
      }
   }
#ifdef USE_IPV6
   else if (mSockaddr.sa_family == AF_INET6 &&
            rhs.mSockaddr.sa_family == AF_INET6)
   {
      int c = memcmp(&m_anonv6.sin6_addr,
                     &rhs.m_anonv6.sin6_addr,
                     sizeof(in6_addr));
      if (c < 0)
      {
         return true;
      }
      else if (c > 0)
      {
         return false;
      }
      else if (m_anonv6.sin6_port < rhs.m_anonv6.sin6_port)
      {
         return true;
      }
      else
      {
         return false;
      }
   }
   else if (mSockaddr.sa_family == AF_INET6 &&
            rhs.mSockaddr.sa_family == AF_INET)
   {
      return true;
   }
   else if (mSockaddr.sa_family == AF_INET &&
            rhs.mSockaddr.sa_family == AF_INET6)
   {
      return false;
   }
#endif
   else
   {
      //assert(0);
      return false;
   }
}

std::ostream&
resip::operator<<(std::ostream& ostrm, const Tuple& tuple)
{
   ostrm << "[ " ;
   
#ifdef USE_IPV6
   if (tuple.mSockaddr.sa_family == AF_INET6)
   {
      ostrm << "V6 " << DnsUtil::inet_ntop(tuple.m_anonv6.sin6_addr) << " port=" << tuple.getPort();
   }
   else
#endif
   if (tuple.mSockaddr.sa_family == AF_INET)
   {
      ostrm << "V4 " << Tuple::inet_ntop(tuple) << ":" << tuple.getPort();
   }
   else
   {
      assert(0);
   }

   ostrm << " " << Tuple::toData(tuple.mTransportType);
   ostrm << " target domain=";
   if (tuple.mTargetDomain.empty()) ostrm << "unspecified";
   else ostrm << tuple.mTargetDomain;
   
   if (tuple.transport)
   {
      ostrm << " received on: " << *tuple.transport;
   }
   ostrm << " connectionId=" << tuple.connectionId
         << " ]";
   
   return ostrm;
}

size_t 
Tuple::hash() const
{
   // !dlb! do not include the connection
#ifdef USE_IPV6
   if (mSockaddr.sa_family == AF_INET6)
   {
      const sockaddr_in6& in6 =
         reinterpret_cast<const sockaddr_in6&>(mSockaddr);

      return size_t(Data(Data::Share, (const char *)&in6.sin6_addr.s6_addr, sizeof(in6.sin6_addr.s6_addr)).hash() +
                    5*in6.sin6_port +
                    25*mTransportType);
   }
   else
#endif
   {
      const sockaddr_in& in4 =
         reinterpret_cast<const sockaddr_in&>(mSockaddr);
         
      return size_t(in4.sin_addr.s_addr +
                    5*in4.sin_port +
                    25*mTransportType);
   }    
}

HashValueImp(resip::Tuple, data.hash());

static const Data transportNames[MAX_TRANSPORT] =
{
   Data("UNKNOWN_TRANSPORT"),
   Data("TLS"),
   Data("TCP"),
   Data("UDP"),
   Data("SCTP"),
   Data("DCCP"),
   Data("DTLS")
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

Data
Tuple::inet_ntop(const Tuple& tuple)
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


bool
Tuple::isEqualWithMask(const Tuple& compare, short mask, bool ignorePort, bool ignoreTransport) const
{
   if(ignoreTransport || getType() == compare.getType())  // check if transport type matches
   {
      if (mSockaddr.sa_family == compare.getSockaddr().sa_family && mSockaddr.sa_family == AF_INET) // v4
      {
         sockaddr_in* addr1 = (sockaddr_in*)&mSockaddr;
         sockaddr_in* addr2 = (sockaddr_in*)&compare.getSockaddr();

         return ((ignorePort || addr1->sin_port == addr2->sin_port)  &&
                 (addr1->sin_addr.s_addr & htonl((0xFFFFFFFF << (32 - mask)))) == 
                  (addr2->sin_addr.s_addr & htonl((0xFFFFFFFF << (32 - mask)))));
      }
#ifdef USE_IPV6
      else if (mSockaddr.sa_family == compare.getSockaddr().sa_family && mSockaddr.sa_family == AF_INET6) // v6
      {
         sockaddr_in6* addr1 = (sockaddr_in6*)&mSockaddr;
         sockaddr_in6* addr2 = (sockaddr_in6*)&compare.getSockaddr();

         if(ignorePort || addr1->sin6_port == addr2->sin6_port)
         {
            unsigned long mask6part;
            unsigned long temp;
            bool match=true;
            for(int i = 3; i >= 0; i--)
            {
               if(mask <= 32*i)
               {
                  mask6part = 0;
               }
               else
               {
                  temp = mask - 32*i;
                  if(temp >= 32)
                  {
                     mask6part = 0xffffffff;
                  }
                  else
                  {
                     mask6part = 0xffffffff << (32 - temp);
                  }
               }
#ifdef WIN32
               if((*((unsigned long*)&addr1->sin6_addr.u.Word[i*2]) & htonl(mask6part)) != 
                  (*((unsigned long*)&addr2->sin6_addr.u.Word[i*2]) & htonl(mask6part)))
#elif defined(sun)
               // sun has no s6_addr16
               if((*((unsigned long*)&addr1->sin6_addr._S6_un._S6_u32[i]) & htonl(mask6part)) !=
                  (*((unsigned long*)&addr2->sin6_addr._S6_un._S6_u32[i]) & htonl(mask6part)))
#elif defined(__APPLE__)
               if((*((unsigned long*)&addr1->sin6_addr.__u6_addr.__u6_addr32[i]) & htonl(mask6part)) != 
                  (*((unsigned long*)&addr2->sin6_addr.__u6_addr.__u6_addr32[i]) & htonl(mask6part)))				  
#else
               if((*((unsigned long*)&addr1->sin6_addr.s6_addr16[i*2]) & htonl(mask6part)) != 
                  (*((unsigned long*)&addr2->sin6_addr.s6_addr16[i*2]) & htonl(mask6part)))
#endif
               {
                  match=false;
                  break;
               }
            }
            if(match)
            {
               return true;
            }
         }
      }
#endif
   }
   return false;
}


// special comparitors
bool
Tuple::AnyInterfaceCompare::operator()(const Tuple& lhs,
                                       const Tuple& rhs) const
{
   if (lhs.mTransportType < rhs.mTransportType)
   {
      return true;
   }
   else if (lhs.mTransportType > rhs.mTransportType)
   {
      return false;
   }
   else if (lhs.mSockaddr.sa_family == AF_INET && rhs.mSockaddr.sa_family == AF_INET)
   {
      if (lhs.m_anonv4.sin_port < rhs.m_anonv4.sin_port)
      {
         return true;
      }
      else
      {
         return false;
      }
   }
#ifdef USE_IPV6
   else if (lhs.mSockaddr.sa_family == AF_INET6 &&
            rhs.mSockaddr.sa_family == AF_INET6)
   {
      if (lhs.m_anonv6.sin6_port < rhs.m_anonv6.sin6_port)
      {
         return true;
      }
      else
      {
         return false;
      }
   }
   else if (lhs.mSockaddr.sa_family == AF_INET6 &&
            rhs.mSockaddr.sa_family == AF_INET)
   {
      return true;
   }
   else if (lhs.mSockaddr.sa_family == AF_INET &&
            rhs.mSockaddr.sa_family == AF_INET6)
   {
      return false;
   }
#endif
   else
   {
      return false;
   }
};

bool
Tuple::AnyPortCompare::operator()(const Tuple& lhs,
                                  const Tuple& rhs) const
{
   if (lhs.mTransportType < rhs.mTransportType)
   {
      return true;
   }
   else if (lhs.mTransportType > rhs.mTransportType)
   {
      return false;
   }
   else if (lhs.mSockaddr.sa_family == AF_INET && rhs.mSockaddr.sa_family == AF_INET)
   {
      int c = memcmp(&lhs.m_anonv4.sin_addr,
                     &rhs.m_anonv4.sin_addr,
                     sizeof(in_addr));

      if (c < 0)
      {
         return true;
      }
      else if (c > 0)
      {
         return false;
      }
   }
#ifdef USE_IPV6
   else if (lhs.mSockaddr.sa_family == AF_INET6 &&
            rhs.mSockaddr.sa_family == AF_INET6)
   {
      int c = memcmp(&lhs.m_anonv6.sin6_addr,
                     &rhs.m_anonv6.sin6_addr,
                     sizeof(in6_addr));
      if (c < 0)
      {
         return true;
      }
      else if (c > 0)
      {
         return false;
      }
   }
   else if (lhs.mSockaddr.sa_family == AF_INET6 &&
            rhs.mSockaddr.sa_family == AF_INET)
   {
      return true;
   }
   else if (lhs.mSockaddr.sa_family == AF_INET &&
            rhs.mSockaddr.sa_family == AF_INET6)
   {
      return false;
   }
#endif

   return false;
}

GenericIPAddress 
Tuple::toGenericIPAddress() const
{
   if (isV4())
   {
      return GenericIPAddress(m_anonv4);
   }
   else
#ifdef USE_IPV6
  {
      return GenericIPAddress(m_anonv6);
  }
#else
  {
     assert(0);
     return m_anonv4; //bogus
  }
#endif
}

bool
Tuple::AnyPortAnyInterfaceCompare::operator()(const Tuple& lhs,
                                              const Tuple& rhs) const
{
   if (lhs.mTransportType < rhs.mTransportType)
   {
      return true;
   }
   else if (lhs.mTransportType > rhs.mTransportType)
   {
      return false;
   }
#ifdef USE_IPV6
   else if (lhs.mSockaddr.sa_family == AF_INET6 &&
            rhs.mSockaddr.sa_family == AF_INET)
   {
      return true;
   }
   else if (lhs.mSockaddr.sa_family == AF_INET &&
            rhs.mSockaddr.sa_family == AF_INET6)
   {
      return false;
   }
#endif
   else
   {
      return false;
   }
};

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
