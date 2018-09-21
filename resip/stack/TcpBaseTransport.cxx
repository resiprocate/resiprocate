#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <memory>
#include "rutil/compat.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/NetNs.hxx"
#include "resip/stack/TcpBaseTransport.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

using namespace std;
using namespace resip;


const size_t TcpBaseTransport::MaxWriteSize = 4096;
const size_t TcpBaseTransport::MaxReadSize = 4096;

TcpBaseTransport::TcpBaseTransport(Fifo<TransactionMessage>& fifo,
                                   int portNum, IpVersion version,
                                   const Data& pinterface,
                                   AfterSocketCreationFuncPtr socketFunc,
                                   Compression &compression,
                                   unsigned transportFlags,
                                   const Data& netNs)
   : InternalTransport(fifo, portNum, version, pinterface, socketFunc, compression, transportFlags, netNs)
{
   if ( (mTransportFlags & RESIP_TRANSPORT_FLAG_NOBIND)==0 )
   {
#ifdef USE_NETNS
      DebugLog(<< "TcpBaseTransport: " << this << " netns: " << netNs);
      // setns here
      NetNs::setNs(netNs);
#endif
      mFd = InternalTransport::socket(TCP, version);
   }
}


TcpBaseTransport::~TcpBaseTransport()
{
   //DebugLog (<< "Shutting down TCP Transport " << this << " " << mFd << " " << mInterface << ":" << port());

   // !jf! this is not right. should drain the sends before
   while (mTxFifoOutBuffer.messageAvailable())
   {
      SendData* data = mTxFifoOutBuffer.getNext();
      InfoLog (<< "Throwing away queued data for " << data->destination);

      fail(data->transactionId, TransportFailure::TransportShutdown);
      delete data;
   }
   DebugLog (<< "Shutting down " << mTuple);
   //mSendRoundRobin.clear(); // clear before we delete the connections
   if(mPollGrp && mPollItemHandle)
   {
      mPollGrp->delPollItem(mPollItemHandle);
      mPollItemHandle=0;
   }
}

// called from constructor of TcpTransport
void
TcpBaseTransport::init()
{
   if ( (mTransportFlags & RESIP_TRANSPORT_FLAG_NOBIND)!=0 )
   {
      return;
   }

   //DebugLog (<< "Opening TCP " << mFd << " : " << this);

   int on = 1;
#if !defined(WIN32)
   if ( ::setsockopt ( mFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) )
#else
   if ( ::setsockopt ( mFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on)) )
#endif
   {
       int e = getErrno();
       InfoLog (<< "Couldn't set sockoptions SO_REUSEPORT | SO_REUSEADDR: " << strerror(e));
       error(e);
       throw Exception("Failed setsockopt", __FILE__,__LINE__);
   }

   bind();
   makeSocketNonBlocking(mFd);

   // do the listen, seting the maximum queue size for compeletly established
   // sockets -- on linux, tcp_max_syn_backlog should be used for the incomplete
   // queue size(see man listen)
   int e = listen(mFd,64 );

   if (e != 0 )
   {
      int e = getErrno();
      InfoLog (<< "Failed listen " << strerror(e));
      error(e);
      // !cj! deal with errors
      throw Transport::Exception("Address already in use", __FILE__,__LINE__);
   }
}

// ?kw?: when should this be called relative to init() above? merge?
void
TcpBaseTransport::setPollGrp(FdPollGrp *grp)
{
   if(mPollGrp && mPollItemHandle)
   {
      mPollGrp->delPollItem(mPollItemHandle);
      mPollItemHandle=0;
   }

   if ( mFd!=INVALID_SOCKET && grp)
   {
      mPollItemHandle = grp->addPollItem(mFd, FPEM_Read|FPEM_Edge, this);
      // above released by InternalTransport destructor
      // ?bwc? Is this really a good idea? If the InternalTransport d'tor is
      // freeing this, shouldn't InternalTransport::setPollGrp() handle 
      // creating it?
   }
   mConnectionManager.setPollGrp(grp);

   InternalTransport::setPollGrp(grp);
}

void
TcpBaseTransport::buildFdSet( FdSet& fdset)
{
   resip_assert( mPollGrp==NULL );
   mConnectionManager.buildFdSet(fdset);
   if ( mFd!=INVALID_SOCKET )
   {
      fdset.setRead(mFd); // for the transport itself (accept)
   }
   if(!shareStackProcessAndSelect())
   {
      mSelectInterruptor.buildFdSet(fdset);
   }
}

