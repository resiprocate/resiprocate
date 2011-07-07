#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include <memory>
#include "rutil/Socket.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
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
                                   unsigned transportFlags)
   : InternalTransport(fifo, portNum, version, pinterface, socketFunc, compression, transportFlags)
{
   if ( (mTransportFlags & RESIP_TRANSPORT_FLAG_NOBIND)==0 )
   {
      mFd = InternalTransport::socket(TCP, version);
   }
}


TcpBaseTransport::~TcpBaseTransport()
{
   //DebugLog (<< "Shutting down TCP Transport " << this << " " << mFd << " " << mInterface << ":" << port());

   // !jf! this is not right. should drain the sends before
   while (mTxFifo.messageAvailable())
   {
      SendData* data = mTxFifo.getNext();
      InfoLog (<< "Throwing away queued data for " << data->destination);

      fail(data->transactionId, TransportFailure::TransportShutdown);
      delete data;
   }
   DebugLog (<< "Shutting down " << mTuple);
   //mSendRoundRobin.clear(); // clear before we delete the connections
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
   assert(mPollGrp==NULL && grp!=NULL);
   if ( mFd!=INVALID_SOCKET )
   {
      mPollItemHandle = grp->addPollItem(mFd, FPEM_Read|FPEM_Edge, this);
      // above released by InternalTransport destructor
   }
   mPollGrp = grp;
   mConnectionManager.setPollGrp(grp);
}

void
TcpBaseTransport::buildFdSet( FdSet& fdset)
{
   assert( mPollGrp==NULL );
   mConnectionManager.buildFdSet(fdset);
   if ( mFd!=INVALID_SOCKET )
   {
      fdset.setRead(mFd); // for the transport itself (accept)
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
            case EWOULDBLOCK:
               // !jf! this can not be ready in some cases
               // !kw! this will happen every epoll cycle
               return 0;
            default:
               Transport::error(e);
         }
         return -1;
      }
      makeSocketNonBlocking(sock);

      DebugLog (<< "Received TCP connection from: " << tuple << " as fd=" << sock);

      if (mSocketFunc)
      {
         mSocketFunc(sock, transport(), __FILE__, __LINE__);
      }

      if(!mConnectionManager.findConnection(tuple))
      {
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
   Socket sock = InternalTransport::socket( TCP, ipVersion());
   // fdset.clear(sock); !kw! removed as part of epoll impl

   if ( sock == INVALID_SOCKET ) // no socket found - try to free one up and try again
   {
      int err = getErrno();
      InfoLog (<< "Failed to create a socket " << strerror(err));
      error(err);
      mConnectionManager.gc(ConnectionManager::MinimumGcAge, 1); // free one up

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

   assert(sock != INVALID_SOCKET);

   DebugLog (<<"Opening new connection to " << dest);
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
         case EWOULDBLOCK:
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
   assert(conn);
   conn->mRequestPostConnectSocketFuncCall = true;
   return conn;
}

void
TcpBaseTransport::processAllWriteRequests()
{
   while (mTxFifo.messageAvailable())
   {
      SendData* data = mTxFifo.getNext();
      DebugLog (<< "Processing write for " << data->destination);

      // this will check by connectionId first, then by address
      Connection* conn = mConnectionManager.findConnection(data->destination);

      //DebugLog (<< "TcpBaseTransport::processAllWriteRequests() using " << conn);

      // There is no connection yet, so make a client connection
      if (conn == 0 && !data->destination.onlyUseExistingConnection)
      {
         TransportFailure::FailureReason failCode = TransportFailure::Failure;
         int subCode = 0;
         if((conn=makeOutgoingConnection(data->destination, failCode, subCode)) == NULL)
         {
            fail(data->transactionId, failCode, subCode);
            delete data;
            return;	// .kw. WHY? What about messages left in queue?
         }
         assert(conn->getSocket() != INVALID_SOCKET);
         // .kw. why do below? We already have the conn, who uses key?
         data->destination.mFlowKey = conn->getSocket(); // !jf!
      }

      if (conn == 0)
      {
         DebugLog (<< "Failed to create/get connection: " << data->destination);
         fail(data->transactionId, TransportFailure::TransportNoExistConn, 0);
         delete data;
         // NOTE: We fail this one but don't give up on others in queue
      }
      else // have a connection
      {
         conn->requestWrite(data);
      }
   }
}

void
TcpBaseTransport::checkTransmitQueue()
{
   // called within SipStack's thread. There is some risk of
   // recursion here if connection starts doing anything fancy.
   // For backward-compat when not-epoll, don't handle transmit synchronously
   // now, but rather wait for the process() call
   if (mPollGrp)
   {
      processAllWriteRequests();
   }
}

void
TcpBaseTransport::process(FdSet& fdSet)
{
   assert( mPollGrp==NULL );

   processAllWriteRequests();

   // process the connections in ConnectionManager
   mConnectionManager.process(fdSet);

   // process our own listen/accept socket for incoming connections
   if (mFd!=INVALID_SOCKET && fdSet.readyToRead(mFd))
   {
      processListen();
   }
}

void
TcpBaseTransport::processPollEvent(FdPollEventMask mask) {
   if ( mask & FPEM_Read )
   {
      while ( processListen() > 0 )
         ;
   }
}

void
TcpBaseTransport::setRcvBufLen(int buflen)
{
   assert(0);	// not implemented yet
   // need to store away the length and use when setting up new connections
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
