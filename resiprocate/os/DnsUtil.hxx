#if !defined(RESIP_DNSUTIL_HXX)
#define RESIP_DNSUTIL_HXX

#include <list>
#include "BaseException.hxx"
#include "Data.hxx"


struct in_addr;

namespace resip
{

class Tuple;

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

      static Data getLocalHostName();
      static Data getLocalDomainName();
      
      // wrappers for the not so ubiquitous inet_pton, inet_ntop (e.g. WIN32)
      static Data inet_ntop(const struct in_addr& addr);
      static Data inet_ntop(const struct in6_addr& addr);
      static Data inet_ntop(const struct sockaddr& addr);
      static Data inet_ntop(const Tuple& tuple);

      static int inet_pton(const Data& printableIp, struct in_addr& dst);
      static int inet_pton(const Data& printableIp, struct in6_addr& dst);

      static bool isIpAddress(const Data& ipAddress);
      static bool isIpV4Address(const Data& ipAddress);
      static bool isIpV6Address(const Data& ipAddress);

      // returns pair of interface name, ip address
      static std::list<std::pair<Data,Data> > getInterfaces();

      // XXXX:0:0:0:YYYY:192.168.2.233 => XXXX::::YYYY:192.168.2.233
      // so string (case) comparison will work
      // or something
      static void canonicalizeIpV6Address(Data& ipV6Address);
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
