#if !defined(RESIP_SOCKET_HXX)
#define RESIP_SOCKET_HXX  

#include "compat.hxx"

#include <cassert>

#ifdef WIN32
#include <winsock2.h>
#include <stdlib.h>
#include <io.h>
#endif


#ifdef WIN32

typedef int socklen_t;
//#define errno WSAGetLastError()
typedef SOCKET Socket;

#define EWOULDBLOCK             WSAEWOULDBLOCK
#define EINPROGRESS             WSAEINPROGRESS
#define EALREADY                WSAEALREADY
#define ENOTSOCK                WSAENOTSOCK
#define EDESTADDRREQ            WSAEDESTADDRREQ
#define EMSGSIZE                WSAEMSGSIZE
#define EPROTOTYPE              WSAEPROTOTYPE
#define ENOPROTOOPT             WSAENOPROTOOPT
#define EPROTONOSUPPORT         WSAEPROTONOSUPPORT
#define ESOCKTNOSUPPORT         WSAESOCKTNOSUPPORT
#define EOPNOTSUPP              WSAEOPNOTSUPP
#define EPFNOSUPPORT            WSAEPFNOSUPPORT
#define EAFNOSUPPORT            WSAEAFNOSUPPORT
#define EADDRINUSE              WSAEADDRINUSE
#define EADDRNOTAVAIL           WSAEADDRNOTAVAIL
#define ENETDOWN                WSAENETDOWN
#define ENETUNREACH             WSAENETUNREACH
#define ENETRESET               WSAENETRESET
#define ECONNABORTED            WSAECONNABORTED
#define ECONNRESET              WSAECONNRESET
#define ENOBUFS                 WSAENOBUFS
#define EISCONN                 WSAEISCONN
#define ENOTCONN                WSAENOTCONN
#define ESHUTDOWN               WSAESHUTDOWN
#define ETOOMANYREFS            WSAETOOMANYREFS
#define ETIMEDOUT               WSAETIMEDOUT
#define ECONNREFUSED            WSAECONNREFUSED
#define ELOOP                   WSAELOOP
#define EHOSTDOWN               WSAEHOSTDOWN
#define EHOSTUNREACH            WSAEHOSTUNREACH
#define EPROCLIM                WSAEPROCLIM
#define EUSERS                  WSAEUSERS
#define EDQUOT                  WSAEDQUOT
#define ESTALE                  WSAESTALE
#define EREMOTE                 WSAEREMOTE

#else

#ifdef __APPLE__
typedef int socklen_t;
#endif

#define WSANOTINITIALISED  EPROTONOSUPPORT

#endif


namespace resip
{

/// set up network - does nothing in unix but needed for windows
void
initNetwork();

#ifndef WIN32
typedef int Socket;
static const Socket INVALID_SOCKET = -1;
static const int SOCKET_ERROR = -1;
#endif

bool makeSocketNonBlocking(Socket fd);
bool makeSocketBlocking(Socket fd);

int closesocket( Socket fd );



class FdSet
{
   public:
      FdSet() : size(0)
      {
         FD_ZERO(&read);
         FD_ZERO(&write);
      }
      
      int select(struct timeval& tv)
      {
         return ::select(size, &read, &write, NULL, &tv);
      }

      int selectMilliSeconds(unsigned long ms)
      {
         struct timeval tv;
         tv.tv_sec = (ms/1000);
         tv.tv_usec = (ms%1000)*1000;
         return ::select(size, &read, &write, NULL, &tv);
      }

      bool readyToRead(Socket fd)
      {
         return ( FD_ISSET(fd, &read) != 0);
      }
      
      bool readyToWrite(Socket fd)
      {
         return ( FD_ISSET(fd, &write) != 0);
      }

      void setRead(Socket fd)
      {
         assert( fd < FD_SETSIZE ); // redefineing FD_SETSIZE will not work 
         FD_SET(fd, &read);
         size = ( int(fd+1) > size) ? int(fd+1) : size;
      }

      void setWrite(Socket fd)
      {
         assert( fd < FD_SETSIZE ); // redefinitn FD_SETSIZE will not work 
         FD_SET(fd, &write);
         size = ( int(fd+1) > size) ? int(fd+1) : size;
      }
      
      void clear(Socket fd)
      {
         FD_CLR(fd, &read);
         FD_CLR(fd, &write);
      }
      
      void reset()
      {
         size = 0;
         FD_ZERO(&read);
         FD_ZERO(&write);
      }

      // Make this stuff public for async dns/ares to use
      fd_set read;
      fd_set write;
      int size;
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
