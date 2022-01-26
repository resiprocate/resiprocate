#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <iostream>

#if defined(HAVE_SYS_SOCKIO_H)
#include <sys/sockio.h>
#endif

#include "resip/stack/Helper.hxx"
#include "resip/stack/InternalTransport.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT


InternalTransport::InternalTransport(Fifo<TransactionMessage>& rxFifo,
                                     int portNum,
                                     IpVersion version,
                                     const Data& interfaceObj,
                                     AfterSocketCreationFuncPtr socketFunc,
                                     Compression &compression,
                                     unsigned transportFlags,
                                     const Data& netNs) :
   Transport(rxFifo, portNum, version, interfaceObj, Data::Empty,
             socketFunc, compression, transportFlags, netNs),
   mFd(INVALID_SOCKET),
   mInterruptorHandle(0),
   mTxFifoOutBuffer(mTxFifo),
   mPollGrp(NULL),
   mPollItemHandle(NULL)
{}

InternalTransport::~InternalTransport()
{
   if (mPollItemHandle)
   {
      mPollGrp->delPollItem(mPollItemHandle);
   }
   if (mInterruptorHandle)
   {
      mPollGrp->delPollItem(mInterruptorHandle);
   }

   if  (mFd != INVALID_SOCKET)
   {
      //DebugLog (<< "Closing " << mFd);
      closeSocket(mFd);
   }
   mFd = -2;
   if(!mTxFifo.empty())
   {
      WarningLog(<< "TX Fifo non-empty in ~InternalTransport! Has " << mTxFifo.size() << " messages.");
   }
   setCongestionManager(0);  // Clear out congestion manager
}

bool
InternalTransport::isFinished() const
{
   return !mTxFifoOutBuffer.messageAvailable();
}

Socket
InternalTransport::socket(TransportType type, IpVersion ipVer)
{
   Socket fd;
   switch (type)
   {
      case UDP:
#ifdef USE_IPV6
         fd = ::socket(ipVer == V4 ? PF_INET : PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
#else
         fd = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
#endif
         break;
      case TCP:
      case TLS:
#ifdef USE_IPV6
         fd = ::socket(ipVer == V4 ? PF_INET : PF_INET6, SOCK_STREAM, 0);
#else
         fd = ::socket(PF_INET, SOCK_STREAM, 0);
#endif
         break;
      default:
         InfoLog (<< "Try to create an unsupported socket type: " << Tuple::toData(type));
         resip_assert(0);
         throw Transport::Exception("Unsupported transport", __FILE__,__LINE__);
   }

   if ( fd == INVALID_SOCKET )
   {
      int e = getErrno();
      ErrLog (<< "Failed to create socket: " << strerror(e));
      throw Transport::Exception("Can't create TcpBaseTransport", __FILE__,__LINE__);
   }

#ifdef USE_IPV6
#ifdef __linux__
   int on = 1;
   if (ipVer == V6)
   {
      if ( ::setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) )
      {
          int e = getErrno();
          InfoLog (<< "Couldn't set sockoptions IPV6_V6ONLY: " << strerror(e));
          error(e);
          throw Exception("Failed setsockopt", __FILE__,__LINE__);
      }
   }
#endif
#endif

   DebugLog (<< "Creating fd=" << fd << (ipVer == V4 ? " V4/" : " V6/") << (type == UDP ? "UDP" : "TCP"));

   return fd;
}

void
InternalTransport::bind()
{
#ifdef USE_NETNS
   DebugLog (<< "Binding to " << Tuple::inet_ntop(mTuple) 
             << " in netns=\"" <<mTuple.getNetNs() << "\"");
#else
   DebugLog (<< "Binding to " << Tuple::inet_ntop(mTuple)); 
#endif

   if ( ::bind( mFd, &mTuple.getMutableSockaddr(), mTuple.length()) == SOCKET_ERROR )
   {
      int e = getErrno();
      if ( e == EADDRINUSE )
      {
         error(e);
         ErrLog (<< mTuple << " already in use ");
         throw Transport::Exception("port already in use", __FILE__,__LINE__);
      }
      else
      {
         error(e);
         ErrLog (<< "Could not bind to " << mTuple);
         throw Transport::Exception("Could not use port", __FILE__,__LINE__);
      }
   }

   // If we bound to port 0, then query OS for assigned port number
   if (mTuple.getPort() == 0)
   {
      socklen_t len = mTuple.length();
      if (::getsockname(mFd, &mTuple.getMutableSockaddr(), &len) == SOCKET_ERROR)
      {
         int e = getErrno();
         ErrLog (<<"getsockname failed, error=" << e);
         throw Transport::Exception("Could not query port", __FILE__,__LINE__);
      }
   }

   bool ok = makeSocketNonBlocking(mFd);
   if ( !ok )
   {
      ErrLog (<< "Could not make socket non-blocking " << port());
      throw Transport::Exception("Failed making socket non-blocking", __FILE__,__LINE__);
   }

   if (mSocketFunc)
   {
      mSocketFunc(mFd, transport(), __FILE__, __LINE__);
   }
}

unsigned int
InternalTransport::getFifoSize() const
{
   return mTxFifo.size();
}

bool
InternalTransport::hasDataToSend() const
{
   return mTxFifoOutBuffer.messageAvailable();
}

void
InternalTransport::send(std::unique_ptr<SendData> data)
{
   mTxFifo.add(data.release());
}

void
InternalTransport::setPollGrp(FdPollGrp *grp)
{
   if(!shareStackProcessAndSelect())
   {
      // If this transport does not have its own thread, it does not need to
      // register its SelectInterruptor because the TransportSelector will take
      // care of interrupting the select()/epoll() loop when necessary.
      if(mPollGrp && mInterruptorHandle)
      {
         mPollGrp->delPollItem(mInterruptorHandle);
         mInterruptorHandle=0;
      }

      if (grp)
      {
         mInterruptorHandle = grp->addPollItem(mSelectInterruptor.getReadSocket(), FPEM_Read, &mSelectInterruptor);
      }
   }

   mPollGrp = grp;
}

void 
InternalTransport::poke()
{
   // !bwc! I have tried installing mSelectInterruptor in mTxFifo, but it 
   // hampers performance. This seems to be because we get a significant 
   // performance boost from having multiple messages added to mTxFifo before 
   // mSelectInterruptor is invoked (this is what the TransactionController 
   // does; it processes at most 16 TransactionMessages, and then pokes the 
   // Transports). Once we have buffered producer queues in place, this 
   // performance concern will be rendered moot, and we'll be able to install 
   // the interruptor in mTxFifo.
   if(mTxFifoOutBuffer.messageAvailable())
   {
      // This will interrupt the select statement and cause processing of 
      // this new outgoing message.
      mSelectInterruptor.handleProcessNotification();
   }
}

void 
InternalTransport::invokeAfterSocketCreationFunc() const
{
    if (mSocketFunc)
    {
        mSocketFunc(mFd, transport(), __FILE__, __LINE__);
    }
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
