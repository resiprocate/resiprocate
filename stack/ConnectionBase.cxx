#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "rutil/Logger.hxx"
#include "resip/stack/ConnectionBase.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Security.hxx"
#include "resip/stack/TlsConnection.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

char 
ConnectionBase::connectionStates[ConnectionBase::MAX][32] = { "NewMessage", "ReadingHeaders", "PartialBody" };


ConnectionBase::ConnectionBase()
   : mSendPos(0),
     mWho(),
     mFailureReason(TransportFailure::None),
     mMessage(0),
     mBuffer(0),
     mBufferPos(0),
     mBufferSize(0),
     mLastUsed(0),
     mConnState(NewMessage)
{
   DebugLog (<< "ConnectionBase::ConnectionBase, no params: " << this);
}

ConnectionBase::ConnectionBase(const Tuple& who)
   : mSendPos(0),
     mWho(who),
     mFailureReason(TransportFailure::None),
     mMessage(0),
     mBuffer(0),
     mBufferPos(0),
     mBufferSize(0),
     mLastUsed(Timer::getTimeMs()),
     mConnState(NewMessage)
{
   DebugLog (<< "ConnectionBase::ConnectionBase, who: " << mWho << " " << this);
}

ConnectionBase::~ConnectionBase()
{
   if (mWho.transport)
   {
      mWho.transport->connectionTerminated(getId());
   }

   while (!mOutstandingSends.empty())
   {
      SendData* sendData = mOutstandingSends.front();
      mWho.transport->fail(sendData->transactionId, mFailureReason);
      
      delete sendData;
      mOutstandingSends.pop_front();
   }
   DebugLog (<< "ConnectionBase::~ConnectionBase " << this);
   delete [] mBuffer;
   delete mMessage;
}

ConnectionId
ConnectionBase::getId() const
{
   return mWho.connectionId;
}

