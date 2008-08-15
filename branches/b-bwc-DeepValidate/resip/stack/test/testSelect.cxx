
#include <cassert>
#include <iostream>

#include "rutil/DnsUtil.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Timer.hxx"
#include "resip/stack/UdpTransport.hxx"

using namespace resip;
using namespace std;


int 
main(int argc, char* argv[])
{
   if ( argc != 3 ) 
   {
      cerr << "Usage: testSelect startPort endPort\n";
      exit(1);
   }
   
   int startPort = atoi( argv[1] );
   int endPort = atoi( argv[2] );
   
   cout << "Doing port " << startPort << " to " << endPort << endl;
   
   int fd[0xFFFF];
   
 #define NUMCYCLES 50
   
   UInt64 time1[NUMCYCLES];
   UInt64 time2[NUMCYCLES];
   UInt64 time3[NUMCYCLES];
   
   // open the fd 
   for( int port=startPort; port<endPort; port++)
   {
      fd[port] =  socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if( fd[port] ==  INVALID_SOCKET )
      {
         cerr << "Fail on creating socket for port " << port << endl;
         assert(0);
      }
      
      sockaddr_in addr4; 
      sockaddr* saddr=0;
      int saddrLen = 0;
      
      memset(&addr4, 0, sizeof(addr4));
      addr4.sin_family = AF_INET;
      addr4.sin_port = htons(port);
      addr4.sin_addr.s_addr = htonl(INADDR_ANY); 

      saddr = reinterpret_cast<sockaddr*>(&addr4);
      saddrLen = sizeof(addr4);

      if ( bind( fd[port], saddr, saddrLen) == SOCKET_ERROR )
      {
         assert(0);
      }
      
      bool ok = makeSocketNonBlocking( fd[port] );
      assert(ok);
   }
   

   for ( int i=0; i<NUMCYCLES; i++ )
   {
      FdSet fdset;
      
      time1[i] = Timer::getTimeMicroSec();
      // build the fd
      for( int port=startPort; port<endPort; port++)
      {
         fdset.setRead(fd[port]);
      }
      
      time2[i] = Timer::getTimeMicroSec();
      // select
      int  err = fdset.selectMilliSeconds( 0 );
      assert( err != -1 );
      
      time3[i] = Timer::getTimeMicroSec();
      // do the reads
      for( int port=startPort; port<endPort; port++)
      {
         if (fdset.readyToRead(fd[port]))
         {
            struct sockaddr from;
            
            const int MaxBufferSize=1024*4;
            char buffer[MaxBufferSize];
            socklen_t fromLen = sizeof(from);
            
            int len = recvfrom( fd[port],
                                buffer,
                                MaxBufferSize,
                                0 /*flags */,
                                (struct sockaddr*)&from,
                                &fromLen);
            
            if ( len == SOCKET_ERROR )
            {
               int err = errno;
               switch (err)
               {
                  case WSANOTINITIALISED:
                     assert(0);
                     break;
                     
                  case EWOULDBLOCK:
                     //cerr << " UdpTransport recvfrom got EWOULDBLOCK";
                     break;
                     
                  case 0:
                     cerr << " UdpTransport recvfrom got error 0 ";
                     break;
                     
                  default:
                     cerr <<"Error receiving, errno="<<err << " " << strerror(err);
                     break;
               }
            }
         }
         
      }
   }
   
   for ( int i=1; i<NUMCYCLES; i++ )
   {
      cout << time2[i] - time1[i] << " ";
      cout << time1[i] - time1[i-1] << " ";
      
      cout << endl;
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
