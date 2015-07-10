#include "rutil/ResipAssert.h"

#include <rutil/Data.hxx>
#include <rutil/Socket.hxx>
#include <resip/stack/Symbols.hxx>
#include <rutil/TransportType.hxx>
#include <rutil/Logger.hxx>
#include <resip/stack/Tuple.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/ParseBuffer.hxx>
#include <resip/stack/Transport.hxx>

#include "AppSubsystem.hxx"
#include "XmlRpcServerBase.hxx"
#include "XmlRpcConnection.hxx"
#include <rutil/WinLeakCheck.hxx>

using namespace clicktocall;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::CLICKTOCALL


XmlRpcServerBase::XmlRpcServerBase(int port, IpVersion ipVer) :
   mTuple(Data::Empty,port,ipVer,TCP,Data::Empty),
   mSane(true)
{   
#ifdef USE_IPV6
   mFd = ::socket(ipVer == V4 ? PF_INET : PF_INET6, SOCK_STREAM, 0);
#else
   mFd = ::socket(PF_INET, SOCK_STREAM, 0);
#endif
   
   if (mFd == INVALID_SOCKET)
   {
      int e = getErrno();
      logSocketError(e);
      ErrLog(<< "XmlRpcServerBase::XmlRpcServerBase: Failed to create socket: " << strerror(e));
      mSane = false;
      return;
   }

   DebugLog (<< "XmlRpcServerBase::XmlRpcServerBase: Creating fd=" << (int)mFd 
             << (ipVer == V4 ? " V4/" : " V6/") );
      
   int on = 1;
#if !defined(WIN32)
   if (::setsockopt(mFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
#else
   if (::setsockopt(mFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on)))
#endif
   {
      int e = getErrno();
      logSocketError(e);
      ErrLog(<< "XmlRpcServerBase::XmlRpcServerBase: Couldn't set sockoptions SO_REUSEPORT | SO_REUSEADDR: " << strerror(e));
      mSane = false;
      return;
   }
   
   DebugLog(<< "XmlRpcServerBase::XmlRpcServerBase: Binding to " << Tuple::inet_ntop(mTuple));
   
   if (::bind( mFd, &mTuple.getMutableSockaddr(), mTuple.length()) == SOCKET_ERROR)
   {
      int e = getErrno();
      logSocketError(e);
      if (e == EADDRINUSE)
      {
         ErrLog(<< "XmlRpcServerBase::XmlRpcServerBase: " << mTuple << " already in use ");
      }
      else
      {
         ErrLog(<< "XmlRpcServerBase::XmlRpcServerBase: Could not bind to " << mTuple);
      }
      mSane = false;
      return;
   }
   
   bool ok = makeSocketNonBlocking(mFd);
   if (!ok)
   {
      int e = getErrno();
      logSocketError(e);
      ErrLog(<< "XmlRpcServerBase::XmlRpcServerBase: Could not make HTTP socket non-blocking " << port);
      mSane = false;
      return;
   }
   
   // do the listen, seting the maximum queue size for compeletly established
   // sockets -- on linux, tcp_max_syn_backlog should be used for the incomplete
   // queue size(see man listen)
   int e = listen(mFd,5);

   if (e != 0)
   {
      int e = getErrno();
      InfoLog(<< "XmlRpcServerBase::XmlRpcServerBase: Failed listen " << strerror(e));
      mSane = false;
      return;
   }
}


XmlRpcServerBase::~XmlRpcServerBase()
{
#if defined(WIN32)
   closesocket(mFd);
#else
   close(mFd); 
#endif
   mFd = 0;
   ConnectionMap::iterator it = mConnections.begin();
   for(; it != mConnections.end(); it++)
   {
      delete it->second; 
   }
}


void 
XmlRpcServerBase::buildFdSet(FdSet& fdset)
{ 
   mSelectInterruptor.buildFdSet(fdset);

   fdset.setRead(mFd);  // listen socket for server

   ConnectionMap::iterator it = mConnections.begin();
   for(; it != mConnections.end(); it++)
   {
      it->second->buildFdSet(fdset);
   }
}


void 
XmlRpcServerBase::process(FdSet& fdset)
{
   // Process Response fifo first
   while (mResponseFifo.messageAvailable())
   {
      ResponseInfo* responseInfo = mResponseFifo.getNext();
      ConnectionMap::iterator it = mConnections.find(responseInfo->getConnectionId());
      if(it != mConnections.end())
      {
         it->second->sendResponse(responseInfo->getRequestId(), responseInfo->getResponseData(), responseInfo->getIsFinal());
         delete responseInfo;
      }
   }

   mSelectInterruptor.process(fdset);

   if (fdset.readyToRead(mFd))
   {
      Tuple tuple(mTuple);
      struct sockaddr& peer = tuple.getMutableSockaddr();
      socklen_t peerLen = tuple.length();
      Socket sock = accept( mFd, &peer, &peerLen);
      if (sock == SOCKET_ERROR)
      {
         int e = getErrno();
         switch (e)
         {
            case EWOULDBLOCK:
               return;
            default:
               logSocketError(e);
               ErrLog(<< "XmlRpcServerBase::process: Some error reading from socket: " << e);
         }
         return;
      }
      makeSocketNonBlocking(sock);
      
      if(mConnections.size() == MaxConnections)
      {
         closeOldestConnection();
      }

      XmlRpcConnection* connection = new XmlRpcConnection(*this,sock);
      mConnections[connection->getConnectionId()] = connection;
      
      DebugLog (<< "XmlRpcServerBase::process: Received TCP connection as connection=" << connection->getConnectionId() << " fd=" << (int)sock);
   }

   // Call process on each connection
   ConnectionMap::iterator it = mConnections.begin();
   for(; it != mConnections.end(); )
   {
      bool ok = it->second->process(fdset);
      if (!ok)
      {
         delete it->second;
         mConnections.erase(it++);
      }
      else
      {
         it++;
      }
   }
}


void 
XmlRpcServerBase::sendResponse(unsigned int connectionId,
                               unsigned int requestId, 
                               const Data& responseData,
                               bool isFinal)
{
   mResponseFifo.add(new ResponseInfo(connectionId, requestId, responseData, isFinal));
   mSelectInterruptor.interrupt();
}

bool 
XmlRpcServerBase::isSane()
{
  return mSane;
}

void
XmlRpcServerBase::closeOldestConnection()
{
   if(mConnections.empty()) return;

   // Oldest Connection is the one with the lowest Id
   ConnectionMap::iterator lowestConnectionIdIt = mConnections.end();
   ConnectionMap::iterator it = mConnections.begin();
   for(; it != mConnections.end(); it++)
   {
      if(it->second->getConnectionId() < lowestConnectionIdIt->second->getConnectionId())
      {
         lowestConnectionIdIt = it;
      }
   }
   delete lowestConnectionIdIt->second;
   mConnections.erase(lowestConnectionIdIt);
}

void
XmlRpcServerBase::logSocketError(int e)
{
   switch (e)
   {
      case EAGAIN:
         InfoLog (<< "No data ready to read" << strerror(e));
         break;
      case EINTR:
         InfoLog (<< "The call was interrupted by a signal before any data was read : " << strerror(e));
         break;
      case EIO:
         InfoLog (<< "I/O error : " << strerror(e));
         break;
      case EBADF:
         InfoLog (<< "fd is not a valid file descriptor or is not open for reading : " << strerror(e));
         break;
      case EINVAL:
         InfoLog (<< "fd is attached to an object which is unsuitable for reading : " << strerror(e));
         break;
      case EFAULT:
         InfoLog (<< "buf is outside your accessible address space : " << strerror(e));
         break;

#if defined(WIN32)
      case WSAENETDOWN: 
         InfoLog (<<" The network subsystem has failed.  ");
         break;
      case WSAEFAULT:
         InfoLog (<<" The buf or from parameters are not part of the user address space, "
                   "or the fromlen parameter is too small to accommodate the peer address.  ");
         break;
      case WSAEINTR: 
         InfoLog (<<" The (blocking) call was canceled through WSACancelBlockingCall.  ");
         break;
      case WSAEINPROGRESS: 
         InfoLog (<<" A blocking Windows Sockets 1.1 call is in progress, or the "
                   "service provider is still processing a callback function.  ");
         break;
      case WSAEINVAL: 
         InfoLog (<<" The socket has not been bound with bind, or an unknown flag was specified, "
                   "or MSG_OOB was specified for a socket with SO_OOBINLINE enabled, "
                   "or (for byte stream-style sockets only) len was zero or negative.  ");
         break;
      case WSAEISCONN : 
         InfoLog (<<"The socket is connected. This function is not permitted with a connected socket, "
                  "whether the socket is connection-oriented or connectionless.  ");
         break;
      case WSAENETRESET:
         InfoLog (<<" The connection has been broken due to the keep-alive activity "
                  "detecting a failure while the operation was in progress.  ");
         break;
      case WSAENOTSOCK :
         InfoLog (<<"The descriptor is not a socket.  ");
         break;
      case WSAEOPNOTSUPP:
         InfoLog (<<" MSG_OOB was specified, but the socket is not stream-style such as type "
                   "SOCK_STREAM, OOB data is not supported in the communication domain associated with this socket, "
                   "or the socket is unidirectional and supports only send operations.  ");
         break;
      case WSAESHUTDOWN:
         InfoLog (<<"The socket has been shut down; it is not possible to recvfrom on a socket after "
                  "shutdown has been invoked with how set to SD_RECEIVE or SD_BOTH.  ");
         break;
      case WSAEMSGSIZE:
         InfoLog (<<" The message was too large to fit into the specified buffer and was truncated.  ");
         break;
      case WSAETIMEDOUT: 
         InfoLog (<<" The connection has been dropped, because of a network failure or because the "
                  "system on the other end went down without notice.  ");
         break;
      case WSAECONNRESET : 
         InfoLog (<<"Connection reset ");
         break;

	   case WSAEWOULDBLOCK:
         DebugLog (<<"Would Block ");
         break;

      case WSAEHOSTUNREACH:
         InfoLog (<<"A socket operation was attempted to an unreachable host ");
         break;
      case WSANOTINITIALISED:
         InfoLog (<<"Either the application has not called WSAStartup or WSAStartup failed. "
                  "The application may be accessing a socket that the current active task does not own (that is, trying to share a socket between tasks),"
                  "or WSACleanup has been called too many times.  ");
         break;
      case WSAEACCES:
         InfoLog (<<"An attempt was made to access a socket in a way forbidden by its access permissions ");
         break;
      case WSAENOBUFS:
         InfoLog (<<"An operation on a socket could not be performed because the system lacked sufficient "
                  "buffer space or because a queue was full");
         break;
      case WSAENOTCONN:
         InfoLog (<<"A request to send or receive data was disallowed because the socket is not connected "
                  "and (when sending on a datagram socket using sendto) no address was supplied");
         break;
      case WSAECONNABORTED:
         InfoLog (<<"An established connection was aborted by the software in your host computer, possibly "
                  "due to a data transmission time-out or protocol error");
         break;
      case WSAEADDRNOTAVAIL:
         InfoLog (<<"The requested address is not valid in its context. This normally results from an attempt to "
                  "bind to an address that is not valid for the local computer");
         break;
      case WSAEAFNOSUPPORT:
         InfoLog (<<"An address incompatible with the requested protocol was used");
         break;
      case WSAEDESTADDRREQ:
         InfoLog (<<"A required address was omitted from an operation on a socket");
         break;
      case WSAENETUNREACH:
         InfoLog (<<"A socket operation was attempted to an unreachable network");
         break;

#endif

      default:
         InfoLog (<< "Some other error (" << e << "): " << strerror(e));
         break;
   }
}


/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
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

