/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
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

#ifdef RESIP_USE_SCTP
#include <netinet/sctp.h>
#endif

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
                                       bool hasOwnThread) :
   Transport(rxFifo, portNum, version, interfaceObj, Data::Empty, 
             socketFunc, compression, transportFlags),
   mFd(INVALID_SOCKET),
   mNeedsProcess(false),
   mHasOwnThread(hasOwnThread)
{}

InternalTransport::~InternalTransport()
{
   if (mFd != INVALID_SOCKET)
   {
      //DebugLog (<< "Closing " << mFd);
      closeSocket(mFd);
   }
   mFd = -2;
   if(!mTxFifo.empty())
   {
      WarningLog(<< "TX Fifo non-empty in ~InternalTransport! Has " << mTxFifo.size() << " messages.");
   }
}

bool
InternalTransport::isFinished() const
{
   return !mTxFifo.messageAvailable();
}

Socket
InternalTransport::socket(TransportType type, IpVersion ipVer, int typeOverride)
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
#ifdef HAVE_SCTP
      case SCTP:
#ifdef USE_IPV6
         // !bwc! This only does one-to-many SCTP. Maybe throw in a bool to 
         // disambiguate? Or do we just make the caller create the socket 
         // themselves?
         fd = ::socket(ipVer == V4 ? PF_INET : PF_INET6, 
                        typeOverride ? typeOverride : SOCK_STREAM, 
                        IPPROTO_SCTP);
#else
         fd = ::socket(PF_INET, 
                        typeOverride ? typeOverride : SOCK_STREAM, 
                        IPPROTO_SCTP);
#endif
         break;
#endif
      default:
         InfoLog (<< "Try to create an unsupported socket type: " << Tuple::toData(type));
         assert(0);
         throw Transport::Exception("Unsupported transport", __FILE__,__LINE__);
   }
   
   if ( fd == INVALID_SOCKET )
   {
      int e = getErrno();
      InfoLog (<< "Failed to create socket: " << strerror(e));
      throw Transport::Exception("Can't create TcpBaseTransport", __FILE__,__LINE__);
   }

   DebugLog (<< "Creating fd=" << fd << (ipVer == V4 ? " V4/" : " V6/") << toData(type));
   
   return fd;
}

void 
InternalTransport::bind()
{
   DebugLog (<< "Binding to " << Tuple::inet_ntop(mTuple));
   
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
   if(mTuple.getPort() == 0)
   {
      socklen_t len = sizeof(mTuple.getMutableSockaddr());
      if(::getsockname(mFd, &mTuple.getMutableSockaddr(), &len) == SOCKET_ERROR)
      {
         int e = getErrno();
         ErrLog (<<"getsockname failed, error=" << e);
         throw Transport::Exception("Could not query port", __FILE__,__LINE__);
      }
   }

   // If we bound to port 0, then query OS for assigned port number
   if(mTuple.getPort() == 0)
   {
      socklen_t len = sizeof(mTuple.getMutableSockaddr());
      if(::getsockname(mFd, &mTuple.getMutableSockaddr(), &len) == SOCKET_ERROR)
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
   return mTxFifo.messageAvailable();
}

void
InternalTransport::send(std::auto_ptr<SendData> data)
{
   if(mTxFifo.getRejectionBehavior() == CongestionManager::NORMAL)
   {
      if(mHasOwnThread && !mNeedsProcess && !mTxFifo.messageAvailable())
      {
         // We do not call mSelectInterruptor.handleProcessNotification() here;
         // when the TransactionController is done with a process call (which 
         // may result in several messages being sent to this transport), it 
         // will call poke(). This way, we are guaranteed that the transport 
         // thread will be interrupted in a timely fashion when new data is 
         // ready to be sent, but at the same time we will be economizing our 
         // interrupts when under heavy load.
         mNeedsProcess = true;
      }
      mTxFifo.add(data.release());
   }
}

void 
InternalTransport::poke()
{
   if(mNeedsProcess)
   {
      // This will interrupt the select statement and cause processing of 
      // this new outgoing message.
      mSelectInterruptor.handleProcessNotification();
      mNeedsProcess=false;
   }
}


/* Copyright 2007 Estacado Systems */


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
