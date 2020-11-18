
#include "rutil/DnsUtil.hxx"
#include "rutil/GenericIPAddress.hxx"

using namespace resip;

EncodeStream&
resip::operator<<(EncodeStream& ostrm, const GenericIPAddress& addr)
{
   ostrm << "[ " ;

#ifdef USE_IPV6
   if (addr.address.sa_family == AF_INET6)
   {
      ostrm << "V6 " << DnsUtil::inet_ntop(addr.v6Address.sin6_addr) << " port=" << ntohs(addr.v6Address.sin6_port);
   }
   else
#endif
   if (addr.address.sa_family == AF_INET)
   {
      ostrm << "V4 " << DnsUtil::inet_ntop(addr.v4Address.sin_addr) << ":" << ntohs(addr.v4Address.sin_port);
   }
   else
   {
      resip_assert(0);
   }

   ostrm << " ]";

   return ostrm;
}

/* ====================================================================
*
* Copyright 2016 Daniel Pocock https://danielpocock.com
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
* 3. Neither the name of the author(s) nor the names of any contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*
* ====================================================================
*
*
*/

