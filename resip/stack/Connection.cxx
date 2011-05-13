#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

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
     mRequestPostConnectSocketFuncCall(false),
     mInWritable(false),
     mFlowTimerEnabled(false),
     mPollItemHandle(0)
{
   mWho.mFlowKey=(FlowKey)socket;
   InfoLog (<< "Connection::Connection: new connection created to who: " << mWho);

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
Connection::performWrite()
{
   if(transportWrite())
   {
      assert(mInWritable);
      getConnectionManager().removeFromWritable(this);
      mInWritable = false;
      return;
   }

   assert(!mOutstandingSends.empty());

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

   if(mEnablePostConnectSocketFuncCall && mRequestPostConnectSocketFuncCall)
   {
       // Note:  The first time the socket is available for write, is when the TCP connect call is completed
      mRequestPostConnectSocketFuncCall = false;
      mTransport->callSocketFunc(getSocket());
   }

   const Data& data = mOutstandingSends.front()->data;

   int nBytes = write(data.data() + mSendPos,int(data.size() - mSendPos));

   //DebugLog (<< "Tried to send " << data.size() - mSendPos << " bytes, sent " << nBytes << " bytes");

   if (nBytes < 0)
   {
      //fail(data.transactionId);
      InfoLog(<< "Write failed on socket: " << this->getSocket() << ", closing connection");
      delete this;
   }
   else
   {
      // Safe because of the conditional above ( < 0 ).
      Data::size_type bytesWritten = static_cast<Data::size_type>(nBytes);
      mSendPos += bytesWritten;
      if (mSendPos == data.size())
      {
         mSendPos = 0;
         delete mOutstandingSends.front();
         mOutstandingSends.pop_front();

         if (mOutstandingSends.empty())
         {
            assert(mInWritable);
            getConnectionManager().removeFromWritable(this);
            mInWritable = false;
         }
      }
   }
}
    
void 
Connection::ensureWritable()
{
   if(!mInWritable)
   {
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
         
   assert(bytesToRead > 0);

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
     preparseNewBytes(bytesRead); //.dcm. may delete this
   }
   return bytesRead;
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
      performWrite();
   }
   if ( mask & FPEM_Read ) 
   {
      int bytesRead = read();
      if ( bytesRead < 0 ) 
      {
         delete this;
         return;
      }
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
