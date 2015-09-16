#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "rutil/ResipAssert.h"
#include "rutil/Socket.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/Connection.hxx"
#include "resip/stack/ConnectionManager.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/TcpBaseTransport.hxx"
#include "rutil/WinLeakCheck.hxx"

#ifdef USE_SSL
#include "resip/stack/ssl/Security.hxx"
#endif

#ifdef WIN32
#include <Mswsock.h>
#endif

#ifdef USE_SIGCOMP
#include <osc/Stack.h>
#include <osc/SigcompMessage.h>
#endif

using namespace resip;

volatile bool Connection::mEnablePostConnectSocketFuncCall = false;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

Connection::Connection(Transport* transport,const Tuple& who, Socket socket,
                       Compression &compression)
   : ConnectionBase(transport,who,compression),
     mFirstWriteAfterConnectedPending(false),
     mInWritable(false),
     mFlowTimerEnabled(false),
     mPollItemHandle(0)
{
   mWho.mFlowKey=(FlowKey)socket;
   InfoLog (<< "Connection::Connection: new connection created to who: " << mWho);

   if(transport && isWebSocket(transport->transport()))
   {
      mSendingTransmissionFormat = WebSocketHandshake;
      mReceivingTransmissionFormat = WebSocketHandshake;
   }

   if(mWho.mFlowKey && ConnectionBase::transport())
   {
      getConnectionManager().addConnection(this);
   }
}

Connection::~Connection()
{
   if(mWho.mFlowKey && ConnectionBase::transport())
   {
      getConnectionManager().removeConnection(this);
      // remove first then close, since conn manager may need socket
      closeSocket(mWho.mFlowKey);
   }
}

void
Connection::requestWrite(SendData* sendData)
{
   mOutstandingSends.push_back(sendData);
   if (isWritable())
   {
      ensureWritable();
   }
}

void 
Connection::removeFrontOutstandingSend()
{
   delete mOutstandingSends.front();
   mOutstandingSends.pop_front();

   if (mOutstandingSends.empty())
   {
      resip_assert(mInWritable);
      getConnectionManager().removeFromWritable(this);
      mInWritable = false;
   }
}

