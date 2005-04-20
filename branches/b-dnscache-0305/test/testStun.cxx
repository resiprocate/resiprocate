#include <iostream>

#include "resiprocate/os/Socket.hxx"
#include "resiprocate/Stun.hxx"
#include "resiprocate/os/Logger.hxx"

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

   if ( host )
   {
      sin_addr = *(struct in_addr*)host->h_addr;
      ip = ntohl(sin_addr.s_addr);
      Stun stun;
      stun.quickTest(true);
      stun.symNatTest(false);
      Stun::StunResult result;
      result = stun.NATType(ip, 3478);
      cout << "Quick test:" << endl;
      if ( result.type == Stun::NatTypeFailure )
      {
         cout << result.msg << endl << endl;
      }
      else
      {
         cout << result.msg << endl;
         unsigned long ip = htonl(result.ip);
         cout << "Address: " << inet_ntoa(*(in_addr*)&ip) << ":" << (result.port) << endl << endl;
      }

      stun.quickTest(false);
      stun.symNatTest(true);      
      result = stun.NATType(ip, 3478);
      cout << "Symmetric test:" << endl;
      if ( result.type == Stun::NatTypeFailure )
      {
         cout << result.msg << endl << endl;
      }
      else
      {
         cout << result.msg << endl;
         unsigned long ip = htonl(result.ip);
         cout << "Address: " << inet_ntoa(*(in_addr*)&ip) << ":" << (result.port) << endl << endl;
      }

      stun.quickTest(false);
      stun.symNatTest(false);
      result = stun.NATType(ip, 3478);
      cout << "Full test:" << endl;
      if ( result.type == Stun::NatTypeFailure )
      {
         cout << result.msg << endl << endl;
      }
      else
      {
         cout << result.msg << endl;
         unsigned long ip = htonl(result.ip);
         cout << "Address: " << inet_ntoa(*(in_addr*)&ip) << ":" << (result.port) << endl << endl;
      }
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
