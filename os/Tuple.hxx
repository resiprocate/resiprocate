#if !defined(RESIP_TUPLE_HXX)
#define RESIP_TUPLE_HXX

#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/os/HashMap.hxx"

#include "resiprocate/os/compat.hxx"

#ifdef WIN32

#else
#include <netinet/in.h>
#endif

namespace resip
{

typedef enum 
{
   UNKNOWN_TRANSPORT = 0,
   UDP,
   TCP,
   TLS,
   SCTP,
   DCCP,
   MAX_TRANSPORT
} TransportType;

class Data;
class Transport;

// WARNING!!
// When you change this structure, make sure to update the hash function,
// operator== and operator< to be consistent with the new structure. For
// instance, the Connection* and Transport* change value in the Tuple over
// its lifetime so they must not be included in the hash or comparisons. 

typedef unsigned long ConnectionId;

class Tuple
{
   public:
      Tuple();
      Tuple(const Data& printableAddress, int port, bool ipv4, TransportType type=UNKNOWN_TRANSPORT);
      Tuple(const Data& printableAddress, int port, TransportType type);
      Tuple(const in_addr& pipv4, int pport, TransportType ptype);
      Tuple(const sockaddr& addr, TransportType ptype);
#ifdef USE_IPV6
      Tuple(const in6_addr& pipv6,  int pport, TransportType ptype);
#endif
      
      // convert from a tuple to a sockaddr structure
      const sockaddr& getSockaddr() const { return mSockaddr; }
      sockaddr& getMutableSockaddr() { return mSockaddr; }

      TransportType getType() const { return mTransportType; }
      void setAny(); // set to INADDR_ANY
      void setPort(int port);
      int getPort() const;
      bool isV4() const;
      socklen_t length() const; // of sockaddr
      
      bool operator<(const Tuple& rhs) const;
      bool operator==(const Tuple& rhs) const;
      
      static TransportType toTransport( const Data& );
      static const Data& toData( TransportType );

      Transport* transport;
      ConnectionId connectionId;
      
   private:
      // !ah! needs to big enough for the IPv6 address.
      union {
            sockaddr mSockaddr;
            sockaddr_in m_anonv4;
#ifdef USE_IPV6
			sockaddr_in6 m_anonv6;
#endif
      };
      TransportType mTransportType;

      friend std::ostream& operator<<(std::ostream& strm, const Tuple& tuple);
};

}

#if  defined(__INTEL_COMPILER )

namespace std
{
size_t hash_value(const resip::Tuple& tuple);
}

#elif defined(HASH_MAP_NAMESPACE)

namespace __gnu_cxx
{

struct hash<resip::Tuple>
{
      size_t operator()(const resip::Tuple& addrPort) const;
};
 
}

#endif // hash stuff


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
