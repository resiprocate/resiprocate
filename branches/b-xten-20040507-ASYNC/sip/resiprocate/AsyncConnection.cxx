#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/AsyncConnection.hxx"
#include "resiprocate/ConnectionManager.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Security.hxx"
#include "resiprocate/TcpBaseTransport.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT


AsyncConnection::AsyncConnection(const Tuple& who, AsyncID streamID, AsyncStreamTransport& tport, 
                                 bool fromAccept)
   : ConnectionBase(who),
     mStreamID(streamID), 
     mAsyncStreamTransport(tport),
     mState(Connected),
     mCurrentTid(Data::Empty)
{
   InfoLog(<< "AsyncConnection::AsyncConnection(accept): " << streamID);   
   mAsyncStreamTransport.mConnectionsByDest[who] = this;
   mAsyncStreamTransport.mConnectionsByStream[streamID] = this;
}

AsyncConnection::AsyncConnection(const Tuple& who, AsyncID streamID, AsyncStreamTransport& tport)
: ConnectionBase(who),
  mStreamID(streamID), 
  mAsyncStreamTransport(tport),
  mState(New),
  mCurrentTid(Data::Empty)
{
   InfoLog(<< "AsyncConnection::AsyncConnection: " << streamID);   
   mAsyncStreamTransport.mConnectionsByDest[who] = this;
   mAsyncStreamTransport.mConnectionsByStream[streamID] = this;
   mAsyncStreamTransport.mExternalTransport->connect(who.toGenericIPAddress(), streamID);   
}

AsyncConnection::~AsyncConnection()
{
   mAsyncStreamTransport.mExternalTransport->close(mStreamID);
   mAsyncStreamTransport.mConnectionsByDest.erase(mWho);
   mAsyncStreamTransport.mConnectionsByStream.erase(mStreamID);
}

// void
// AsyncConnection::requestWrite(SendData* sendData)
// {
// }

void 
AsyncConnection::handleConnectSuccess()
{
   mState = Connected;
   while(!mQueue.empty())
   {
      SendData* sd = mQueue.front();
      mQueue.pop_front();
      unsigned char* bytes = new unsigned char[sd->data.size()];
      
      mCurrentTid = sd->transactionId;      
      memcpy(bytes, sd->data.data(), sd->data.size());
      
      mAsyncStreamTransport.mExternalTransport->write(getAsyncStreamID(), bytes, sd->data.size());
      delete sd;
   }
}

void 
AsyncConnection::write(const Tuple& dest, const Data& pdata, const Data& tid)
{
   mCurrentTid = tid;    
   
   if (mState == Connected)
   {
      unsigned char* bytes = new unsigned char[pdata.size()];
      memcpy(bytes, pdata.data(), pdata.size());
      
      mAsyncStreamTransport.mExternalTransport->write(getAsyncStreamID(), bytes, pdata.size());
   }
   else
   {
      mQueue.push_back(new SendData(dest, pdata, tid));
   }
}



//!dcm! -- why is the fifo passed through each time in these interfaces instead of being set at construction time?
void 
AsyncConnection::handleRead(char* bytes, int count, Fifo< TransactionMessage >& fifo)
{
   //uses the passed in buffer if the state is NewMessage(avoids allocation) otherwise
   //writes bytes into the write buffer.  Definitely room for optimization here.
   if(getCurrentState() == NewMessage)
   {
      setBuffer(bytes, count);
      preparseNewBytes(count, fifo);
   }
   else
   {
      int currentInputPos = 0;  
      while(currentInputPos < count)
      {
         std::pair<char*, size_t> res = getWriteBuffer();
         int bytesToCopy = resipMin((int) res.second, count);
         preparseNewBytes(bytesToCopy, fifo);
         memcpy(res.first, bytes + currentInputPos, bytesToCopy);
         currentInputPos += count;
      }
      delete [] bytes;
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