int
Connection::performWrite()
{
   if(transportWrite())
   {
      // If we get here it means:
      // a. on a previous invocation, SSL_do_handshake wanted to write
      //         (SSL_ERROR_WANT_WRITE)
      // b. now the handshake is complete or it wants to read
      if(mInWritable)
      {
         getConnectionManager().removeFromWritable(this);
         mInWritable = false;
      }
      else
      {
         WarningLog(<<"performWrite invoked while not in write set");
      }
      return 0; // Q. What does this transportWrite() mean?
                // A. It makes the TLS handshake move along after it
                //    was waiting in the write set.
   }

   // If the TLS handshake returned SSL_ERROR_WANT_WRITE again
   // then we could get here without really having something to write
   // so just return, remaining in the write set.
   if(mOutstandingSends.empty())
   {
      // FIXME: this needs to be more elaborate with respect
      // to TLS handshaking but it doesn't appear we can do that
      // without ABI breakage.
      return 0;
   }

   switch(mOutstandingSends.front()->command)
   {
   case SendData::CloseConnection:
      // .bwc. Close this connection.
      return -1;
      break;
   case SendData::EnableFlowTimer:
      enableFlowTimer();
      removeFrontOutstandingSend();
      return 0;
      break;
   default:
      // do nothing
      break;
   }

   const Data& sigcompId = mOutstandingSends.front()->sigcompId;

   if(mSendingTransmissionFormat == Unknown)
   {
      if (sigcompId.size() > 0 && mCompression.isEnabled())
      {
         mSendingTransmissionFormat = Compressed;
      }
      else
      {
         mSendingTransmissionFormat = Uncompressed;
      }
   }
   else if(mSendingTransmissionFormat == WebSocketHandshake)
   {
      mSendingTransmissionFormat = WebSocketData;
   }
   else if(mSendingTransmissionFormat == WebSocketData)
   {
      SendData *dataWs, *oldSd;
      const Data& dataRaw = mOutstandingSends.front()->data;
      UInt64 dataSize = 1 + 1 + dataRaw.size();
      UInt64 lSize = (UInt64)dataRaw.size();
      UInt8* uBuffer;

      if(lSize > 0x7D && lSize <= 0xFFFF)
      {
         dataSize += 2;
      }
      else if(lSize > 0xFFFF)
      {
         dataSize += 8;
      }

      oldSd = mOutstandingSends.front();
      dataWs = new SendData(oldSd->destination,
            Data(Data::Take, new char[(int)dataSize], (Data::size_type)dataSize),
            oldSd->transactionId,
            oldSd->sigcompId,
            false);
      resip_assert(dataWs && dataWs->data.data());
      uBuffer = (UInt8*)dataWs->data.data();

      uBuffer[0] = 0x82;
      if(lSize <= 0x7D)
      {
         uBuffer[1] = (UInt8)lSize;
         uBuffer = &uBuffer[2];
      }
      else if(lSize <= 0xFFFF)
      {
         uBuffer[1] = 0x7E;
         uBuffer[2] = (UInt8)((lSize >> 8) & 0xFF);
         uBuffer[3] = (UInt8)(lSize & 0xFF);
         uBuffer = &uBuffer[4];
      }
      else
      {
         uBuffer[1] = 0x7F;
         uBuffer[2] = (UInt8)((lSize >> 56) & 0xFF);
         uBuffer[3] = (UInt8)((lSize >> 48) & 0xFF);
         uBuffer[4] = (UInt8)((lSize >> 40) & 0xFF);
         uBuffer[5] = (UInt8)((lSize >> 32) & 0xFF);
         uBuffer[6] = (UInt8)((lSize >> 24) & 0xFF);
         uBuffer[7] = (UInt8)((lSize >> 16) & 0xFF);
         uBuffer[8] = (UInt8)((lSize >> 8) & 0xFF);
         uBuffer[9] = (UInt8)(lSize & 0xFF);
         uBuffer = &uBuffer[10];
      }

      memcpy(uBuffer, dataRaw.data(), dataRaw.size());
      mOutstandingSends.front() = dataWs;
      dataWs = 0;
      delete oldSd;
   }

#ifdef USE_SIGCOMP
   // Perform compression here, if appropriate
   if (mSendingTransmissionFormat == Compressed
       && !(mOutstandingSends.front()->isAlreadyCompressed))
   {
      const Data& uncompressed = mOutstandingSends.front()->data;
      osc::SigcompMessage *sm = 
        mSigcompStack->compressMessage(uncompressed.data(), uncompressed.size(),
                                       sigcompId.data(), sigcompId.size(),
                                       true);
      DebugLog (<< "Compressed message from "
                << uncompressed.size() << " bytes to " 
                << sm->getStreamLength() << " bytes");

      SendData *oldSd = mOutstandingSends.front();
      SendData *newSd = new SendData(oldSd->destination,
                                     Data(sm->getStreamMessage(),
                                          sm->getStreamLength()),
                                     oldSd->transactionId,
                                     oldSd->sigcompId,
                                     true);
      mOutstandingSends.front() = newSd;
      delete oldSd;
      delete sm;
   }
#endif

   // Note:  The first time the socket is available for write, is when the TCP connect call is completed
   if (mFirstWriteAfterConnectedPending)
   {
      mFirstWriteAfterConnectedPending = false;  // reset

      // Notify all outstanding sends that we are now connected - stops the TCP Connection timer for all transactions
      for (std::list<SendData*>::iterator it = mOutstandingSends.begin(); it != mOutstandingSends.end(); it++)
      {
         mTransport->setTcpConnectState((*it)->transactionId, TcpConnectState::Connected);
      }
      if (mEnablePostConnectSocketFuncCall)
      {
          mTransport->callSocketFunc(getSocket());
      }
   }

   const Data& data = mOutstandingSends.front()->data;
   int nBytes = write(data.data() + mSendPos,int(data.size() - mSendPos));

   //DebugLog (<< "Tried to send " << data.size() - mSendPos << " bytes, sent " << nBytes << " bytes");

   if (nBytes < 0)
   {
      //fail(data.transactionId);
      InfoLog(<< "Write failed on socket: " << this->getSocket() << ", closing connection");
      return -1;
   }
   else if (nBytes == 0)
   {
      // Nothing was written - likely socket buffers are backed up and EWOULDBLOCK was returned
      // no need to do calculations in else statement
      return 0;
   }
   else
   {
      // Safe because of the conditional above ( < 0 ).
      Data::size_type bytesWritten = static_cast<Data::size_type>(nBytes);
      mSendPos += bytesWritten;
      if (mSendPos == data.size())
      {
         mSendPos = 0;
         removeFrontOutstandingSend();
      }
      return bytesWritten;
   }
}


bool 
Connection::performWrites(unsigned int max)
{
   int res;
   // if max==0, we will overflow into UINT_MAX. This is intentional.
   while((res=performWrite())>0 && !mOutstandingSends.empty() && --max!=0)
   {;}

   if(res<0)
   {
      delete this;
      return false;
   }
   return true;
}

void 
Connection::ensureWritable()
{
   if(!mInWritable)
   {
      //assert(!mOutstandingSends.empty()); // empty during TLS handshake
      // therefore must be careful to check mOutstandingSends later
      getConnectionManager().addToWritable(this);
      mInWritable = true;
   }
}

ConnectionManager&
Connection::getConnectionManager() const
{
   TcpBaseTransport* transport = static_cast<TcpBaseTransport*>(ConnectionBase::transport());
   
   return transport->getConnectionManager();
}
            
EncodeStream& 
resip::operator<<(EncodeStream& strm, const resip::Connection& c)
{
   strm << "CONN: " << &c << " " << int(c.getSocket()) << " " << c.mWho;
   return strm;
}