/**
    Returns 1 if created new connection, -1 if "bad" error,
    and 0 if nothing to do (EWOULDBLOCK)
**/
int
TcpBaseTransport::processListen()
{
   if (1)
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
               // !kw! this will happen every epoll cycle
               return 0;
            default:
               Transport::error(e);
         }
         return -1;
      }
      if(!configureConnectedSocket(sock))
      {
         throw Exception("Failed to configure connected socket", __FILE__,__LINE__);
      }
      makeSocketNonBlocking(sock);

      DebugLog (<< this << " Received TCP connection from: " << tuple << " mTuple: " << mTuple << " as fd=" << sock);

      if (mSocketFunc)
      {
         mSocketFunc(sock, transport(), __FILE__, __LINE__);
      }

      Connection* c = mConnectionManager.findConnection(tuple);
      if(!c)
      {
         createConnection(tuple, sock, true);
      }
      else if(false == c->isServer())
      {
         InfoLog( << "Have client connection for " << tuple << ", but got server one, recreate connection" );
         delete c;
         createConnection(tuple, sock, true);
      }
      else
      {
         InfoLog(<<"Someone probably sent a reciprocal SYN at us.");
         // ?bwc? Can we call this right after calling accept()?
         closeSocket(sock);
      }
   }
   return 1;
}

Connection*
TcpBaseTransport::makeOutgoingConnection(const Tuple &dest,
      TransportFailure::FailureReason &failReason, int &failSubCode)
{
   // attempt to open
#ifdef USE_NETNS
      NetNs::setNs(netNs());
#endif
   Socket sock = InternalTransport::socket( TCP, ipVersion());
   // fdset.clear(sock); !kw! removed as part of epoll impl

   if ( sock == INVALID_SOCKET ) // no socket found - try to free one up and try again
   {
      int err = getErrno();
      InfoLog (<< "Failed to create a socket " << strerror(err));
      error(err);
      if(mConnectionManager.gc(ConnectionManager::MinimumGcAge, 1) == 0)
      {
         mConnectionManager.gcWithTarget(1); // free one up
      }

#ifdef USE_NETNS
      NetNs::setNs(netNs());
#endif
      sock = InternalTransport::socket( TCP, ipVersion());
      if ( sock == INVALID_SOCKET )
      {
         err = getErrno();
         WarningLog( << "Error in finding free filedescriptor to use. " << strerror(err));
         error(err);
         failReason = TransportFailure::TransportNoSocket;
         failSubCode = err;
         return NULL;
      }
   }

   resip_assert(sock != INVALID_SOCKET);

   DebugLog (<<"Opening new connection to " << dest);
   char _sa[RESIP_MAX_SOCKADDR_SIZE];
   sockaddr *sa = reinterpret_cast<sockaddr*>(_sa);
   resip_assert(RESIP_MAX_SOCKADDR_SIZE >= mTuple.length());
   mTuple.copySockaddrAnyPort(sa);
#ifdef USE_NETNS
      NetNs::setNs(netNs());
#endif
   if(::bind(sock, sa, mTuple.length()) != 0)
   {
      WarningLog( << "Error in binding to source interface address. " << strerror(errno));
      failReason = TransportFailure::Failure;
      failSubCode = errno;
      return NULL;
   }
   if(!configureConnectedSocket(sock))
   {
      throw Exception("Failed to configure connected socket", __FILE__,__LINE__);
   }
   makeSocketNonBlocking(sock);
   if (mSocketFunc)
   {
      mSocketFunc(sock, transport(), __FILE__, __LINE__);
   }
   const sockaddr& servaddr = dest.getSockaddr();
   int ret = connect( sock, &servaddr, dest.length() );

   // See Chapter 15.3 of Stevens, Unix Network Programming Vol. 1 2nd Edition
   if (ret == SOCKET_ERROR)
   {
      int err = getErrno();

      switch (err)
      {
         case EINPROGRESS:
         case EAGAIN:
#if EAGAIN != EWOULDBLOCK
         case EWOULDBLOCK:  // Treat EGAIN and EWOULDBLOCK as the same: http://stackoverflow.com/questions/7003234/which-systems-define-eagain-and-ewouldblock-as-different-values
#endif
            break;
         default:
         {
            // !jf! this has failed
            InfoLog( << "Error on TCP connect to " <<  dest << ", err=" << err << ": " << strerror(err));
            error(err);
            //fdset.clear(sock);
            closeSocket(sock);
            failReason = TransportFailure::TransportBadConnect;
            failSubCode = err;
            return NULL;
         }
      }
   }

   // This will add the connection to the manager
   Connection *conn = createConnection(dest, sock, false);
   resip_assert(conn);
   conn->mFirstWriteAfterConnectedPending = true;

   return conn;
}

