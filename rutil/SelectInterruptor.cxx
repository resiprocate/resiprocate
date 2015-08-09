#include "rutil/SelectInterruptor.hxx"

#include "rutil/ResipAssert.h"
#include "rutil/Logger.hxx"

#ifndef WIN32
#include <unistd.h>
#endif

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

SelectInterruptor::SelectInterruptor()
{
#ifdef WIN32
   initNetwork();  // Required for windows

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
   resip_assert(error == 0);
   error= connect(mSocket, &mWakeupAddr, sizeof(mWakeupAddr));
   resip_assert(error == 0);
   mReadThing = mSocket;
#else
   int x = pipe(mPipe);
   (void)x;
   resip_assert( x != -1 );
   // make write-side non-blocking to avoid deadlock
   makeSocketNonBlocking(mPipe[1]);
   // make read-side non-blocking so safe to read out entire pipe
   // all in one go (also just safer)
   makeSocketNonBlocking(mPipe[0]);
   mReadThing = mPipe[0];
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
SelectInterruptor::processCleanup()
{
#ifdef WIN32
  char rdBuf[16];
  recv(mSocket, rdBuf, sizeof(rdBuf), 0);
#else
  char rdBuf[16];
  int x;
  while ( (x=read(mPipe[0], rdBuf, sizeof(rdBuf))) == sizeof(rdBuf) )
     ;
  // WATCHOUT: EWOULDBLOCK *will* happen above when the pending
  // number of bytes is exactly size of rdBuf
  // XXX: should check for certain errors (like fd closed) and die?
#endif
}

void
SelectInterruptor::process(FdSet& fdset)
{
   if (fdset.readyToRead(mReadThing))
   {
      processCleanup();
   }
}

void 
SelectInterruptor::processPollEvent(FdPollEventMask mask)
{
   if(mask & FPEM_Read)
   {
      processCleanup();
   }
}


void
SelectInterruptor::interrupt()
{
   static char wakeUp[] = "w";
#ifdef WIN32
   u_long readSize = 0;
   ioctlsocket(mSocket, FIONREAD, &readSize);
   // Only bother signalling socket if there is no data on it already
   if(readSize == 0)  
   {
      int count = send(mSocket, wakeUp, sizeof(wakeUp), 0);
      resip_assert(count == sizeof(wakeUp));
   }
#else
   ssize_t res = write(mPipe[1], wakeUp, sizeof(wakeUp));

   if ( res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) )   // Treat EGAIN and EWOULDBLOCK as the same: http://stackoverflow.com/questions/7003234/which-systems-define-eagain-and-ewouldblock-as-different-values
   {
      ; // this can happen when SipStack thread gets behind.
      // no need to block since our only purpose is to wake up the thread
      // also, this write can occur within the SipStack thread, in which
      // case we get dead-lock if this blocks
   } 
   else 
   {
      resip_assert(res == sizeof(wakeUp));
   }
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
 * vi: set shiftwidth=3 expandtab:
 */
