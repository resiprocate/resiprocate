#include <memory>
#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/TcpTransport.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

using namespace std;
using namespace resip;

const size_t TcpTransport::MaxWriteSize = 4096;
const size_t TcpTransport::MaxReadSize = 4096;


TcpTransport::TcpTransport(const Data& sendhost, int portNum, const Data& nic, Fifo<Message>& fifo) :
   Transport(sendhost, portNum, nic , fifo)
{
   DebugLog (<< "Creating TCP transport on " << sendhost << ":" << portNum << " " << nic);

   mSendPos = mSendRoundRobin.end();
   mFd = socket(PF_INET, SOCK_STREAM, 0);
   
   if ( mFd == INVALID_SOCKET )
   {
      InfoLog (<< "Failed to open socket: " << portNum);
      throw Exception("Can't create TcpTransport", __FILE__,__LINE__);
   }
   assert(mFd > 2);
   
#ifndef WIN32
   int on = 1;
   if ( ::setsockopt ( mFd, SOL_SOCKET, SO_REUSEADDR, // | SO_REUSEPORT,
                       (void*)(&on), sizeof ( on )) )
   {
      int err = errno;
      InfoLog (<< "Couldn't set sockoptions SO_REUSEPORT | SO_REUSEADDR: "
               << strerror(err));
      throw Exception("Failed setsockopt", __FILE__,__LINE__);
   }
#endif
   
   //makeSocketNonBlocking(mFd);

   sockaddr_in addr;
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY); // !jf!--change when nic specfied
   addr.sin_port = htons(portNum);

   if ( bind( mFd, (struct sockaddr*) &addr, sizeof(addr)) == SOCKET_ERROR )
   {
      int err = errno;
      ErrLog (<< strerror(err) << " for TCP transport port " << portNum);
      throw Exception(strerror(err), __FILE__,__LINE__);
   }

   makeSocketNonBlocking(mFd);

   // do the listen, seting the maximum queue size for compeletly established
   // sockets -- on linux, tcp_max_syn_backlog should be used for the incomplete
   // queue size(see man listen)
   int e = listen(mFd,64 );

   if (e != 0 )
   {
      //int err = errno;
      // !cj! deal with errors
	  throw Exception("Address already in use", __FILE__,__LINE__);
   }
}


TcpTransport::~TcpTransport()
{
   DebugLog (<< "Shutting down TCP Transport " << mFd << " " << this);   //!rm!
   
   //::shutdown(mFd, SHUT_RDWR);
   closesocket(mFd);
   // !jf! this is not right. should drain the sends before 
   while (mTxFifo.messageAvailable()) mTxFifo.getNext();
   
   mSendRoundRobin.clear(); // clear before we delete the connections
}


void
TcpTransport::buildFdSet( FdSet& fdset)
{
   for (ConnectionMap::Map::iterator it = mConnectionMap.mConnections.begin();
        it != mConnectionMap.mConnections.end(); it++)
   {
      fdset.setRead(it->second->getSocket());

      // !cj! only should add this if
      //there is data to write
      //fdset.setWrite(it->second->getSocket());
   }
   fdset.setRead(mFd);
}


void
TcpTransport::processListen(FdSet& fdset)
{
   if (fdset.readyToRead(mFd))
   {
      struct sockaddr_in peer;

      socklen_t peerLen = sizeof(peer);
      Socket sock = accept( mFd, (struct sockaddr*)&peer,&peerLen);
      if ( sock == -1 )
      {
         int err = errno;
         InfoLog( << "Error on accept: " << mFd << " : " << " : " << this << " " << strerror(err));
         return;
      }

      makeSocketNonBlocking(sock);

      Transport::Tuple who;
      who.ipv4 = peer.sin_addr;
      who.port = ntohs(peer.sin_port);
      who.transportType = Transport::TCP;
      who.transport = this;
      mConnectionMap.add(who, sock);

      DebugLog (<< "Received TCP connection from: "
               << inet_ntoa(who.ipv4)
               << ":" <<  who.port); //!rm!
   }
}


