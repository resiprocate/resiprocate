/*
Copyright (c) 2007, Adobe Systems, Incorporated
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

* Neither the name of Adobe Systems, Network Resonance nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <errno.h>
#include <time.h>

#ifdef WIN32

#include <winsock2.h>
#include <stdlib.h>
#include <io.h>

#else

#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#endif

#include <string.h>

#include "stun.h"
#include "udp.h"
#include "r_types.h"
#include "r_macros.h"
#include "r_errors.h"
#include "r_log.h"

Socket
openPort( unsigned short port, unsigned int interfaceIp)
{
   Socket fd;

   fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
   if ( fd == INVALID_SOCKET )
   {
      r_log_e(NR_LOG_STUN, LOG_WARNING,"Could not create a UDP socket");
      return INVALID_SOCKET;
   }

   struct sockaddr_in addr;
   memset((char*) &(addr),0, sizeof((addr)));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port = htons(port);

   if ( (interfaceIp != 0) &&
        ( interfaceIp != 0x7f000001 ) )
   {
      addr.sin_addr.s_addr = htonl(interfaceIp);
      r_log(NR_LOG_STUN, LOG_DEBUG, "Binding to interface %s",inet_ntoa(addr.sin_addr));
   }

   if ( bind( fd,(struct sockaddr*)&addr, sizeof(addr)) != 0 )
   {
      switch (errno)
      {
         case 0:
         {
            r_log(NR_LOG_STUN, LOG_WARNING,"Could not bind socket");
            return INVALID_SOCKET;
         }
         case EADDRINUSE:
         {
            r_log_e(NR_LOG_STUN, LOG_WARNING,"Port %d for receiving UDP is in use",port);
            return INVALID_SOCKET;
         }
         break;
         case EADDRNOTAVAIL:
         {
            r_log(NR_LOG_STUN, LOG_DEBUG,"Cannot assign requested address");
            return INVALID_SOCKET;
         }
         break;
         default:
         {
            r_log(NR_LOG_STUN, LOG_WARNING, "Could not bind UDP receive port");
            return INVALID_SOCKET;
         }
         break;
      }
   }
   r_log(NR_LOG_STUN, LOG_DEBUG, "Opened port %d with fd %d",port,fd);

   assert( fd != INVALID_SOCKET  );

   return fd;
}


int
getMessage( Socket fd, char* buf, int* len,
            unsigned int* srcIp, unsigned short* srcPort)
{
   int _status;

   assert( fd != INVALID_SOCKET );

   int originalSize = *len;
   assert( originalSize > 0 );

   struct sockaddr_in from;
   int fromLen = sizeof(from);

/* TODO: implement backoff for reads on client side */
   *len = recvfrom(fd,
                   buf,
                   originalSize,
                   0,
                   (struct sockaddr *)&from,
                   (socklen_t*)&fromLen);

   if ( *len == SOCKET_ERROR )
   {
      switch (errno)
      {
         case ENOTSOCK:
            r_log_e(NR_LOG_STUN, LOG_WARNING, "Error not a socket");
            break;
         case ECONNRESET:
            r_log_e(NR_LOG_STUN, LOG_WARNING, "Error connection reset - host not reachable");
            break;

         default:
            r_log_e(NR_LOG_STUN, LOG_WARNING, "Socket Error");
      }

      ABORT(R_FAILED);
   }

   if ( *len < 0 )
   {
      r_log(NR_LOG_STUN, LOG_WARNING, "socket closed? negative len");
      ABORT(R_FAILED);
   }

   if ( *len == 0 )
   {
      r_log(NR_LOG_STUN, LOG_WARNING, "socket closed? zero len");
      ABORT(R_FAILED);
   }

   *srcPort = ntohs(from.sin_port);
   *srcIp = ntohl(from.sin_addr.s_addr);

   if ( (*len)+1 >= originalSize )
   {
      r_log(NR_LOG_STUN, LOG_DEBUG, "Received a message that was too large");
      ABORT(R_FAILED);
   }
   buf[*len]=0;

    _status = 0;
  abort:
    return _status;
}

int
sendMessage( Socket fd, char* buf, int l,
             unsigned int dstIp, unsigned short dstPort)
{
   int _status;

   assert( fd != INVALID_SOCKET );

   int s;
   if ( dstPort == 0 )
   {
      // sending on a connected port
      assert( dstIp == 0 );

      s = send(fd,buf,l,0);
   }
   else
   {
      assert( dstIp != 0 );
      assert( dstPort != 0 );

      struct sockaddr_in to;
      int toLen = sizeof(to);
      memset(&to,0,toLen);

      to.sin_family = AF_INET;
      to.sin_port = htons(dstPort);
      to.sin_addr.s_addr = htonl(dstIp);

      s = sendto(fd, buf, l, 0,(struct sockaddr*)&to, toLen);
   }

   if ( s == SOCKET_ERROR )
   {
      switch (errno)
      {
         case ECONNREFUSED:
         case EHOSTDOWN:
         case EHOSTUNREACH:
         {
            // quietly ignore this
         }
         break;
         case EAFNOSUPPORT:
         {
            r_log_e(NR_LOG_STUN, LOG_WARNING, "Could not send");
         }
         break;
         default:
         {
            r_log_e(NR_LOG_STUN, LOG_WARNING, "Could not send");
         }
      }
      ABORT(R_FAILED);
   }

   if ( s == 0 )
   {
      r_log(NR_LOG_STUN, LOG_WARNING,"no data sent in send");
      ABORT(R_FAILED);
   }

   if ( s != l )
   {
      r_log(NR_LOG_STUN, LOG_DEBUG,"only %d out of %d bytes sent",s,l);
      ABORT(R_FAILED);
   }

    _status = 0;
  abort:
    return _status;
}

int
initNetwork()
{
#ifdef WIN32
   WORD wVersionRequested = MAKEWORD( 2, 2 );
   WSADATA wsaData;
   int err;

   err = WSAStartup( wVersionRequested, &wsaData );
   if ( err != 0 )
   {
      // could not find a usable WinSock DLL
      r_log(NR_LOG_STUN, LOG_WARNING,"Could not load winsock");
      assert(0); // TODO: is this is failing, try a different version that 2.2, 1.0 or later will likely work
      exit(1);
   }

   /* Confirm that the WinSock DLL supports 2.2.*/
   /* Note that if the DLL supports versions greater    */
   /* than 2.2 in addition to 2.2, it will still return */
   /* 2.2 in wVersion since that is the version we      */
   /* requested.                                        */

   if ( LOBYTE( wsaData.wVersion ) != 2 ||
        HIBYTE( wsaData.wVersion ) != 2 )
   {
      /* Tell the user that we could not find a usable */
      /* WinSock DLL.                                  */
      WSACleanup( );
      r_log(NR_LOG_STUN, LOG_WARNING,"Bad winsock verion");
      assert(0); // TODO: is this is failing, try a different version that 2.2, 1.0 or later will likely work
      exit(1);
   }
#endif
   return 0;
}

#ifndef WIN32
int
closesocket( Socket fd )
{
    return close(fd);
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

// Local Variables:
// mode:c++
// c-file-style:"ellemtel"
// c-file-offsets:((case-label . +))
// indent-tabs-mode:nil
// End:
