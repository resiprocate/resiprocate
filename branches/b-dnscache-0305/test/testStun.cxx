#include <iostream>

#ifndef WIN32
#include <netdb.h>
#endif

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Fifo.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/Stun.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

int
main(int argc, char* argv[])
{
   if (argc !=2 )
   {
      cerr << "Usage:" << endl

           << "    ./test stunServerHostname" << endl;
      return 0;
   }

   Log::Level l = Log::Warning;
   //Log::Level l = Log::Info;
   Log::initialize(Log::Cout, l, argv[0]);

   initNetwork();


   struct hostent* host;
   host = gethostbyname(argv[1]);

   in_addr sin_addr;
   unsigned int ip;

   if (host)
   {
      sin_addr = *(struct in_addr*)host->h_addr;
      ip = ntohl(sin_addr.s_addr);
      Socket fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
      assert(fd!=INVALID_SOCKET);
      struct sockaddr_in addr;
      memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(5060);
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
      assert(bind(fd, (struct sockaddr*)&addr, sizeof(addr))==0);
      Fifo<Stun::StunResult> fifo;
      Stun stun(fd, ip, 3478, fifo);
      while (fifo.empty())
      {
         FdSet fdset;
         stun.buildFdSet(fdset);
         fdset.selectMilliSeconds(100);
         stun.process(fdset);
      }
      Stun::StunResult* msg;
      msg = fifo.getNext();
      cout << "NAT type: " << msg->mMsg << endl;
      in_addr ip;
      ip.s_addr = ntohl(msg->mIp);
      cout << "Ip: " << DnsUtil::inet_ntop(ip) << endl;
      cout << "Port: " << msg->mPort << endl;
      delete msg;
      closeSocket(fd);
   }
   else
   {
      cerr << "Check server name is correct" << endl;
   }

   return 0;
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
