#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <cassert>

#ifndef WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include "resiprocate/os/Tuple.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/HashMap.hxx"

using namespace resip;

Tuple::Tuple() : 
   v6(false),
   port(0), 
   transportType(UNKNOWN_TRANSPORT), 
   transport(0),
   connection(0)
{
   memset(&ipv4, 0, sizeof(ipv4));
#ifdef USE_IPV6
   memset(&ipv6, 0, sizeof(ipv6));
#endif
}

Tuple::Tuple(const in_addr& pipv4,
             int pport,
             TransportType ptype)
   : v6(false),
     ipv4(pipv4),
     port(pport),
     transportType(ptype),
     transport(0),
     connection(0)
{
}

#ifdef USE_IPV6
Tuple::Tuple(const in6_addr& pipv6,
             int pport,
             TransportType ptype)
   : v6(true),
     ipv6(pipv6),
     port(pport),
     transportType(ptype),
     transport(0),
     connection(0)
{
}
#endif


bool Tuple::operator==(const Tuple& rhs) const
{
   if (v6 && rhs.v6)
   {
#if USE_IPV6
	   return ( (memcmp(&ipv6, &ipv6, sizeof(ipv6)) == 0) &&
               (port == rhs.port) &&
               (transportType == rhs.transportType));
#else
		assert(0);
		return false;
#endif	
   }
   else if (!v6 && !rhs.v6)
   {
      return ( (memcmp(&ipv4, &ipv4, sizeof(ipv4)) == 0) &&
               (port == rhs.port) &&
               (transportType == rhs.transportType));
   }
   else
   {
      return false;
   }
   
   // !dlb! don't include connection 
}

bool Tuple::operator<(const Tuple& rhs) const
{
   int c=0;
   
   if (v6 && rhs.v6)
   {
      c = memcmp(&ipv4, &rhs.ipv4, sizeof(ipv4));
   }
   else if (!v6 && !rhs.v6)
   {
      c = memcmp(&ipv4, &ipv4, sizeof(ipv4));
   }
   else if (v6 && !rhs.v6)
   {
      return true;
   }
   else if (!v6 && rhs.v6)
   {
      return false;
   }
   else
   {
      if (c < 0)
      {
         return true;
      }
      else if (c > 0)
      {
         return false;
      }
      else if (port < rhs.port)
      {
         return true;
      }
      else if (port > rhs.port)
      {
         return false;
      }
      else
      {
         return transportType < rhs.transportType;
      }
   }
   // !jf!
   return false;
}

std::ostream&
resip::operator<<(std::ostream& ostrm, const Tuple& tuple)
{
	ostrm << "[ " ;

#if defined(WIN32) 
//	ostrm   << inet_ntoa(tuple.ipv4);
	
#else	
	char str[256];
    if (tuple.v6)
    {
       ostrm << inet_ntop(AF_INET6, &tuple.ipv6.s6_addr, str, sizeof(str));       
    }
    else
    {
       ostrm << inet_ntop(AF_INET, &tuple.ipv4.s_addr, str, sizeof(str));
    }
#endif	
	
	ostrm  << " , " 
	       << tuple.port
	       << " , "
	       << Tuple::toData(tuple.transportType) 
	       << " ,transport="
	       << tuple.transport 
           << " ,connection=" 
           << tuple.connection
	       << " ]";
	
	return ostrm;
}


#if ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )

size_t 
__gnu_cxx::hash<resip::Tuple>::operator()(const resip::Tuple& tuple) const
{
   // !dlb! do not include the connection
   if (tuple.v6)
   {
      return size_t(tuple.ipv6.s6_addr + 5*tuple.port + 25*tuple.transportType);
   }
   else
   {
      return size_t(tuple.ipv4.s_addr + 5*tuple.port + 25*tuple.transportType);
   }
}

#elif  defined(__INTEL_COMPILER )
size_t 
std::hash_value(const resip::Tuple& tuple) 
{
   // !dlb! do not include the connection
   if (tuple.v6)
   {
      return size_t(tuple.ipv6.s6_addr + 5*tuple.port + 25*tuple.transportType);
   }
   else
   {
      return size_t(tuple.ipv4.s_addr + 5*tuple.port + 25*tuple.transportType);
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