void
TcpBaseTransport::processAllWriteRequests()
{
   while (mTxFifoOutBuffer.messageAvailable())
   {
      SendData* data = mTxFifoOutBuffer.getNext();
      DebugLog (<< "Processing write for " << data->destination);

      // this will check by connectionId first, then by address
      Connection* conn = mConnectionManager.findConnection(data->destination);

      //DebugLog (<< "TcpBaseTransport::processAllWriteRequests() using " << conn);

#ifdef WIN32
      if(conn && mPollGrp && mPollGrp->getImplType() == FdPollGrp::PollImpl)
      {
         // Workaround for bug in WSAPoll implementation: see 
         // http://daniel.haxx.se/blog/2012/10/10/wsapoll-is-broken/
         // http://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/18769abd-fca0-4d3c-9884-1a38ce27ae90/wsapoll-and-nonblocking-connects-to-nonexistent-ports?forum=wsk
         // Note:  This is not an ideal solution - since we won't cleanup the connection until 
         //        after the connect has timedout and someone else tries to write to the same 
         //        destination.  However the only impact to users is that requests will take the 
         //        full 32 seconds transaction timeout to get an error vs the 21s connect timeout
         //        observered when using the select implemention (vs Poll).  This does save us from
         //        having to use some form of timer to periodically check the connect state though.
         if(conn->checkConnectionTimedout())
         {
            // If checkConnectionTimedout returns true, then connection is no longer available.
            // Clear conn so that we create a new connection below.
            conn = 0;
         }
      }
#endif

      // There is no connection yet, so make a client connection
      if (conn == 0 && 
          !data->destination.onlyUseExistingConnection &&
          data->command == 0)  // SendData commands (ie. close connection and enable flow timers) shouldn't cause new connections to form
      {
         TransportFailure::FailureReason failCode = TransportFailure::Failure;
         int subCode = 0;
         if((conn = makeOutgoingConnection(data->destination, failCode, subCode)) == 0)
         {
            DebugLog (<< "Failed to create connection: " << data->destination);
            fail(data->transactionId, failCode, subCode);
            delete data;
            // NOTE: We fail this one but don't give up on others in queue
            return;
         }
         resip_assert(conn->getSocket() != INVALID_SOCKET);
         data->destination.mFlowKey = conn->getSocket();
      }

      if (conn == 0)
      {
         DebugLog (<< "Failed to find connection: " << data->destination);
         fail(data->transactionId, TransportFailure::TransportNoExistConn, 0);
         delete data;
         // NOTE: We fail this one but don't give up on others in queue
      }
      else // have a connection
      {
         // Check if we have written anything or not on the connection.  If not, then this is either the first or 
         // a subsequent transaction trying to use this connection attempt - set TcpConnectState for this 
         // transaction to ConnectStarted
         if (conn->mFirstWriteAfterConnectedPending == true)
         {
             // Notify the transaction state that we have started a TCP connect, so that it can run a TCP connect timer
             setTcpConnectState(data->transactionId, TcpConnectState::ConnectStarted);
         }
         conn->requestWrite(data);
      }
   }
}

void
TcpBaseTransport::process()
{
   // called within SipStack's thread. There is some risk of
   // recursion here if connection starts doing anything fancy.
   // For backward-compat when not-epoll, don't handle transmit synchronously
   // now, but rather wait for the process() call
   if (mPollGrp)
   {
       processAllWriteRequests();
   }
   mStateMachineFifo.flush();
}

void
TcpBaseTransport::process(FdSet& fdSet)
{
   resip_assert( mPollGrp==NULL );

   processAllWriteRequests();

   // process the connections in ConnectionManager
   mConnectionManager.process(fdSet);

   // process our own listen/accept socket for incoming connections
   if (mFd!=INVALID_SOCKET && fdSet.readyToRead(mFd))
   {
      processListen();
   }

   mStateMachineFifo.flush();
}

void
TcpBaseTransport::processPollEvent(FdPollEventMask mask) 
{
   if (mask & FPEM_Read)
   {
      while(processListen() > 0);
   }
}

void
TcpBaseTransport::setRcvBufLen(int buflen)
{
   resip_assert(0);	// not implemented yet
   // need to store away the length and use when setting up new connections
}

void 
TcpBaseTransport::invokeAfterSocketCreationFunc() const
{
    // Call for base socket
    InternalTransport::invokeAfterSocketCreationFunc();
    // Call for each connection
    mConnectionManager.invokeAfterSocketCreationFunc();
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
