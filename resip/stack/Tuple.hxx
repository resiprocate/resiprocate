#if !defined(RESIP_TUPLE_HXX)
#define RESIP_TUPLE_HXX

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <memory>

#include "rutil/Socket.hxx"
#include "rutil/compat.hxx"

#include "rutil/HashMap.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/HeapInstanceCounter.hxx"
#include "rutil/Data.hxx"

#if defined(WIN32)
#include <Ws2tcpip.h>
#else
#include <netinet/in.h>
#endif

namespace resip
{

struct GenericIPAddress;

// WARNING!!
// When you change this structure, make sure to update the hash function,
// operator== and operator< to be consistent with the new structure. Be
// careful not to include members that change value in the Tuple over
// its lifetime (they must not be included in the hash or comparisons). 

typedef unsigned long FlowKey;
typedef unsigned long TransportKey;

/**
   @ingroup resip_crit
   @brief Represents a network address.

   This includes:
      - IP address
      - port
      - protocol (TransportType)
      - TLS hostname (since this is integral to connection establishment)

   Internally the class is aware of the struct
   sockaddr/sin_addr/sin6addr binary representation of the address.
   The sa_family of struct sockaddr is used to keep track of whether a
   Tuple is representing a an IPv4 or IPv6 address.

   Also included are some comparator classes that can be used for
   containers of Tuple.
*/
class Tuple
{
   public:
      RESIP_HeapCount(Tuple);

      Tuple();

      explicit Tuple(const GenericIPAddress& genericAddress, 
                     TransportType type=UNKNOWN_TRANSPORT, 
                     const Data& targetDomain = Data::Empty);

      Tuple(const Data& printableAddress, 
            int port, 
            IpVersion ipVer, 
            TransportType type=UNKNOWN_TRANSPORT, 
            const Data& targetDomain = Data::Empty,
            const Data& netNs = Data::Empty);

      Tuple(const Data& printableAddress, 
            int port, 
            TransportType type, 
            const Data& targetDomain = Data::Empty,
            const Data& netNs = Data::Empty);

      Tuple(const in_addr& pipv4, 
            int pport,
            TransportType ptype, 
            const Data& targetDomain = Data::Empty,
            const Data& netNs = Data::Empty);

      Tuple(const sockaddr& addr, 
            TransportType ptype, 
            const Data& targetDomain = Data::Empty);

#ifdef IPPROTO_IPV6
      // enable this if the current platform supports IPV6; the USE_IPV6 #define
      // will determine if this c'tor is actually implemented.
      // ?bwc? Is there a more standard preprocessor macro for this?
      // ?bwc? Is there a way we can add something more informative to the 
      // linker error we'll see if we compiled without USE_IPV6, on a platform
      // with IPV6, and someone tries to invoke this c'tor? (ie; "This library
      // was built with IPV6 support disabled")
      Tuple(const in6_addr& pipv6,  
            int pport, 
            TransportType ptype, 
            const Data& targetDomain = Data::Empty,
            const Data& netNs = Data::Empty);
#endif
      
      /// @brief Retrieve a const binary representation of the socket address
      /// for this tuple.
      const sockaddr& getSockaddr() const { return mSockaddr; }

      ///  @brief Retrieve the binary representation of the socket address for
      /// this tuple.
      sockaddr& getMutableSockaddr() { return mSockaddr; }
      ///  @brief Get a copy of the socket address including the interface and
      /// not the port number
      void copySockaddrAnyPort(sockaddr *sa);

      ///  @brief Set the internal binary representation of the socket address
      /// from the GenericIPAddress.
      void setSockaddr(const GenericIPAddress &);

      TransportType getType() const { return mTransportType; }
      void setType(TransportType type) { mTransportType = type; }
      void setPort(int port);
      int getPort() const;
      inline FlowKey getFlowKey() const { return mFlowKey; } 

      /// @deprecated use ipVersion()
      /// @todo !dcm! -- should deprecate asap
      bool isV4() const; 

      /// Returns V4 or V6 as appropriate.
      IpVersion ipVersion() const;

      ///  @brief TRUE if this address is equal to the "INADDR_ANY" value for
      /// this address family.  
      bool isAnyInterface() const;
      socklen_t length() const; // of sockaddr
      bool isLoopback() const;
      bool isPrivateAddress() const;  // Return boolean based on definitions in RFC1918(v4) and RFC4193(v6)
      
      ///  @brief Compares TransportType, the binary address, port, and
      /// address family of the Tuple.
      bool operator<(const Tuple& rhs) const;

      ///  @brief Compares TransportType, the binary address, port, and
      /// address family of the Tuple.
      bool operator==(const Tuple& rhs) const;
      
