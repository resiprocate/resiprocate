/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "rutil/EsLogger.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Socket.hxx"
#include "resip/stack/TcpConnection.hxx"
#include "resip/stack/Tuple.hxx"

#ifdef RESIP_USE_SCTP
#include <netinet/sctp.h>
#endif

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

TcpConnection::TcpConnection(Transport* transport,const Tuple& peer, Socket fd,
                             Compression &compression) 
  : Connection(transport,peer, fd, compression)
{
   DebugLog (<< "Creating "<< toData(peer.getType()) <<" connection " << peer << " on " << fd);
   if(who().getType()==SCTP)
   {
#ifdef RESIP_USE_SCTP
      // Set up subscription to peer address changes; this causes a struct 
      // sctp_paddr_change to be received by sctp_recvmsg whenever the peer 
      // address set changes.

      struct sockaddr* addrPtr=0;
      int res = sctp_getpaddrs(fd, 0, &addrPtr);
      if(res <= 0 || !addrPtr)
      {
         ErrLog(<<"Error getting paddrs with sctp_getpaddrs(): res=" << res << ", addrPtr=" << addrPtr);
      }
      else
      {
         for(;res > 0; --res)
         {
            Tuple addr(addrPtr[res-1], SCTP);
            InfoLog(<< "Adding initial peer addr: " << addr);
            addPeerAddress(addr);
         }
         sctp_freepaddrs(addrPtr);
      }

#ifdef SCTP_EVENT
      struct sctp_event sub;
      memset(&sub, 0, sizeof(sub));
      event.se_on=1;
      event.se_type=SCTP_PEER_ADDR_CHANGE;
      ::setsockopt(fd, SOL_SCTP, SCTP_EVENT, &sub, sizeof(sub));
#else
      // .bwc. The SCTP_EVENTS sockopt is deprecated; eventually we will start 
      // using SCTP_EVENT instead.
      struct sctp_event_subscribe sub;
      memset(&sub, 0, sizeof(sub));
      sub.sctp_address_event=1;
      ::setsockopt(fd, SOL_SCTP, SCTP_EVENTS, &sub, sizeof(sub));
#endif // SCTP_EVENT

#else
      assert(0);
#endif
   }
}

int 
TcpConnection::read( char* buf, int count )
{
   assert(buf);
   assert(count > 0);
   
#if defined(WIN32)
   int bytesRead = ::recv(getSocket(), buf, count, 0);
#else
   int bytesRead = 0;
   if(who().getType()==SCTP)
   {
#ifdef RESIP_USE_SCTP
      int flags=0;
      // We need to use sctp_recvmsg, because that's the only way we can get 
      // notifications of peer address changes.
      bytesRead = ::sctp_recvmsg(getSocket(), 
                                 buf, 
                                 count, 
                                 0, 
                                 0,
                                 0,
                                 &flags);
      if(flags & MSG_NOTIFICATION)
      {
         // Got an sctp event notification; this should be a peer-address change
         handlePeerAddressChange(buf, bytesRead);
         return 0;
      }
#else
      assert(0);
#endif
   }
   else
   {
      bytesRead = ::read(getSocket(), buf, count);
   }
#endif

   if (bytesRead == INVALID_SOCKET)
   {
      int e = getErrno();
      switch (e)
      {
         case EAGAIN:
#ifdef WIN32 //EWOULDBLOCK is not returned from recv on *nix/*bsd
         case EWOULDBLOCK:  
#endif
            InfoLog (<< "No data ready to read");
            return 0;
         case EINTR:
            InfoLog (<< "The call was interrupted by a signal before any data was read.");
            return 0;            
            break;
         case EIO:
            InfoLog (<< "I/O error");
            break;
         case EBADF:
            InfoLog (<< "fd is not a valid file descriptor or is not open for reading.");
            break;
         case EINVAL:
            InfoLog (<< "fd is attached to an object which is unsuitable for reading.");
            break;
         case EFAULT:
            InfoLog (<< "buf is outside your accessible address space.");
            break;
         default:
            InfoLog (<< "Some other error");
            break;
      }

      InfoLog (<< "Failed read on " << getSocket() << " " << strerror(e));
      Transport::error(e);
      
      return -1;
   }
   else if (bytesRead == 0)
   {
      InfoLog (<< "Connection closed by remote " << *this);
      return -1;
   }

   return bytesRead;
}


