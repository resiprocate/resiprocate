//#include <sys/socket.h>
//#include <arpa/inet.h>
//#include <netdb.h>
//#include <sys/types.h>
//#include <stdio.h>
//#include <errno.h>
#include <iostream>

#include "Resolver.hxx"
#include "DnsUtils.hxx"
#include "rutil/DnsUtil.hxx"

using namespace resip;
using namespace std;


SourceSet 
DnsUtils::makeSourceSet(const Data& dnsName, int port, resip::TransportType transport)
{
   SourceSet s;
   s.insert(Source(dnsName, port, transport));
   if (!Resolver::isIpAddress(dnsName))
   {
      Resolver r(dnsName, port, transport);
      for (list<Tuple>::const_iterator it = r.mNextHops.begin();
           it != r.mNextHops.end(); it++)
      {
         Source x;
         x.host = resip::Tuple::inet_ntop(*it);
         x.port = it->getPort();
         x.transportType = it->getType();
         s.insert(x);
      }
   }
   return s;
}

/*
  Copyright (c) 2005, PurpleComm, Inc. 
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of PurpleComm, Inc. nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
