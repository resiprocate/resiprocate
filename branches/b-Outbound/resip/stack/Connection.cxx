#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "rutil/Socket.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/Connection.hxx"
#include "resip/stack/ConnectionManager.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Security.hxx"
#include "resip/stack/TcpBaseTransport.hxx"
#include "rutil/WinLeakCheck.hxx"

#ifdef USE_SIGCOMP
#include <osc/Stack.h>
#include <osc/SigcompMessage.h>
#endif

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

Connection::Connection(Transport* transport,const Tuple& who, Socket socket,
                       Compression &compression)
   : ConnectionBase(transport,who,compression),
     mInWritable(false)
{
   mWho.mFlowKey=socket;
   if(mWho.mFlowKey && ConnectionBase::transport())
   {
      getConnectionManager().addConnection(this);
   }
}

Connection::~Connection()
{
   if(mWho.mFlowKey && ConnectionBase::transport())
   {
      closeSocket(mWho.mFlowKey);
      getConnectionManager().removeConnection(this);
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

   const Data& data = mOutstandingSends.front()->data;

   int nBytes = write(data.data() + mSendPos,data.size() - mSendPos);

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
   if(!mInWritable && !mOutstandingSends.empty())
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
            
std::ostream& 
resip::operator<<(std::ostream& strm, const resip::Connection& c)
{
   strm << "CONN: " << &c << " " << int(c.getSocket()) << " " << c.mWho;
   return strm;
}

int
Connection::read(Fifo<TransactionMessage>& fifo)
{
   std::pair<char*, size_t> writePair = getWriteBuffer();
   size_t bytesToRead = resipMin(writePair.second, 
                                 static_cast<size_t>(Connection::ChunkSize));
         
   assert(bytesToRead > 0);

   int bytesRead = read(writePair.first, bytesToRead);
   if (bytesRead <= 0)
   {
      return bytesRead;
   }  
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
     decompressNewBytes(bytesRead, fifo);
   }
   else
#endif
   {
     preparseNewBytes(bytesRead, fifo); //.dcm. may delete this   
   }
   return bytesRead;
}

void
Connection::onDoubleCRLF()
{
   // !bwc! TODO might need to make this more efficient.
   // ?bwc? We don't need a sigcomp id, do we?
   if(InteropHelper::getOutboundVersion()<7)
   {
      DebugLog(<<"Sending response CRLF (aka pong).");
      requestWrite(new SendData(mWho,Symbols::CRLF,Data::Empty,Data::Empty));
   }
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
 */
