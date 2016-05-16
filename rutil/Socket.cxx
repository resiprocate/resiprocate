
#include "rutil/ResipAssert.h"
#include <fcntl.h>
#include <errno.h>

#include "rutil/compat.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Logger.hxx"

#ifndef WIN32
#include <unistd.h>
#include <sys/resource.h>	// for getrlimit()
#endif

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

bool
resip::makeSocketNonBlocking(Socket fd)
{
#if defined(WIN32)
	unsigned long noBlock = 1;
	int errNoBlock = ioctlsocket( fd, FIONBIO , &noBlock );
	if ( errNoBlock != 0 )
	{
		return false;
	}
#else
	int flags  = fcntl( fd, F_GETFL, 0);
	int errNoBlock = fcntl(fd, F_SETFL, flags | O_NONBLOCK );
	if ( errNoBlock != 0 ) // !cj! I may have messed up this line
	{
		return false;
	}
#endif
	return true;
}


bool
resip::makeSocketBlocking(Socket fd)
{
#if defined(WIN32)
	unsigned long noBlock = 0;
	int errNoBlock = ioctlsocket( fd, FIONBIO , &noBlock );
	if ( errNoBlock != 0 )
	{
		return false;
	}
#else
	int flags  = fcntl( fd, F_GETFL, 0);
	int errNoBlock = fcntl(fd, F_SETFL, flags & ~O_NONBLOCK );
	if ( errNoBlock != 0 ) // !cj! I may have messed up this line
	{
		return false;
	}
#endif
	return true;
}



bool
resip::configureConnectedSocket(Socket fd)
{
#ifdef REQUIRE_SO_NOSIGPIPE
   int on = 1;
   if ( ::setsockopt ( fd, SOL_SOCKET, SO_NOSIGPIPE, (const char*)&on, sizeof(on)) )
   {
      int e = getErrno();
      ErrLog (<< "Couldn't set sockoption SO_NOSIGPIPE: " << strerror(e));
      return false;
   }
#endif
   return true;
}



void
resip::initNetwork()
{
#if defined(WIN32)
	bool doneInit=false;
	if( !doneInit )
	{
		doneInit=true;

   WORD wVersionRequested = MAKEWORD( 2, 2 );
   WSADATA wsaData;
   int err;

   err = WSAStartup( wVersionRequested, &wsaData );
   if ( err != 0 )
   {
      // could not find a usable WinSock DLL
      //cerr << "Could not load winsock" << endl;
      resip_assert(0); // is this is failing, try a different version that 2.2, 1.0 or later will likely work
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
      //cerr << "Bad winsock verion" << endl;
      // TODO !cj! - add error message logging
      resip_assert(0); // if this is failing, try a different version that 2.2, 1.0 or later will likely work
      exit(1);
   }
	}
#endif
}


#if defined(WIN32)
int
resip::closeSocket( Socket fd )
{
   return closesocket(fd);
}
#else
int
resip::closeSocket( Socket fd )
{
   //int ret = ::shutdown(fd, SHUT_RDWR); !jf!
   int ret = ::close(fd);
   if (ret < 0)
   {
      InfoLog (<< "Failed to shutdown socket " << fd << " : " << strerror(errno));
   }
   return ret;
}
#endif

// code moved from resip/stack/ConnectionManager.cxx
// appears to work on both linux and windows
int resip::getSocketError(Socket fd)
{
   int errNum = 0;
   int errNumSize = sizeof(errNum);
   getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&errNum, (socklen_t *)&errNumSize);
   /// XXX: should check return code of getsockopt
   return errNum;
}

/**
    Returns negative on error, or number of (positive) allowed fds
**/
int
resip::increaseLimitFds(unsigned int targetFds)
{
#if defined(WIN32)
    // kw: i don't know if any equiv on windows
    return targetFds;
#else
    struct rlimit lim;

    if (getrlimit(RLIMIT_NOFILE, &lim) < 0)
	{
	   CritLog(<<"getrlimit(NOFILE) failed: " << strerror(errno));
	   return -1;
    }
    if (lim.rlim_cur==RLIM_INFINITY || targetFds < lim.rlim_cur)
	{
        return targetFds;
	}

    int euid = geteuid();
    if (lim.rlim_max==RLIM_INFINITY || targetFds < lim.rlim_max)
	{
    	lim.rlim_cur=targetFds;
    }
	else
	{
	   if (euid!=0)
	   {
	      CritLog(<<"Attempting to increase number of fds when not root. This probably wont work");
	   }
       lim.rlim_cur=targetFds;
       lim.rlim_max=targetFds;
    }

    if (setrlimit(RLIMIT_NOFILE, &lim) < 0)
	{
	   CritLog(<<"setrlimit(NOFILE)=(c="<<lim.rlim_cur<<",m="<<lim.rlim_max
	      <<",uid="<<euid<<") failed: " << strerror(errno));
	   /* There is intermediate: could raise cur to max */
	   return -1;
    }
    return targetFds;
#endif
}

/**
    Some OSs (Linux in particular) silently ignore requests to set
    too big and do not return an error code. Thus we always check.
    Also, the returned value (getsockopt) can be larger than the requested
    value (kernel internally doubles).
    manpage says sockopt uses integer as data-type
    If {buflen} is negative, we skip the set and just read
    Return the get size or -1 if the set didn't work
**/
static int trySetRcvBuf(Socket fd, int buflen)
{
   if (buflen > 0)
   {
      int wbuflen = buflen;
#if !defined(WIN32)
      if (::setsockopt (fd, SOL_SOCKET, SO_RCVBUF, &wbuflen, sizeof(wbuflen)) == -1)
#else
      if (::setsockopt (fd, SOL_SOCKET, SO_RCVBUF, (const char*)&wbuflen, sizeof(wbuflen)) == -1)
#endif
      {
         return -1;
      }
   }
   int rbuflen = 0;
   unsigned optlen = sizeof(rbuflen);
   if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&rbuflen, (socklen_t *)&optlen) == -1)
   {
      return -1;
   }
   resip_assert(optlen == sizeof(rbuflen));
   if (rbuflen < buflen)
   {
      return -1;
   }
   return rbuflen;
}

/**
**/
int resip::setSocketRcvBufLen(Socket fd, int buflen)
{
   resip_assert(buflen >= 1024);
   int goal=buflen;
   int trylen=goal;
   int sts;
   int lastgoodset = 0, lastgoodget=0;

   /* go down by factors of 2 */
   for (; ; trylen /= 2)
   {
      if (trylen < 1024)
      {
        ErrLog(<<"setsockopt(SO_RCVBUF) failed");
        return -1;
      }
      if ((sts=trySetRcvBuf(fd, trylen)) >= 0)
      {
         lastgoodset = trylen;
         lastgoodget = sts;
         break;
      }
   }

   /* go up by 10% steps */
   unsigned step = trylen/10;
   for ( ; trylen<goal; trylen+=step)
   {
      if ((sts=trySetRcvBuf(fd,trylen)) < 0)
      {
         break;
      }
      lastgoodset = trylen;
      lastgoodget = sts;
   }
   if (lastgoodset < goal)
   {
      ErrLog(<<"setsockopt(SO_RCVBUF) goal "<<goal<<" not met (set="
         <<lastgoodset<<",get="<<lastgoodget<<")");
   }
   else
   {
      InfoLog(<<"setsockopt(SO_RCVBUF) goal "<<goal<<" met (set="
         <<lastgoodset<<",get="<<lastgoodget<<")");
   }
   return lastgoodset;
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
