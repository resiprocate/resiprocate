
#ifndef WIN32
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

#include <memory>
#include "sip2/util/Data.hxx"
#include "sip2/util/Socket.hxx"
#include "sip2/util/Logger.hxx"
#include "sip2/sipstack/TcpTransport.hxx"
#include "sip2/sipstack/SipMessage.hxx"
#include "sip2/sipstack/Preparse.hxx"


#define VOCAL_SUBSYSTEM Subsystem::SIP

using namespace std;
using namespace Vocal2;

const int TcpTransport::MaxBufferSize = 1024;


TcpTransport::TcpTransport(const Data& sendhost, int portNum, const Data& nic, Fifo<Message>& fifo) : 
   Transport(sendhost, portNum, nic , fifo)
{
   mFd = socket(PF_INET, SOCK_STREAM, 0);

   if ( mFd == INVALID_SOCKET )
   {
      InfoLog (<< "Failed to open socket: " << portNum);
   }
   
   sockaddr_in addr;
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY); // !jf! 
   addr.sin_port = htons(portNum);
   
   if ( bind( mFd, (struct sockaddr*) &addr, sizeof(addr)) == SOCKET_ERROR )
   {
      int err = errno;
      if ( err == EADDRINUSE )
      {
         InfoLog (<< "Address already in use");
      }
      else
      {
         InfoLog (<< "Could not bind to port: " << portNum);
      }
      
      throw Exception("Address already in use", __FILE__,__LINE__);
   }

   // bind it to the local addr

   // make non blocking 
#if WIN32
   unsigned long block = 0;
   int errNoBlock = ioctlsocket( mFd, FIONBIO , &block );
   assert( errNoBlock == 0 );
#else
   int flags  = fcntl( mFd, F_GETFL, 0);
   int errNoBlock = fcntl(mFd,F_SETFL, flags| O_NONBLOCK );
   assert( errNoBlock == 0 );
#endif

   // do the listen
   int e = listen( mFd , /* qued requests */ 64 );
   if (e != 0 )
   {
      //int err = errno;
      // !cj! deal with errors
      assert(0);
   }
}


TcpTransport::~TcpTransport()
{
}


void 
TcpTransport::buildFdSet( FdSet& fdset)
{
   for (ConnectionMap::Map::iterator it = mConnectionMap.mConnections.begin();
        it != mConnectionMap.mConnections.end(); it++)
   {
      fdset.setRead(it->second->getSocket());
      fdset.setWrite(it->second->getSocket());
   }
}

void 
TcpTransport::processListen(FdSet& fdset)
{
   assert( mFd );
	
   if (fdset.readyToRead(mFd))
   {
      struct sockaddr_in peer;
		
      socklen_t peerLen=sizeof(peer);
      Socket s = accept( mFd, (struct sockaddr*)&peer,&peerLen);
      if ( s == -1 )
      {
         //int err = errno;
         // !cj!
         assert(0);
      }

      Transport::Tuple who;
      who.ipv4 = peer.sin_addr;
      who.port = ntohs(peer.sin_port);
      who.transportType = Transport::TCP;
      who.transport = this;
      mConnectionMap.add(who, s);
   }
}


bool
TcpTransport::processRead(ConnectionMap::Connection* c)
{
   c->allocateBuffer(MaxBufferSize);
   int bytesRead = read(c->getSocket(), c->mBuffer + c->mBytesRead, c->mBufferSize - c->mBytesRead);
   if (bytesRead < 0)
   {
      //socket cleanup code
      return false;
   }   

   if(c->process(bytesRead, mStateMachineFifo, mPreparse, MaxBufferSize))
   {
      // socket cleanup code
      return false;
      
   }
   mConnectionMap.touch(c);
   return true;
}



void
TcpTransport::processAllReads(FdSet& fdset)
{
   if (mConnectionMap.mConnections.empty())
   {
      return;
   }

   for (ConnectionMap::Connection * c = mConnectionMap.mPostOldest.mYounger;
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
            mConnectionMap.close(c->mWho);
            return;
         }
      }
   }
}

void
TcpTransport::processAllWrites( FdSet& fdset )
{
   // how do we know that buffer won't get deleted on us !jf!

   //!dcm! have completely punted on backpressure tracking
   while(true)
   {
      if (mTxFifo.messageAvailable())
      {
         SendData* data = mTxFifo.getNext();
         ConnectionMap::Connection* conn = mConnectionMap.get(data->destination);
         
         if (conn == 0)
         {
            //notify ppl that transaction is dead
            delete data;
            return;
         }
         if (conn->mOutstandingSends.empty())
         {
            mSendRoundRobin.push_back(conn);
            conn->mSendPos = 0;
         }
         conn->mOutstandingSends.push_back(data);
      }
      
      if (sendFromRoundRobin(fdset))
      {
         return;
      }
   }
}         

bool 
TcpTransport::sendFromRoundRobin(FdSet& fdset)
{
   if (mSendRoundRobin.empty())
   {
      return true;
   }

   ConnectionList::iterator rrPos = mSendPos;
   do
   {
      if (mSendPos == mSendRoundRobin.end())
      {
         mSendPos = mSendRoundRobin.begin();
      }
      if (fdset.readyToWrite((*mSendPos)->getSocket()))
      {
         if (processWrite(*mSendPos))
         {
            if ((*mSendPos)->mOutstandingSends.empty())
            {
               mConnectionMap.close((*mSendPos)->mWho);
               mSendPos = mSendRoundRobin.erase(mSendPos);
            }
            else
            {
               mSendPos++;
            }
            return true;
         }
         else
         {
            mConnectionMap.close((*mSendPos)->mWho);
            mSendPos = mSendRoundRobin.erase(mSendPos);
            return true;
         }
      }
      else
      {
         mSendPos++;
      }
   } while(mSendPos != rrPos);
   return false;
}

bool
TcpTransport::processWrite(ConnectionMap::Connection* c)
{
   assert(!c->mOutstandingSends.empty());
   SendData* data = c->mOutstandingSends.front();
   
   int bytesToWrite = data->data.size() - c->mSendPos;
   int bytesWritten = write(c->getSocket(), data->data.data() + c->mSendPos, bytesToWrite);

   if (bytesWritten < 0)
   {
      //send error(senddata will have transactionid), or if at start of new
      //message, try to open again !dcm!
      mConnectionMap.close(data->destination);
      //touch if things work on retry
      return false; //or true if you are not dead(retry connection if applicable)
   }
   else if (bytesWritten < bytesToWrite)
   {
      c->mSendPos += bytesWritten;
   }
   else
   {
      delete data;
      c->mOutstandingSends.pop_front();
      c->mSendPos = 0;
   }
   mConnectionMap.touch(c);
   return true;
}

void 
TcpTransport::process(FdSet& fdSet)
{
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