      /// Wrapper around the inet_top() method.
      Data presentationFormat() const;
      
      ///  @brief Converts a string representation of transport type,
      /// i.e. "UDP" to a TransportType
      static TransportType toTransport( const Data& );

      ///  @brief Converts the TransportType to a string representation of the
      /// transport type, e.g. "TCP"
      static const Data& toData( TransportType );

      static const Data& toDataLower(TransportType type);

      ///  @brief Converts the binary socket address to presentation format,
      /// via the DnsUtil::inet_ntop() method.
      static Data inet_ntop(const Tuple& tuple);

      // Creates a binary token from the provided Tuple - if salt is provided, then an HMAC is appended
      // to the end of the token
      static void writeBinaryToken(const Tuple& tuple, Data& container, const Data& salt=Data::Empty);
      // Creates a Tuple from the provided binary token - if salt is provided, then an HMAC is checked
      static Tuple makeTupleFromBinaryToken(const Data& binaryToken, const Data& salt=Data::Empty);

      GenericIPAddress toGenericIPAddress() const;

      /// This is a (largely) opaque key that subclasses of Transport will use
      /// to help record/find flows. For UDP and DTLS, this is just the FD, and
      /// the rest of the information about the flow is carried in the Tuple.
      /// For TCP and TLS, the FD of the connection is used.
      /// For protocols where using the FD would not be appropriate (SCTP),
      /// the transport may use whatever method to generate these it likes.
      /// (It is highly recommended that these ids are unique across all
      /// instances of a transport type)
      FlowKey mFlowKey;
      TransportKey mTransportKey;

      bool onlyUseExistingConnection;

      ///  @brief compares this tuple with the one passed in for family, port
      /// and address equality using the passed in address mask (mask
      /// is specified by number of bits)
      bool isEqualWithMask(const Tuple& tuple, short mask, bool ignorePort=false, bool ignoreTransport=false) const;

      ///  @brief A "less than" comparator for Tuple, for use in map
      /// containers etc. Comparison is based on transport type, and
      /// if those are equal, it is based on port number.
      class AnyInterfaceCompare
      {
         public:
            bool operator()(const Tuple& x,
                            const Tuple& y) const;
      };
      friend class AnyInterfaceCompare;

      ///  @brief A "less than" comparator for Tuple, for use in map
      /// containers etc. Comparison is based on transport type, and
      /// if those are equal, it is based on the binary representation
      /// of the socket internet address (v4 or v6, whichever is
      /// appropriate).
      class AnyPortCompare
      {
         public:
            bool operator()(const Tuple& x,
                            const Tuple& y) const;
      };
      friend class AnyPortCompare;

      ///  @brief A "less than" comparator for Tuple, for use in map
      /// containers etc. Comparison is based only on transport type
      class AnyPortAnyInterfaceCompare
      {
         public:
            bool operator()(const Tuple& x,
                            const Tuple& y) const;
      };
      friend class AnyPortAnyInterfaceCompare;

      class FlowKeyCompare
      {
         public:
            bool operator()(const Tuple& x,
                            const Tuple& y) const;
      };
      friend class FlowKeyCompare;

      ///  @brief Set the domain name this address tuple intends to represent.
      void setTargetDomain(const Data& target)
      {
         mTargetDomain = target;
      }
      
      ///  @brief Get the domain name this address tuple intends to represent.
      /// Useful with DnsUtil, for example.
      const Data& getTargetDomain() const
      {
         return mTargetDomain;
      }

      /// @brief Set the netns (network namespace) for this Tuple
      void setNetNs(const Data& netNs)
      {
          mNetNs = netNs;
      }

      /// @brief Get the netns for this Tuple
      const Data& getNetNs() const
      {
          return(mNetNs);
      }

      /**
         @brief Creates a 32-bit hash based on the contents of this Tuple.
      */
      size_t hash() const;   

private:
      union 
      {
            sockaddr mSockaddr;
            sockaddr_in m_anonv4;
#ifdef IPPROTO_IPV6
            // enable this if the current platform supports IPV6
            // ?bwc? Is there a more standard preprocessor macro for this?
            sockaddr_in6 m_anonv6;
#endif
            char pad[RESIP_MAX_SOCKADDR_SIZE]; //< this make union same size if v6 is in or out
      };
      TransportType mTransportType;
      Data mTargetDomain; 

      Data mNetNs;  ///< The network namespace to which the address and port are scoped

      friend EncodeStream& operator<<(EncodeStream& strm, const Tuple& tuple);
      friend class DnsResult;
};


EncodeStream&
operator<<(EncodeStream& ostrm, const Tuple& tuple);

}

HashValue(resip::Tuple);

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
