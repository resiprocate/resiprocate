#if defined(WIN32)
#include <winsock2.h>
#include <WS2TCPIP.H>
#else
#include <netinet/in.h>
#endif

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include "rutil/ResipAssert.h"

#include "FlowManagerSubsystem.hxx"
#include "FakeSelectSocketDescriptor.hxx"

using namespace flowmanager;
using namespace std;

#define RESIPROCATE_SUBSYSTEM FlowManagerSubsystem::FLOWMANAGER

FakeSelectSocketDescriptor::FakeSelectSocketDescriptor()
{
#ifdef WIN32
   sockaddr socketAddr;
   mSocket = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

   sockaddr_in loopback;
   memset(&loopback, 0, sizeof(loopback));
   loopback.sin_family = AF_INET;
   loopback.sin_port = 0;
   loopback.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
 	unsigned long noBlock = 1;
	ioctlsocket( mSocket, FIONBIO , &noBlock );
   ::bind( mSocket, reinterpret_cast<sockaddr*>(&loopback), sizeof(loopback));
   memset(&socketAddr, 0, sizeof(socketAddr));   
   int len = sizeof(socketAddr);
   int error = getsockname(mSocket, (sockaddr *)&socketAddr, &len);
   resip_assert(error == 0);
   error= connect(mSocket, &socketAddr, sizeof(socketAddr)); 
   resip_assert(error == 0);
#else
   pipe(mPipe);
#endif
}

FakeSelectSocketDescriptor::~FakeSelectSocketDescriptor()
{
#ifdef WIN32
   closesocket(mSocket);
#else
   close(mPipe[0]);
   close(mPipe[1]);
#endif
}

unsigned int 
FakeSelectSocketDescriptor::getSocketDescriptor()
{
#ifdef WIN32
	return mSocket;
#else
   return mPipe[0];
#endif
}

void 
FakeSelectSocketDescriptor::send()
{
   static char fakeData[] = "*";
#ifdef WIN32
   int count = ::send(mSocket, fakeData, 1, 0);
   resip_assert(count == 1);
#else
   size_t res = ::write(mPipe[1], fakeData, 1);
   resip_assert(res == 1);   
#endif
}

void 
FakeSelectSocketDescriptor::receive()
{
#ifdef WIN32
   char rdBuf[1];
   ::recv(mSocket, rdBuf, 1, 0);
#else
   char rdBuf[1];
   ::read(mPipe[0], rdBuf, 1);
#endif
}


/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

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

 ==================================================================== */
