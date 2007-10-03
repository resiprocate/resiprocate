#if !defined(RESIP_DNSUTIL_HXX)
#define RESIP_DNSUTIL_HXX

#include "rutil/Socket.hxx"
#include <list>
#include "BaseException.hxx"
#include "Data.hxx"


struct in_addr;
struct in6_addr;
namespace resip
{

class Tuple;

/** DnsUtil provides a collection of utility functions for
  * manipulating DNS names and IP addresses and discovering
  * details about the local interfaces
  */

class DnsUtil
{
   public:
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg,
                      const Data& file,
                      const int line)
               : BaseException(msg, file, line) {}            
         protected:
            virtual const char* name() const { return "DnsUtil::Exception"; }
      };

      /** @returns the fully qualified local host name as the host
       *   currently knows it ala gethostname(3) 
       */

      static Data getLocalHostName();

      /** @returns the suffix after the first "." in whatever getLocalHostName returns */
      static Data getLocalDomainName();

      /** Gets the IP address of "the" local interface. Note that this will not work
       *  on systems with more than one ethernet interface.
       *  @deprecated Very few real systems have only one interface (loopback plus
       *              physical, vpns, wireless and wired, etc.) Use getInterfaces
       *              instead.
       */

      static Data getLocalIpAddress(const Data& defaultInterface=Data::Empty) ;

      // returns pair of interface name, ip address
      /** @returns a list of interface name, IP addresses pairs of all available 
       *           interfaces (note that some of these may be v4 and others v6.
       */
      static std::list<std::pair<Data,Data> > getInterfaces(const Data& matchingInterface=Data::Empty);

      // wrappers for the not so ubiquitous inet_pton, inet_ntop (e.g. WIN32)
      static Data inet_ntop(const struct in_addr& addr);
      static Data inet_ntop(const struct sockaddr& addr);
      static int inet_pton(const Data& printableIp, struct in_addr& dst);
      
      static bool isIpAddress(const Data& ipAddress);
      static bool isIpV4Address(const Data& ipAddress);
      static bool isIpV6Address(const Data& ipAddress);

#ifdef USE_IPV6
      static Data inet_ntop(const struct in6_addr& addr);
      static int inet_pton(const Data& printableIp, struct in6_addr& dst);
#endif

      //pass-throughs when supported, actual implemenation in the WIN32 case
      static const char * inet_ntop(int af, const void* src, char* dst, size_t size);      
      static int inet_pton(int af, const char * src, void * dst);
      

      // so string (case) comparison will work
      // or something
      /** Converts the printable form of an IPv6 address into
       *  consistent format so that string comparisons will do
       *  what you expect.
       *  For instance, XXXX:0:0:0:YYYY:192.168.2.233 will be
       *  converted to  XXXX::::YYYY:192.168.2.233.
       *  Currently, this is realized by (inet_ntop(inet_pton(input)).
       */
      static Data canonicalizeIpV6Address(const Data& ipV6Address);

      /// Used to synchronously query A records - only for test code usage
      static std::list<Data> lookupARecords(const Data& host);

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
