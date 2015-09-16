#if !defined(RESIP_GENERIC_IP_ADDRESS_HXX)
#define RESIP_GENERIC_IP_ADDRESS_HXX

#ifndef WIN32
#include <netinet/in.h>
#else
#include <winsock2.h>
#include <Ws2tcpip.h>
#endif

#include "rutil/Socket.hxx"
#include "rutil/compat.hxx"


namespace resip
{
/** 
   @brief Represents an IP-address and port (V4 or V6).

   @note This class is misnamed - it is really an IP address and port 
*/
struct GenericIPAddress
{
   public:   
      GenericIPAddress()
      {
      }
      
      GenericIPAddress(const sockaddr& addr) : address(addr) 
      {
#ifdef IPPROTO_IPV6
         if (addr.sa_family == AF_INET6)
         {
            v6Address = reinterpret_cast<const sockaddr_in6&>(addr);
         }
         else
#endif
         {
            v4Address = reinterpret_cast<const sockaddr_in&>(addr);
         }
      }

      GenericIPAddress(const sockaddr_in& v4) : v4Address(v4)
      {
      }

#ifdef IPPROTO_IPV6
      GenericIPAddress(const sockaddr_in6& v6) : v6Address(v6)
      {
      }
#endif
      
      size_t length() const
      {
         if (address.sa_family == AF_INET) // v4
         {
            return sizeof(sockaddr_in);
         }
#ifdef IPPROTO_IPV6
         else  if (address.sa_family == AF_INET6) // v6
         {
            return sizeof(sockaddr_in6);
         }
#endif
         resip_assert(0);
         return 0;
      }
      
      bool isVersion4() const
      {
         return address.sa_family == AF_INET;
      }

      bool isVersion6() const 
      { 
#ifdef IPPROTO_IPV6
         if (address.sa_family == AF_INET6) return true; 
#endif
         return false;
      }

      union
      {
            sockaddr address;
            sockaddr_in v4Address;
#ifdef IPPROTO_IPV6
            sockaddr_in6 v6Address;
#endif
            char pad[28]; // this make union same size if v6 is in or out
      };

      bool operator==(const GenericIPAddress& addr) const
      {
         if (address.sa_family == addr.address.sa_family)
         {
            if (address.sa_family == AF_INET) // v4
            {
               return (v4Address.sin_port == addr.v4Address.sin_port &&
                     memcmp(&v4Address.sin_addr, &addr.v4Address.sin_addr, sizeof(in_addr)) == 0);
            }
            else // v6
            {
#ifdef IPPROTO_IPV6
               return (v6Address.sin6_port == addr.v6Address.sin6_port &&
                     memcmp(&v6Address.sin6_addr, &addr.v6Address.sin6_addr, sizeof(in6_addr)) == 0);
#else
               resip_assert(0);
		         return false;
#endif
            }
         }
         return false;
      }

      bool operator<(const GenericIPAddress& addr) const
      {
    
         if (address.sa_family == AF_INET && addr.address.sa_family == AF_INET)
         {
            int c=memcmp(&v4Address.sin_addr,
                        &addr.v4Address.sin_addr,
                        sizeof(in_addr));

            if (c < 0)
            {
               return true;
            }
            else if (c > 0)
            {
               return false;
            }
            else if (v4Address.sin_port < addr.v4Address.sin_port)
            {
               return true;
            }
            else
            {
               return false;
            }
         }
#ifdef IPPROTO_IPV6
         else if (address.sa_family == AF_INET6 &&
                  addr.address.sa_family == AF_INET6)
         {
            int c = memcmp(&v6Address.sin6_addr,
                           &addr.v6Address.sin6_addr,
                           sizeof(in6_addr));
            if (c < 0)
            {
               return true;
            }
            else if (c > 0)
            {
               return false;
            }
            else if (v6Address.sin6_port < addr.v6Address.sin6_port)
            {
               return true;
            }
            else
            {
               return false;
            }
         }
         else if (address.sa_family == AF_INET6 &&
                  addr.address.sa_family == AF_INET)
         {
            return true;
         }
         else if (address.sa_family == AF_INET &&
                  addr.address.sa_family == AF_INET6)
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

};

}


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
