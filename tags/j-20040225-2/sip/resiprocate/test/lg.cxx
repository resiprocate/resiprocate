#include "resiprocate/os/Logger.hxx"

#include "resiprocate/Uri.hxx"

#include "Register.hxx"
#include "Registrar.hxx"
#include "InviteServer.hxx"
#include "InviteClient.hxx"
#include "Transceiver.hxx"


using namespace resip;
using namespace std;
using namespace Loadgen;


#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

void usage()
{
   cout << "Usage: lg port {REG_RECEIVE, INV_RECEIVE}" << endl
        << "       lg port {REG_SEND, INV_SEND} targetHost:port startingExt endingExt [registers]" << endl;
   exit(-1);
}

int 
main(int argc, char* argv[])
{
   Log::initialize(Log::COUT, Log::INFO, argv[0]);
 
   if (!(argc == 3 || argc == 6 || argc == 7))
   {
      usage();
   }

   int port = atoi(argv[1]);
   if (port == 0)
   {
      usage();
   }
   
   if (Data(argv[2]) == "REG_SEND" || Data(argv[2]) == "INV_SEND")
   {
      Uri target;

      ParseBuffer host(argv[3], strlen(argv[3]));
      const char* start = host.position();
      host.skipToChar(Symbols::COLON[0]);
      if (host.position() == host.end())
      {
         usage();
      }
      
      target.host() = host.data(start);
      host.skipChar();
      target.port() = host.integer();
      target.param(p_transport) = "udp";

      InfoLog(<< target);
   
      Transceiver stack(port);
      
      int startExt = atoi(argv[4]);
      int endExt = atoi(argv[5]);
      if (startExt == 0 || endExt == 0 || startExt > endExt)
      {
         usage();
      }
      int numTimes = 0;
      if (argc == 7)
      {
         numTimes = atoi(argv[6]);
         if (numTimes == 0)
         {
            usage();
         }
      }
      if (Data(argv[2]) == "REG_SEND")
      {
         Register reg(stack, target, 
                      startExt, endExt, numTimes);
         reg.go();
      }
      else
      {
         InviteClient inv(stack, target, 
                          startExt, endExt, numTimes);
         inv.go();
      }
   }
   else if (Data(argv[2]) == "REG_RECEIVE")
   {
      Transceiver stack(port);
      Registrar reg(stack);
      reg.go();
   }
   else if (Data(argv[2]) == "INV_RECEIVE")
   {
      Transceiver stack(port);
      InviteServer inv(stack);
      inv.go();
   }
   else
   {
      usage();
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