int 
TcpConnection::write( const char* buf, const int count )
{
   DebugLog (<< "Writing " << buf);

   assert(buf);
   assert(count > 0);

#if defined(WIN32)
   int bytesWritten = ::send(getSocket(), buf, count, 0);
#else
   int bytesWritten = 0;
   if(who().getType()==SCTP)
   {
#ifdef RESIP_USE_SCTP
      bytesWritten = ::sctp_sendmsg(getSocket(), 
                                    buf, 
                                    count, 
                                    0, // sockaddr override; not used
                                    0, // length of sockaddr override
                                    0, // opaque
                                    SCTP_UNORDERED, // flags
                                    0, // stream number
                                    64*Timer::T1, // ttl; setting this to 64T1
                                    0 // opaque
                                    );
#else
      assert(0);
#endif
   }
   else
   {
      bytesWritten = ::write(getSocket(), buf, count);
   }
#endif

   if (bytesWritten == INVALID_SOCKET)
   {
      int e = getErrno();
      InfoLog (<< "Failed write on " << getSocket() << " " << strerror(e));
      Transport::error(e);
      return -1;
   }
   
   return bytesWritten;
}

bool 
TcpConnection::hasDataToRead()
{
   return false;
}

bool 
TcpConnection::isGood()
{
   return true;
}

bool 
TcpConnection::isWritable()
{
   return true;
}

void 
TcpConnection::handlePeerAddressChange(const char* buffer, size_t length)
{
#ifdef RESIP_USE_SCTP
   InfoLog(<< "Got an SCTP event...");
   const struct sctp_paddr_change* change=reinterpret_cast<const struct sctp_paddr_change*>(buffer);
   if(length >= sizeof(sctp_paddr_change) && 
      change->spc_type==SCTP_PEER_ADDR_CHANGE)
   {
      InfoLog(<< "Got an SCTP peer address event...");
      // ?bwc? Does this sockaddr_storage have the port set?
      Tuple peerAddr(
                  *reinterpret_cast<const struct sockaddr*>(&change->spc_aaddr),
                  resip::SCTP);
      switch(change->spc_state)
      {
         case SCTP_ADDR_REMOVED:
            InfoLog(<< "Peer addr removed: " << peerAddr);
            removePeerAddress(peerAddr);
            break;
         case SCTP_ADDR_ADDED:
            InfoLog(<< "Peer addr added: " << peerAddr);
            addPeerAddress(peerAddr);
            break;
         case SCTP_ADDR_AVAILABLE:
            InfoLog(<< "Peer addr available for " << peerAddr);
            break;
         case SCTP_ADDR_UNREACHABLE:
            InfoLog(<< "Peer addr unreachable for " << peerAddr);
            break;
         case SCTP_ADDR_MADE_PRIM:
            InfoLog(<< "Peer addr made primary for " << peerAddr);
            break;
         case SCTP_ADDR_CONFIRMED:
            InfoLog(<< "Peer addr confirmed for " << peerAddr);
            break;
         default:
            InfoLog(<< "Unknown peer addr event for " << peerAddr);
            break;
            // ?bwc? It is probably not appropriate to remove this connection 
            // from the peer address map when the peer address becomes 
            // unreachable, since this would have a tendency to kill connection
            // reuse for multihoming. Although, it would be nice to do something
            // if all peer addresses become unreachable, but I guess the 
            // connection will fall down when that happens...
      }
   }
   else
   {
      ErrLog(<< "buffer did not contain a valid SCTP peer address event");
      assert(0);
   }
#else
   assert(0);
#endif
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
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
