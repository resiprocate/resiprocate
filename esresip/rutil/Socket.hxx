/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_SOCKET_HXX)
#define RESIP_SOCKET_HXX  

#include <cassert>
#include <errno.h>
#include <algorithm>

#ifdef WIN32
#include <winsock2.h>
#include <stdlib.h>
#include <io.h>
#include <WS2TCPIP.H>
#endif

#include "compat.hxx"
#include "rutil/TransportType.hxx"

/**
   @file
   @brief Handles cross-platform sockets compatibility.
   @ingroup network
*/

#ifdef WIN32

/** With VS 2010 Berkeley errno constants have been redefined and are now different than WSAGetLastError() WSA prefixed constants.
	@see http://msdn.microsoft.com/en-us/library/ms737828(VS.85).aspx, https://connect.microsoft.com/VisualStudio/feedback/details/509380/errno-h-socket-return-codes-now-inconsistent-with-wsagetlasterror?wa=wsignin1.0
	@see <winsock2.h>
	The recommended fix is to use WSAGetLastError() and with the WSAXXX constants.  One way to do this would be to create RESIP_XXX socket constants that would correclty use WSAXXX constants
	on windows without using the Berkeley style constants.  For now just re-assign the Berkeley style constants as WSA constants.
*/
#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma warning (push)
#pragma warning (disable: 4005 )
#endif
typedef int socklen_t;

//this list taken from <winsock2.h>, see #if 0 removed block.
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
//#define ENAMETOOLONG            WSAENAMETOOLONG
#define EHOSTDOWN               WSAEHOSTDOWN
#define EHOSTUNREACH            WSAEHOSTUNREACH
//#define ENOTEMPTY               WSAENOTEMPTY
#define EPROCLIM                WSAEPROCLIM
#define EUSERS                  WSAEUSERS
#define EDQUOT                  WSAEDQUOT
#define ESTALE                  WSAESTALE
#define EREMOTE                 WSAEREMOTE

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma warning (pop)
#endif

#else

#define WSANOTINITIALISED  EPROTONOSUPPORT

#endif

namespace resip
{

/// set up network - does nothing in unix but needed for windows
void
initNetwork();

#ifndef WIN32
typedef int Socket;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)
inline int getErrno() { return errno; }
#else
typedef SOCKET Socket;
inline int getErrno() { return WSAGetLastError(); }
#endif

//c function pointer because of ares
extern "C" {
typedef void(*AfterSocketCreationFuncPtr)(Socket s, int transportType, const char* file, int line);
}

bool makeSocketNonBlocking(Socket fd);
bool makeSocketBlocking(Socket fd);
/// platform-generic wrapper to close a socket.
int closeSocket( Socket fd );

/**
   @brief Object-oriented wrapper for your platform's file-descriptor
   set.

   FdSet uses the platform FD_* macros to build and maintain fd_sets
   for use with select().  This wraps a read fd set, a write fd set,
   and an exception fd set. 
*/
class FdSet
{
   public:
      FdSet() : size(0), numReady(0)
      {
         FD_ZERO(&read);
         FD_ZERO(&write);
         FD_ZERO(&except);
      }

      int select(struct timeval& tv)
      {
         return numReady = ::select(size, &read, &write, &except, &tv);
      }

      int selectMilliSeconds(unsigned long ms)
      {
         struct timeval tv;
         tv.tv_sec = (ms/1000);
         tv.tv_usec = (ms%1000)*1000;
         return select(tv);
      }

      bool readyToRead(Socket fd)
      {
         return ( FD_ISSET(fd, &read) != 0);
      }

      bool readyToWrite(Socket fd)
      {
         return ( FD_ISSET(fd, &write) != 0);
      }

      bool hasException(Socket fd)
      {
          return (FD_ISSET(fd,&except) != 0);
      }

      /// Add a socket to the read fd_set.
      void setRead(Socket fd)
      {
        assert( FD_SETSIZE >= 8 );
#ifndef WIN32 // windows fd are not int's and don't start at 0 - this won't work in windows
         assert( fd < (int)FD_SETSIZE ); // redefineing FD_SETSIZE will not work 
#else
         assert(read.fd_count < FD_SETSIZE); // Ensure there is room to add new FD
#endif
         FD_SET(fd, &read);
         size = ( int(fd+1) > size) ? int(fd+1) : size;
      }

      /// Add a socket to the write fd_set.
      void setWrite(Socket fd)
      {
#ifndef WIN32 // windows fd are not int's and don't start at 0 - this won't work in windows
        assert( fd < (int)FD_SETSIZE ); // redefinitn FD_SETSIZE will not work 
#else
        assert(write.fd_count < FD_SETSIZE); // Ensure there is room to add new FD
#endif
         FD_SET(fd, &write);
         size = ( int(fd+1) > size) ? int(fd+1) : size;
      }

      /// Add a socket to the exception fd_set.
      void setExcept(Socket fd)
      {
#ifndef WIN32 // windows fd are not int's and don't start at 0 - this won't work in windows
        assert( fd < (int)FD_SETSIZE ); // redefinitn FD_SETSIZE will not work 
#else
        assert(except.fd_count < FD_SETSIZE); // Ensure there is room to add new FD
#endif
        FD_SET(fd,&except);
        size = ( int(fd+1) > size) ? int(fd+1) : size;
   }


      /// Remove the fd from all fd_sets in which it might be present.
      void clear(Socket fd)
      {
         FD_CLR(fd, &read);
         FD_CLR(fd, &write);
         FD_CLR(fd, &except);
      }

      /// Clears all fd_sets.  
      void reset()
      {
         size = 0;
         numReady = 0;
         FD_ZERO(&read);
         FD_ZERO(&write);
         FD_ZERO(&except);
      }

      // Make this stuff public for async dns/ares to use
      fd_set read;
      fd_set write;
      fd_set except;
      int size;
     int numReady;  // set after each select call
};

	
}

#endif 

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
