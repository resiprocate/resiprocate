#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include <memory>
#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/TcpBaseTransport.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

using namespace std;
using namespace resip;

const size_t TcpBaseTransport::MaxWriteSize = 4096;
const size_t TcpBaseTransport::MaxReadSize = 4096;


TcpBaseTransport::TcpBaseTransport(Fifo<Message>& fifo, int portNum, const Data& pinterface, bool ipv4)
   : Transport(fifo, portNum, pinterface, ipv4)
{
   mFd = Transport::socket(TCP, ipv4);
   InfoLog (<< "Opening TCP " << mFd << " : " << this);
   
#if !defined(WIN32)
   int on = 1;
   if ( ::setsockopt ( mFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) )
   {
      InfoLog (<< "Couldn't set sockoptions SO_REUSEPORT | SO_REUSEADDR: " << strerror(errno));
      throw Exception("Failed setsockopt", __FILE__,__LINE__);
   }
#endif

   bind();
   makeSocketNonBlocking(mFd);
   
   // do the listen, seting the maximum queue size for compeletly established
   // sockets -- on linux, tcp_max_syn_backlog should be used for the incomplete
   // queue size(see man listen)
   int e = listen(mFd,64 );

   if (e != 0 )
   {
      InfoLog (<< "Failed listen " << strerror(errno));

      // !cj! deal with errors
	  throw Exception("Address already in use", __FILE__,__LINE__);
   }
}


TcpBaseTransport::~TcpBaseTransport()
{
   InfoLog (<< "Shutting down TCP Transport " << mFd << " " << this);   //!rm!
   
   //::shutdown(mFd, SHUT_RDWR);
   closesocket(mFd);

   // !jf! this is not right. should drain the sends before 
   while (mTxFifo.messageAvailable()) 
   {
      SendData* data = mTxFifo.getNext();
      InfoLog (<< "Throwing away queued data for " << data->destination);
      
      fail(data->transactionId);
      delete data;
   }
   
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
      struct sockaddr peer;
      
      socklen_t peerLen = sizeof(peer);
      Socket sock = accept( mFd, &peer, &peerLen);
      if ( sock == -1 )
      {
         // !jf! this can not be ready in some cases 
         Transport::error();
         return;
      }
      makeSocketNonBlocking(sock);

      Tuple who(peer, transport());
      who.transport = this;

      DebugLog (<< "Received TCP connection from: " << who);
      createConnection(who, sock, true);
   }
}

void
TcpBaseTransport::processSomeWrites(FdSet& fdset)
{
   // !jf! may want to do a roundrobin later
   Connection* curr = mConnectionManager.getNextWrite(); 
   if (curr && fdset.readyToWrite(curr->getSocket()))
   {
      //DebugLog (<< "TcpBaseTransport::processSomeWrites() " << curr->getSocket());
      curr->performWrite();
   }
}

void
TcpBaseTransport::processSomeReads(FdSet& fdset)
{
   Connection* curr = mConnectionManager.getNextRead(); 
   if (curr)
   {
      if (fdset.readyToRead(curr->getSocket()))
      {
         //DebugLog (<< "TcpBaseTransport::processSomeReads() " << curr->getSocket());
         //FD_CLR(curr->getSocket(), &fdset.read);

         std::pair<char*, size_t> writePair = curr->getWriteBuffer();
         size_t bytesToRead = resipMin(writePair.second, size_t(Connection::ChunkSize));
            
         int bytesRead = curr->read(writePair.first, bytesToRead);
         if (bytesRead == INVALID_SOCKET)
         {
            delete curr;
         }
         else if (bytesRead > 0)
         {
            curr->performRead(bytesRead, mStateMachineFifo);
         }
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
      // There is no connection yet, so make a client connection
      if (conn == 0)
      {
         // attempt to open
         Socket sock = Transport::socket( TCP, mV4);
         
         if ( sock == INVALID_SOCKET ) // no socket found - try to free one up and try again
         {
            InfoLog (<< "Failed to create a socket " << strerror(errno));
            mConnectionManager.gc(ConnectionManager::MinLastUsed); // free one up

            sock = Transport::socket( TCP, mV4);
            if ( sock == INVALID_SOCKET )
            {
               WarningLog( << "Error in finding free filedescriptor to use. " << strerror(errno));
               fail(data->transactionId);
               delete data;
               return;
            }
         }

         assert(sock != INVALID_SOCKET);
         const sockaddr& servaddr = data->destination.getSockaddr(); 
         
         DebugLog (<<"Opening new connection to " << data->destination);
         makeSocketNonBlocking(sock);         
         int e = connect( sock, &servaddr, sizeof(servaddr) );

         // See Chapter 15.3 of Stevens, Unix Network Programming Vol. 1 2nd Edition
         if (e == INVALID_SOCKET)
         {
            if (errno != EINPROGRESS)
            {
               // !jf! this has failed
               InfoLog( << "Error on TCP connect to " <<  data->destination << ": " << strerror(errno));
               close(sock);
               fail(data->transactionId);
               delete data;
               return;
            }
         }

         // This will add the connection to the manager
         conn = createConnection(data->destination, sock, false);
         assert(conn);
         data->destination.transport = this;
         data->destination.connectionId = conn->getId(); // !jf!
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
   processSomeWrites(fdSet);
   processSomeReads(fdSet);
   processListen(fdSet);
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