bool
TcpTransport::processRead(Connection* c)
{
   std::pair<char*, size_t> writePair = c->getWriteBuffer();

   size_t bytesToRead = resipMin(writePair.second, TcpTransport::MaxReadSize);

#ifndef WIN32
   int bytesRead = ::read(c->getSocket(), writePair.first, bytesToRead);
#else
   int bytesRead = ::recv(c->getSocket(), writePair.first, bytesToRead, 0);
#endif
   DebugLog (<< "received " << bytesRead << " bytes on " << *c);
   if (bytesRead <= 0)
   {
      int err = errno;
      DebugLog(<< "TcpTransport::processRead failed for " << *c << " " << strerror(err));
      return false;
   }
   if (c->process(bytesRead, mStateMachineFifo))
   {
      mConnectionMap.touch(c);
      return true;
   }
   else
   {
      DebugLog (<< "TcpTransport::processRead failed due to bad message " << *c );
      return false;
   }
}


void
TcpTransport::processAllReads(FdSet& fdset)
{
   if (!mConnectionMap.mConnections.empty())
   {
      for (Connection* c = mConnectionMap.mPostOldest.mYounger;
           c != &mConnectionMap.mPreYoungest; c = c->mYounger)
      {
         if (fdset.readyToRead(c->getSocket()))
         {
            if (processRead(c))
            {
               return;
            }
            else
            {
               DebugLog(<< "TcpTransport::processAllReads -- closing: " << *c);
               mConnectionMap.close(c->mWho);
               return;
            }
         }
      }
   }
}


void
TcpTransport::processAllWrites( FdSet& fdset )
{
   if (mTxFifo.messageAvailable())
   {
      SendData* data = mTxFifo.getNext();
      Connection* conn = data->destination.connection;
      if (conn == 0)
      {
         conn = mConnectionMap.get(data->destination);
      }

      if (conn == 0)
      {
         // attempt to open
         Socket sock = socket( AF_INET, SOCK_STREAM, 0 );

         if ( sock == -1 ) // no socket found - try to free one up and try again
         {
            // !dlb! does the file descriptor become available immediately?
            mConnectionMap.gc(ConnectionMap::MinLastUsed); // free one up
            sock = socket( AF_INET, SOCK_STREAM, 0 ); // try again
         }

         if ( sock == -1 )
         {
            InfoLog( << "Error in TCP finding free socket to use" );
         }
         else
         {
            struct sockaddr_in servaddr;
            ::memset( &servaddr, 0, sizeof(servaddr) );
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons( data->destination.port);
            servaddr.sin_addr =  data->destination.ipv4;

            //makeSocketNonBlocking(sock);

            DebugLog (<<"Trying to open new connection");

            int e = connect( sock, (struct sockaddr *)&servaddr, sizeof(servaddr) );
            if (e < 0)
            {
               int err = errno;
               InfoLog( << "Error on TCP connect to " <<  data->destination << ": " << strerror(err));
               close(sock);
            }
            else
            {
#if 1
               // succeeded, add the connection
               data->destination.transport = this;
               mConnectionMap.add( data->destination, sock);
               conn = mConnectionMap.get(data->destination);
               assert( conn );
               DebugLog (<< "Opened new connection to "
                         << inet_ntoa(data->destination.ipv4)
                         << ":" << data->destination.port ); // !rm!
#else
               // !rk! Since we made the connect(2) non-blocking we
               // need to hang out until it succeeds or "times out".  This
               // is still crap because it blocks the stack so we should
               // add the fds to the fdset and process when ready.
               //
               // connect(2) must be non-blocking because a TCP RST may
               // not interrupt a blocking connect and you could end up
               // waiting for a TCP timeout (e.g. 120s) which is likely
               // longer than a SIP timeout.  The consequence of this is
               // that you might not try other suitable transports after
               // TCP fails.

               fd_set rset, wset;
               FD_ZERO(&rset);
               FD_ZERO(&wset);
               FD_SET(sock, &rset);
               wset = rset;
               struct timeval timeout;
               timeout.tv_sec = 16 * Timer::T1 / 1000; /* !rk! change? */
               timeout.tv_usec = 0;

               while ((e = select(sock+1, &rset, &wset, NULL, &timeout)) < 0)
               {
                  // may not be portable outside Linux as Linux will set
                  // the timeout be be the remaining time on EINTR
                  if (errno != EINTR)
                  {
                     break;
                  }
               }
               if (e < 1)
               {
                  InfoLog( << "Timeout on TCP connect to " <<  data->destination);
                  close(sock);
               }
               else
               {
                  // succeeded, add the connection
                  data->destination.transport = this;
                  mConnectionMap.add( data->destination, sock);
                  conn = mConnectionMap.get(data->destination);
                  assert( conn );
                  DebugLog (<< "Opened new connection to "
                           << inet_ntoa(data->destination.ipv4)
                           << ":" << data->destination.port ); // !rm!
               }
#endif
            }
         }
      }
      
      if (conn == 0)
      {
         DebugLog (<< "Failed to create/get connection: " << data->destination);
         fail(data->transactionId);
         delete data;
         return;
      }

      assert(conn);
      if (conn->mOutstandingSends.empty())
      {
         DebugLog (<< "Adding " << *conn << " to send roundrobin");
         mSendRoundRobin.push_back(conn);
         conn->mSendPos = 0;
      }

      conn->mOutstandingSends.push_back(data);
   }

   sendFromRoundRobin(fdset);
}


