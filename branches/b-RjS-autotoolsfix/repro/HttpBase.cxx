
#ifdef WIN32
#include <db_cxx.h>
#elif HAVE_CONFIG_H
#include "config.hxx"
#include DB_HEADER
#else 
#include <db4/db_cxx.h>
#endif

#include <cassert>

#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Tuple.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/ParseBuffer.hxx"

#include "repro/HttpBase.hxx"
#include "repro/HttpConnection.hxx"
#include "repro/WebAdmin.hxx"


using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO


HttpBase::~HttpBase()
{
   close(mFd); mFd=0;
   for( int i=0; i<MaxConnections; i++)
   {
      if ( mConnection[i] )
      {
         delete mConnection[i] ; mConnection[i]=0;
      }
   }
}


HttpBase::HttpBase( int port, IpVersion ipVer, const Data& realm ):
   mRealm(realm),
   nextConnection(0),
   mTuple(Data::Empty,port,ipVer,TCP,Data::Empty)
{
   assert( ipVer == V4 );
   
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
      InfoLog (<< "Failed to create socket: " << strerror(e));
      assert(0); // TODO 
      //throw Exception("Can't create HttpBase listner socket", __FILE__,__LINE__);
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
      InfoLog (<< "Couldn't set sockoptions SO_REUSEPORT | SO_REUSEADDR: " << strerror(e));
      assert(0); //throw Exception("Failed setsockopt", __FILE__,__LINE__);
   }
   
   DebugLog (<< "Binding to " << DnsUtil::inet_ntop(mTuple));
   
   if ( ::bind( mFd, &mTuple.getMutableSockaddr(), mTuple.length()) == SOCKET_ERROR )
   {
      int e = getErrno();
      if ( e == EADDRINUSE )
      {
         ErrLog (<< mTuple << " already in use ");
         assert(0); // throw Transport::Exception("port already in use", __FILE__,__LINE__);
      }
      else
      {
         ErrLog (<< "Could not bind to " << mTuple);
         assert(0); //throw Transport::Exception("Could not use port", __FILE__,__LINE__);
      }
   }
   
   bool ok = makeSocketNonBlocking(mFd);
   if ( !ok )
   {
      ErrLog (<< "Could not make HTTP socket non-blocking " << port );
       assert(0); // tthrow Transport::Exception("Failed making socket non-blocking", __FILE__,__LINE__);
   }
   
   // do the listen, seting the maximum queue size for compeletly established
   // sockets -- on linux, tcp_max_syn_backlog should be used for the incomplete
   // queue size(see man listen)
   int e = listen(mFd,5 );

   if (e != 0 )
   {
      int e = getErrno();
      InfoLog (<< "Failed listen " << strerror(e));
      // !cj! deal with errors
	   assert(0); // tthrow Transport::Exception("Address already in use", __FILE__,__LINE__);
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
            case EWOULDBLOCK:
               // !jf! this can not be ready in some cases 
               return;
            default:
               assert(0); // Transport::error(e);
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
