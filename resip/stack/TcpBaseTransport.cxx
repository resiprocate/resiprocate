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
                                   Compression &compression)
   : InternalTransport(fifo, portNum, version, pinterface, 0, compression)
{
   mFd = InternalTransport::socket(TCP, version);
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


TcpBaseTransport::~TcpBaseTransport()
{
   //DebugLog (<< "Shutting down TCP Transport " << this << " " << mFd << " " << mInterface << ":" << port()); 
   
   // !jf! this is not right. should drain the sends before 
   while (mTxFifo.messageAvailable()) 
   {
      SendData* data = mTxFifo.getNext();
      InfoLog (<< "Throwing away queued data for " << data->destination);
      
      fail(data->transactionId);
      delete data;
   }
   DebugLog (<< "Shutting down " << mTuple);
   //mSendRoundRobin.clear(); // clear before we delete the connections
}

void
TcpBaseTransport::buildFdSet( FdSet& fdset)
{
   mConnectionManager.buildFdSet(fdset);
   fdset.setRead(mFd); // for the transport itself
}

void
TcpBaseTransport::processListen(FdSet& fdset)
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
               Transport::error(e);
         }
         return;
      }
      makeSocketNonBlocking(sock);
      
      
      DebugLog (<< "Received TCP connection from: " << tuple << " as fd=" << sock);
      createConnection(tuple, sock, true);
   }
}
/// @todo  only inspects the first element in ConnectionManager::getNextWrite(lame) 
void
TcpBaseTransport::processSomeWrites(FdSet& fdset)
{
   Connection* curr = mConnectionManager.getNextWrite(); 
   if (curr && fdset.readyToWrite(curr->getSocket()))
   {
      //DebugLog (<< "TcpBaseTransport::processSomeWrites() " << curr->getSocket());
      curr->performWrite();
   }
   else if (curr && fdset.hasException(curr->getSocket()))
   {
        int errNum = 0;
        int errNumSize = sizeof(errNum);
        getsockopt(curr->getSocket(),SOL_SOCKET,SO_ERROR,(char *)&errNum,(socklen_t *)&errNumSize);
        InfoLog (<< "Exception writing to socket " << curr->getSocket() << " code: " << errNum << "; closing connection");
        delete curr;
   }
}

void
TcpBaseTransport::processSomeReads(FdSet& fdset)
{
   Connection* currConnection = mConnectionManager.getNextRead(fdset); 
   if (currConnection)
   {
      if ( fdset.readyToRead(currConnection->getSocket()) ||
           currConnection->hasDataToRead() )
      {
         DebugLog (<< "TcpBaseTransport::processSomeReads() " << *currConnection);
         fdset.clear(currConnection->getSocket());

         int bytesRead = currConnection->read(mStateMachineFifo);
         DebugLog (<< "TcpBaseTransport::processSomeReads() " << " read=" << bytesRead);            
         if (bytesRead < 0)
         {
            DebugLog (<< "Closing connection bytesRead=" << bytesRead);
            delete currConnection;
         }
      }
      else if (fdset.hasException(currConnection->getSocket()))
      {
            int errNum = 0;
            int errNumSize = sizeof(errNum);
            getsockopt(currConnection->getSocket(),SOL_SOCKET,SO_ERROR,(char *)&errNum,(socklen_t *)&errNumSize);
            InfoLog (<< "Exception reading from socket " << currConnection->getSocket() << " code: " << errNum << "; closing connection");
            delete currConnection;
      }
   } 
}


void
TcpBaseTransport::processAllWriteRequests( FdSet& fdset )
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
         // attempt to open
         Socket sock = InternalTransport::socket( TCP, ipVersion());
         fdset.clear(sock);
         
         if ( sock == INVALID_SOCKET ) // no socket found - try to free one up and try again
         {
            int e = getErrno();
            InfoLog (<< "Failed to create a socket " << strerror(e));
            error(e);
            mConnectionManager.gc(ConnectionManager::MinimumGcAge); // free one up

            sock = InternalTransport::socket( TCP, ipVersion());
            if ( sock == INVALID_SOCKET )
            {
               int e = getErrno();
               WarningLog( << "Error in finding free filedescriptor to use. " << strerror(e));
               error(e);
               fail(data->transactionId);
               delete data;
               return;
            }
         }

         assert(sock != INVALID_SOCKET);
         const sockaddr& servaddr = data->destination.getSockaddr(); 
         
         DebugLog (<<"Opening new connection to " << data->destination);
         makeSocketNonBlocking(sock);         
         int e = connect( sock, &servaddr, data->destination.length() );

         // See Chapter 15.3 of Stevens, Unix Network Programming Vol. 1 2nd Edition
         if (e == INVALID_SOCKET)
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
                  InfoLog( << "Error on TCP connect to " <<  data->destination << ": " << strerror(err));
                  error(e);
                  fdset.clear(sock);
                  closeSocket(sock);
                  fail(data->transactionId);
                  delete data;
                  return;
               }
            }
         }
         
         // This will add the connection to the manager
         conn = createConnection(data->destination, sock, false);
         assert(conn);
         assert(conn->getSocket() >= 0);
         data->destination.mFlowKey = conn->getSocket(); // !jf!
      }
   
      if (conn == 0)
      {
         DebugLog (<< "Failed to create/get connection: " << data->destination);
         fail(data->transactionId);
         delete data;
      }
      else // have a connection
      {
         conn->requestWrite(data);
      }
   }
}

void
TcpBaseTransport::process(FdSet& fdSet)
{
   processAllWriteRequests(fdSet);
   if(fdSet.numReady > 0)
   {
      processSomeWrites(fdSet);
      processSomeReads(fdSet);
      processListen(fdSet);
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