void
TcpTransport::sendFromRoundRobin(FdSet& fdset)
{
   if (!mSendRoundRobin.empty())
   {
      ConnectionList::iterator rrPos = mSendPos;
      do
      {
         if (mSendPos == mSendRoundRobin.end())
         {
            mSendPos = mSendRoundRobin.begin();
         }
         if (true) // !cj! - need add to fd set if (fdset.readyToWrite((*mSendPos)->getSocket()))
         {
            if (processWrite(*mSendPos))
            {
               if ((*mSendPos)->mOutstandingSends.empty())
               {
                  DebugLog (<< "Finished send, removing " << **mSendPos << " from roundrobin");
                  mSendPos = mSendRoundRobin.erase(mSendPos);
               }
               else
               {
                  mSendPos++;
               }
               return;
            }
            else
            {
               DebugLog (<< "Failed send, removing " << **mSendPos << " from roundrobin");
               DebugLog(<< "TcpTransport::processAllWrites -- closing: " << **mSendPos);
               mConnectionMap.close((*mSendPos)->mWho);
               mSendPos = mSendRoundRobin.erase(mSendPos);
               return;
            }
         }
         else
         {
            mSendPos++;
         }
      } while(mSendPos != rrPos && !mSendRoundRobin.empty());
   }
}


bool
TcpTransport::processWrite(Connection* c)
{
   assert(!c->mOutstandingSends.empty());
   SendData* data = c->mOutstandingSends.front();

   size_t bytesToWrite = resipMin(data->data.size() - c->mSendPos, TcpTransport::MaxWriteSize);

#ifndef WIN32
   int bytesWritten = ::write(c->getSocket(), data->data.data() + c->mSendPos, bytesToWrite);
#else
   int bytesWritten = ::send(c->getSocket(), data->data.data() + c->mSendPos, bytesToWrite, 0);
#endif
   int err = errno;
   if (bytesWritten <= 0)
   {
      DebugLog (<< "Failed write to " << *c << " :" << strerror(err));
      fail(data->transactionId);
      return false;
   }
   else if ((size_t)bytesWritten < data->data.size() - c->mSendPos)
   {
      DebugLog (<< "##Wrote " << bytesWritten << " to " << *c);
      c->mSendPos += bytesWritten;
   }
   else
   {
      DebugLog (<< "##Finished write " << bytesWritten << " to " << *c);
      DebugLog (<< "Sent: " << endl << data->data);
      c->mOutstandingSends.pop_front();
      DebugLog (<< "mOutstandingSends.size() " << c->mOutstandingSends.size());
      c->mSendPos = 0;
      delete data;
   }
   mConnectionMap.touch(c);
   return true;
}


void
TcpTransport::process(FdSet& fdSet)
{
   if (mTxFifo.messageAvailable())
   {
      DebugLog(<<"mTxFifo:size: " << mTxFifo.size());
      assert(mTxFifo.size() < 1000); // !jf!
   }
   processAllWrites(fdSet);
   processListen(fdSet);
   processAllReads(fdSet);
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