int
Connection::read()
{
   std::pair<char*, size_t> writePair = getWriteBuffer();
   size_t bytesToRead = resipMin(writePair.second, 
                                 static_cast<size_t>(Connection::ChunkSize));
         
   resip_assert(bytesToRead > 0);

   int bytesRead = read(writePair.first, (int)bytesToRead);
   if (bytesRead <= 0)
   {
      return bytesRead;
   }  
   // mBuffer might have been reallocated inside read()
   writePair = getCurrentWriteBuffer();

   getConnectionManager().touch(this);

#ifdef USE_SIGCOMP
   // If this is the first data we read, determine whether the
   // connection is compressed.
   if(mReceivingTransmissionFormat == Unknown)
   {
     if (((writePair.first[0] & 0xf8) == 0xf8) && mCompression.isEnabled())
     {
       mReceivingTransmissionFormat = Compressed;
     }
     else
     {
       mReceivingTransmissionFormat = Uncompressed;
     }
   }

   // SigComp compressed messages are handed very differently
   // than non-compressed messages: they are guaranteed to
   // be framed within SigComp, and each frame contains
   // *exactly* one SIP message. Processing looks a lot like
   // it does for Datagram-oriented transports.

   if (mReceivingTransmissionFormat == Compressed)
   {
     decompressNewBytes(bytesRead);
   }
   else
#endif
   {
      if (mReceivingTransmissionFormat == WebSocketHandshake)
      {
         bool dropConnection = false;
         if(wsProcessHandshake(bytesRead, dropConnection))
         {
            ensureWritable();
            if(performWrites())
            {
               mReceivingTransmissionFormat = WebSocketData;
            }
         }
         else if(dropConnection)
         {
            bytesRead=-1;
         }
      }
      else
      {
         if (mReceivingTransmissionFormat == WebSocketData)
         {
            if(!wsProcessData(bytesRead))
            {
               bytesRead=-1;
            }
         }
         else if(!preparseNewBytes(bytesRead))
         {
            // Iffy; only way we have right now to indicate that this connection has
            // gone away.
            bytesRead=-1;
         }
      }
   }
   return bytesRead;
}

bool 
Connection::performReads(unsigned int max)
{
   int bytesRead;

   // if max==0, we will overflow into UINT_MAX. This is intentional.
   while((bytesRead = read())>0 && --max!=0)
   {
      DebugLog(<< "Connection::performReads() " << " read=" << bytesRead);
   }

   if ( bytesRead < 0 ) 
   {
      DebugLog(<< "Closing connection bytesRead=" << bytesRead);
      delete this;
      return false;
   }
   return true;
}

void
Connection::enableFlowTimer()
{
   if(!mFlowTimerEnabled)
   {
      mFlowTimerEnabled = true;

      // ensure connection is in a FlowTimer LRU list on the connection manager
      getConnectionManager().moveToFlowTimerLru(this);
   }
}

void
Connection::onDoubleCRLF()
{
   // !bwc! TODO might need to make this more efficient.
   // !bwc! Need to make this sigcomp-friendly
   if(InteropHelper::getOutboundVersion()>=8)
   {
      DebugLog(<<"Sending response CRLF (aka pong).");
      requestWrite(new SendData(mWho,Symbols::CRLF,Data::Empty,Data::Empty));
   }
}

void
Connection::onSingleCRLF()
{
   DebugLog(<<"Received response CRLF (aka pong).");
   mTransport->keepAlivePong(mWho);
}

bool 
Connection::hasDataToRead()
{
   return true;
}

bool 
Connection::isGood()
{
   return true;
}

bool 
Connection::checkConnectionTimedout()
{
   int errNum = 0;
   int errNumSize = sizeof(errNum);
   if(getsockopt(mWho.mFlowKey, SOL_SOCKET, SO_ERROR, (char *)&errNum, (socklen_t *)&errNumSize) == 0)
   {
      if (errNum == ETIMEDOUT || errNum == EHOSTUNREACH || errNum == ECONNREFUSED)
      {
         InfoLog(<< "Exception on socket " << mWho.mFlowKey << " code: " << errNum << "; closing connection");
         setFailureReason(TransportFailure::ConnectionException, errNum);
         delete this;
         return true;
      }
      else if (errNum != 0)
      {
         WarningLog(<< "checkConnectionTimedout " << mWho.mFlowKey << " code: " << errNum << "; ignoring - should we error out?");
      }
   }
   return false;
}

bool 
Connection::isWritable()
{
   return true;
}

/**
    Virtual function of FdPollItemIf, called to process io events
**/
void
Connection::processPollEvent(FdPollEventMask mask) {
   /* The original code in ConnectionManager.cxx didn't check
    * for error events unless no writable event. (e.g., writable
    * masked error. Why?)
    */
   if ( mask & FPEM_Error ) 
   {
      Socket fd = getSocket();
      int errNum = getSocketError(fd);
      InfoLog(<< "Exception on socket " << fd << " code: " << errNum << "; closing connection");
      setFailureReason(TransportFailure::ConnectionException, errNum);
      delete this;
      return;
   }
   if ( mask & FPEM_Write ) 
   {
      if(!performWrites())
      {
         // Just deleted self
         return;
      }
   }
   if ( mask & FPEM_Read ) 
   {
      performReads();
   }
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000
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
