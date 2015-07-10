#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rutil/ResipAssert.h"

#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/Tuple.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/ParseBuffer.hxx"
#include "resip/stack/Transport.hxx"

#include "repro/HttpBase.hxx"
#include "repro/HttpConnection.hxx"
#include "repro/WebAdmin.hxx"
#include "rutil/WinLeakCheck.hxx"


using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO


HttpBase::~HttpBase()
{
#if defined(WIN32)
   closesocket(mFd);
#else
   close(mFd); 
#endif
   mFd=0;
   for( int i=0; i<MaxConnections; i++)
   {
      if ( mConnection[i] )
      {
         delete mConnection[i] ; mConnection[i]=0;
      }
   }
}


HttpBase::HttpBase( int port, IpVersion ipVer, const Data& realm, const resip::Data& ipAddr ):
   mRealm(realm),
   nextConnection(0),
   mTuple(ipAddr,port,ipVer,TCP,Data::Empty)
{
   // !rwm! [TODO] check that this works for IPv6   
   //assert( ipVer == V4 );

   sane = true;
   
   for ( int i=0 ; i<MaxConnections; i++)
   {
      mConnection[i]=0;
   }
   
#ifdef USE_IPV6
   mFd = ::socket(ipVer == V4 ? PF_INET : PF_INET6, SOCK_STREAM, 0);
#else
   mFd = ::socket(PF_INET, SOCK_STREAM, 0);
#endif
   
   if ( mFd == INVALID_SOCKET )
   {
      int e = getErrno();
      ErrLog (<< "Failed to create socket: " << strerror(e));
      sane = false;
      return;
   }

   DebugLog (<< "Creating fd=" << (int)mFd 
             << (ipVer == V4 ? " V4/" : " V6/") );
      
   int on = 1;
#if !defined(WIN32)
   if ( ::setsockopt ( mFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) )
#else
   if ( ::setsockopt ( mFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on)) )
#endif
   {
      int e = getErrno();
      ErrLog (<< "Couldn't set sockoptions SO_REUSEPORT | SO_REUSEADDR: " << strerror(e));
      sane = false;
      return;
   }

#ifdef USE_IPV6
#ifdef __linux__
   if (ipVer == V6)
   {
      if ( ::setsockopt(mFd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) )
      {
          int e = getErrno();
          ErrLog(<< "HttpBase::HttpBase: Couldn't set sockoptions IPV6_V6ONLY: " << strerror(e));
          sane = false;
          return;
      }
   }
#endif
#endif

   DebugLog (<< "Binding to " << Tuple::inet_ntop(mTuple));
   
   if ( ::bind( mFd, &mTuple.getMutableSockaddr(), mTuple.length()) == SOCKET_ERROR )
   {
      int e = getErrno();
      if ( e == EADDRINUSE )
      {
         ErrLog (<< mTuple << " already in use ");
      }
      else
      {
         ErrLog (<< "Could not bind to " << mTuple);
      }
      sane = false;
      return;
   }
   
   bool ok = makeSocketNonBlocking(mFd);
   if ( !ok )
   {
      ErrLog (<< "Could not make HTTP socket non-blocking " << port );
      sane = false;
      return;
   }
   
   // do the listen, seting the maximum queue size for compeletly established
   // sockets -- on linux, tcp_max_syn_backlog should be used for the incomplete
   // queue size(see man listen)
   int e = listen(mFd,5 );

   if (e != 0 )
   {
      int e = getErrno();
      InfoLog (<< "Failed listen " << strerror(e));
      sane = false;
      return;
   }
}


void 
HttpBase::buildFdSet(FdSet& fdset)
{ 
   fdset.setRead( mFd );
   
   for( int i=0; i<MaxConnections; i++)
   {
      if ( mConnection[i] )
      {
         mConnection[i]->buildFdSet(fdset);
      }
   }
}


void 
HttpBase::process(FdSet& fdset)
{
   if (fdset.readyToRead(mFd))
   {
      Tuple tuple(mTuple);
      struct sockaddr& peer = tuple.getMutableSockaddr();
      socklen_t peerLen = tuple.length();
      Socket sock = accept( mFd, &peer, &peerLen);
      if ( sock == SOCKET_ERROR )
      {
         int e = getErrno();
         switch (e)
         {
            case EAGAIN:
#if EAGAIN != EWOULDBLOCK
            case EWOULDBLOCK:  // Treat EGAIN and EWOULDBLOCK as the same: http://stackoverflow.com/questions/7003234/which-systems-define-eagain-and-ewouldblock-as-different-values
#endif
               // !jf! this can not be ready in some cases 
               return;
            default:
               ErrLog(<< "Some error reading from socket: " << e);
               // .bwc. This is almost certainly a bad assert that a nefarious
               // endpoint could hit.
               // assert(0); // Transport::error(e);
         }
         return;
      }
      makeSocketNonBlocking(sock);
      
      int c = nextConnection;
      nextConnection = ( nextConnection+1 ) % MaxConnections;
      
      if ( mConnection[c] )
      {
         delete mConnection[c]; mConnection[c] = 0;
      }
      
      mConnection[c] = new HttpConnection(*this,sock);
      
      DebugLog (<< "Received TCP connection as connection=" << c << " fd=" << (int)sock);
   }
    
   for( int i=0; i<MaxConnections; i++)
   {
      if ( mConnection[i] )
      {
         bool ok = mConnection[i]->process(fdset);
         if ( !ok )
         {
            delete mConnection[i]; mConnection[i]=0;
         }
      }
   }
}


void HttpBase::setPage( const Data& page, int pageNumber, int response, const Mime& type )
{
   for ( int i=0 ; i<MaxConnections; i++)
   {
      if ( mConnection[i] )
      {
         if ( mConnection[i]->mPageNumber == pageNumber )
         {
            mConnection[i]->setPage( page,response,type );
         }
      }
   }
}

bool HttpBase::isSane()
{
  return sane;
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
