
#include <cassert>
#include <fcntl.h>

#include "resiprocate/os/Socket.hxx"

#ifndef WIN32
#include <unistd.h>
#endif

using namespace resip;
using namespace std;

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
      assert(0); // is this is failing, try a different version that 2.2, 1.0 or later will likely work 
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
      assert(0); // is this is failing, try a different version that 2.2, 1.0 or later will likely work 
      exit(1);
   }  
	}
#endif
}


#if !defined(WIN32)
int 
resip::closesocket( Socket fd )
{
   return close( fd );
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
