#include "precompile.h"
#include "resip/stack/SelectInterruptor.hxx"

#include <cassert>
#include "rutil/Logger.hxx"

#ifndef WIN32
#include <unistd.h>
#endif

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

SelectInterruptor::SelectInterruptor()
{
#ifdef WIN32
   mSocket = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

   sockaddr_in loopback;
   memset(&loopback, 0, sizeof(loopback));
   loopback.sin_family = AF_INET;
   loopback.sin_port = 0;
   loopback.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   makeSocketNonBlocking(mSocket); //win32 woes    
   ::bind( mSocket, reinterpret_cast<sockaddr*>(&loopback), sizeof(loopback));
   memset(&mWakeupAddr, 0, sizeof(mWakeupAddr));   
   int len = sizeof(mWakeupAddr);
   int error = getsockname(mSocket, (sockaddr *)&mWakeupAddr, &len);
   assert(error == 0);
   error= connect(mSocket, &mWakeupAddr, sizeof(mWakeupAddr)); 
   assert(error == 0);
#else
   pipe(mPipe);
#endif
}

SelectInterruptor::~SelectInterruptor()
{
#ifdef WIN32
   closesocket(mSocket);
#else
   close(mPipe[0]);
   close(mPipe[1]);
#endif
}   

void 
SelectInterruptor::handleProcessNotification()
{
   interrupt();   
}

void 
SelectInterruptor::buildFdSet(FdSet& fdset)
{
#ifdef WIN32
	fdset.setRead(mSocket);
#else
   fdset.setRead(mPipe[0]);
#endif
}

void 
SelectInterruptor::process(FdSet& fdset)
{      
#ifdef WIN32
   if ( fdset.readyToRead(mSocket))
   {
      char rdBuf[16];
      recv(mSocket, rdBuf, sizeof(rdBuf), 0);
   }
#else
   if ( fdset.readyToRead(mPipe[0]))
   {
      char rdBuf[16];
      read(mPipe[0], rdBuf, sizeof(rdBuf));
   }
#endif
}

void 
SelectInterruptor::interrupt()
{
   static char wakeUp[] = "w";
#ifdef WIN32
   int count = send(mSocket, wakeUp, sizeof(wakeUp), 0);
   assert(count == sizeof(wakeUp));
#else
   size_t res = write(mPipe[1], wakeUp, sizeof(wakeUp));
   assert(res == sizeof(wakeUp));   
#endif
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