void
ConnectionBase::preparseNewBytes(int bytesRead, Fifo<TransactionMessage>& fifo)
{
   assert(mWho.transport);

   DebugLog(<< "In State: " << connectionStates[mConnState]);
   //getConnectionManager().touch(this); -- !dcm!
   
  start:   // If there is an overhang come back here, effectively recursing
   
   switch(mConnState)
   {
      case NewMessage:
      {
         if (strncmp(mBuffer + mBufferPos, Symbols::CRLFCRLF, 4) == 0)
         {
            StackLog(<<"Throwing away incoming firewall keep-alive");
            mBufferPos += 4;
            bytesRead -= 4;
            if (bytesRead)
            {
               goto start;
            }
            else
            {
               delete [] mBuffer;
               mBuffer = 0;
               return;
            }
         }
         assert(mWho.transport);
         mMessage = new SipMessage(mWho.transport);
         
         DebugLog(<< "ConnectionBase::process setting source " << mWho);
         mMessage->setSource(mWho);
         mMessage->setTlsDomain(mWho.transport->tlsDomain());

         // Set TlsPeerName if message is from TlsConnection
         TlsConnection *tlsConnection = dynamic_cast<TlsConnection *>(this);
         if(tlsConnection)
         {
            mMessage->setTlsPeerNames(tlsConnection->getPeerNames());
         }
         mMsgHeaderScanner.prepareForMessage(mMessage);
         // Fall through to the next case.
      }
      case ReadingHeaders:
      {
         unsigned int chunkLength = mBufferPos + bytesRead;
         char *unprocessedCharPtr;
         MsgHeaderScanner::ScanChunkResult scanChunkResult =
            mMsgHeaderScanner.scanChunk(mBuffer,
                                        chunkLength,
                                        &unprocessedCharPtr);
         if (scanChunkResult == MsgHeaderScanner::scrError)
         {
            //.jacob. Not a terribly informative warning.
            WarningLog(<< "Discarding preparse!");
            delete [] mBuffer;
            mBuffer = 0;
            delete mMessage;
            mMessage = 0;
            //.jacob. Shouldn't the state also be set here?
            delete this;
            return;
         }
         mMessage->addBuffer(mBuffer);
         unsigned int numUnprocessedChars =
            (mBuffer + chunkLength) - unprocessedCharPtr;
         if (scanChunkResult == MsgHeaderScanner::scrNextChunk)
         {
            // Message header is incomplete...
            if (numUnprocessedChars == 0)
            {
               // ...but the chunk is completely processed.
               //.jacob. I've discarded the "assigned" concept.
               //DebugLog(<< "Data assigned, not fragmented, not complete");
               mBuffer = MsgHeaderScanner::allocateBuffer(ChunkSize);
               mBufferPos = 0;
               mBufferSize = ChunkSize;
            }
            else
            {
               // ...but some of the chunk must be shifted into the next one.
               size_t size = numUnprocessedChars*3/2;
               if (size < ConnectionBase::ChunkSize)
               {
                  size = ConnectionBase::ChunkSize;
               }
               char* newBuffer = MsgHeaderScanner::allocateBuffer(size);
               memcpy(newBuffer, unprocessedCharPtr, numUnprocessedChars);
               mBuffer = newBuffer;
               mBufferPos = numUnprocessedChars;
               mBufferSize = size;
            }
            mConnState = ReadingHeaders;
         }
         else
         {         
            // The message header is complete.
            size_t contentLength = mMessage->header(h_ContentLength).value();
            
            if (numUnprocessedChars < contentLength)
            {
               // The message body is incomplete.
               DebugLog(<< "partial body received");
               char* newBuffer = MsgHeaderScanner::allocateBuffer(contentLength);               
               memcpy(newBuffer, unprocessedCharPtr, numUnprocessedChars);
               mBufferPos = numUnprocessedChars;
               mBufferSize = contentLength;
               mBuffer = newBuffer;
            
               mConnState = PartialBody;
            }
            else
            {
               // The message body is complete.
               mMessage->setBody(unprocessedCharPtr, contentLength);
               if (!transport()->basicCheck(*mMessage))
               {
                  delete mMessage;
                  mMessage = 0;
               }
               else
               {
                  Transport::stampReceived(mMessage);
                  DebugLog(<< "##Connection: " << *this << " received: " << *mMessage);
                  fifo.add(mMessage);
                  mMessage = 0;                  
               }

               int overHang = numUnprocessedChars - contentLength;

               mConnState = NewMessage;
               mBuffer = 0;               
               if (overHang > 0) 
               {
                  // The next message has been partially read.
                  size_t size = overHang*3/2;
                  if (size < ConnectionBase::ChunkSize)
                  {
                     size = ConnectionBase::ChunkSize;
                  }
                  char* newBuffer = MsgHeaderScanner::allocateBuffer(size);
                  memcpy(newBuffer,
                         unprocessedCharPtr + contentLength,
                         overHang);
                  mBuffer = newBuffer;
                  mBufferPos = 0;
                  mBufferSize = size;
                  
                  DebugLog (<< "Extra bytes after message: " << overHang);
                  DebugLog (<< Data(mBuffer, overHang));
                  
                  bytesRead = overHang;
                  goto start;
               }
            }
         }
         break;
      }
      case PartialBody:
      {
         size_t contentLength = mMessage->header(h_ContentLength).value();
         mBufferPos += bytesRead;
         if (mBufferPos == contentLength)
         {
            mMessage->addBuffer(mBuffer);
            mMessage->setBody(mBuffer, contentLength);
            if (!transport()->basicCheck(*mMessage))
            {
               delete mMessage;
               mMessage = 0;
            }
            else
            {
               DebugLog(<< "##ConnectionBase: " << *this << " received: " << *mMessage);

               Transport::stampReceived(mMessage);
               fifo.add(mMessage);
               mMessage = 0;
            }
            mConnState = NewMessage;
            mBuffer = 0;            
         }
         break;
      }
      default:
         assert(0);
   }
}
            
std::pair<char*, size_t> 
ConnectionBase::getWriteBuffer()
{
   if (mConnState == NewMessage)
   {
      if (mBuffer)
      {
	 delete [] mBuffer;
      }

      DebugLog (<< "Creating buffer for " << *this);

      mBuffer = MsgHeaderScanner::allocateBuffer(ConnectionBase::ChunkSize);
      mBufferSize = ConnectionBase::ChunkSize;
      mBufferPos = 0;
   }
   return std::make_pair(mBuffer + mBufferPos, mBufferSize - mBufferPos);
}

char*
ConnectionBase::getWriteBufferForExtraBytes(int extraBytes)
{
   char* buffer = MsgHeaderScanner::allocateBuffer(mBufferSize + extraBytes);
   memcpy(buffer, mBuffer, mBufferSize);
   delete [] mBuffer;
   mBuffer = buffer;
   buffer += mBufferSize;
   mBufferSize += extraBytes;
   return buffer;
}
            
void 
ConnectionBase::setBuffer(char* bytes, int count)
{
   mBuffer = bytes;
   mBufferPos = 0;
   mBufferSize = count;
}

Transport* 
ConnectionBase::transport()
{
   assert(this);
   return mWho.transport;
}

std::ostream& 
resip::operator<<(std::ostream& strm, 
                  const resip::ConnectionBase& c)

{
   strm << "CONN_BASE: " << &c << " " << c.mWho;
   return strm;
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

