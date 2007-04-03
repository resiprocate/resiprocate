#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "rutil/Logger.hxx"
#include "resip/stack/ConnectionBase.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Security.hxx"
#include "resip/stack/TlsConnection.hxx"
#include "rutil/WinLeakCheck.hxx"

#ifdef USE_SIGCOMP
#include <osc/Stack.h>
#include <osc/TcpStream.h>
#include <osc/SigcompMessage.h>
#include <osc/StateChanges.h>
#endif

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

char 
ConnectionBase::connectionStates[ConnectionBase::MAX][32] = { "NewMessage", "ReadingHeaders", "PartialBody" };


ConnectionBase::ConnectionBase(Transport* transport, const Tuple& who, Compression &compression)
   : mSendPos(0),
     mTransport(transport),
     mWho(who),
     mFailureReason(TransportFailure::None),
     mCompression(compression),
#ifdef USE_SIGCOMP
     mSigcompStack(0),
     mSigcompFramer(0),
#endif
     mSendingTransmissionFormat(Unknown),
     mReceivingTransmissionFormat(Unknown),
     mMessage(0),
     mBuffer(0),
     mBufferPos(0),
     mBufferSize(0),
     mLastUsed(Timer::getTimeMs()),
     mConnState(NewMessage)
{
   DebugLog (<< "ConnectionBase::ConnectionBase, who: " << mWho << " " << this);
#ifdef USE_SIGCOMP
   if (mCompression.isEnabled())
   {
      DebugLog (<< "Compression enabled for connection: " << this);
      mSigcompStack = new osc::Stack(mCompression.getStateHandler());
      mCompression.addCompressorsToStack(mSigcompStack);
   }
   else
   {
      DebugLog (<< "Compression disabled for connection: " << this);
   }
#else
   DebugLog (<< "No compression library available: " << this);
#endif

   mWho.transport=mTransport;
}

ConnectionBase::~ConnectionBase()
{
   if(mTransport)
   {
      mTransport->flowTerminated(mWho);
   }

   while (!mOutstandingSends.empty())
   {
      SendData* sendData = mOutstandingSends.front();
      mTransport->fail(sendData->transactionId, mFailureReason);
      
      delete sendData;
      mOutstandingSends.pop_front();
   }
   DebugLog (<< "ConnectionBase::~ConnectionBase " << this);
   delete [] mBuffer;
   delete mMessage;
#ifdef USE_SIGCOMP
   delete mSigcompStack;
#endif
}

FlowKey
ConnectionBase::getFlowKey() const
{
   return mWho.mFlowKey;
}

void
ConnectionBase::preparseNewBytes(int bytesRead, Fifo<TransactionMessage>& fifo)
{

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
            onDoubleCRLF();
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
         assert(mTransport);
         mMessage = new SipMessage(mTransport);
         
         DebugLog(<< "ConnectionBase::process setting source " << mWho);
         mMessage->setSource(mWho);
         mMessage->setTlsDomain(mTransport->tlsDomain());

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
            size_t contentLength = 0;
            
            try
            {
               // The message header is complete.
               contentLength=mMessage->header(h_ContentLength).value();
            }
            catch(resip::ParseBuffer::Exception& e)
            {
               WarningLog(<<"Malformed Content-Length in connection-based transport"
                           ". Not much we can do to fix this.  " << e);
               // !bwc! Bad Content-Length. We are hosed.
               delete mMessage;
               mMessage = 0;
               mBuffer = 0;
               // !bwc! mMessage just took ownership of mBuffer, so we don't
               // delete it here. We do zero it though, for completeness.
               //.jacob. Shouldn't the state also be set here?
               delete this;
               return;
            }
            
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
         size_t contentLength = 0;

         try
         {
             contentLength = mMessage->header(h_ContentLength).value();
         }
         catch(resip::ParseBuffer::Exception& e)
         {
            WarningLog(<<"Malformed Content-Length in connection-based transport"
                        ". Not much we can do to fix this. " << e);
            // !bwc! Bad Content-Length. We are hosed.
            delete [] mBuffer;
            mBuffer = 0;
            delete mMessage;
            mMessage = 0;
            //.jacob. Shouldn't the state also be set here?
            delete this;
            return;
         }

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

#ifdef USE_SIGCOMP
void
ConnectionBase::decompressNewBytes(int bytesRead,
                                   Fifo<TransactionMessage>& fifo)
{
  mConnState = SigComp;

  if (!mSigcompFramer)
  {
    mSigcompFramer = new osc::TcpStream();
  }

  mSigcompFramer->addData(mBuffer, bytesRead);
  size_t bytesUncompressed;
  osc::StateChanges *sc = 0;
  char *uncompressed = new char[65536];
  while ((bytesUncompressed = mSigcompStack->uncompressMessage(
                *mSigcompFramer, uncompressed, 65536, sc)) > 0)
  {
    DebugLog (<< "Uncompressed Connection-oriented message");
    mMessage = new SipMessage(mWho.transport);

    mMessage->setSource(mWho);
    mMessage->setTlsDomain(mWho.transport->tlsDomain());

    char *sipBuffer = new char[bytesUncompressed];
    memmove(sipBuffer, uncompressed, bytesUncompressed);
    mMessage->addBuffer(sipBuffer);
    mMsgHeaderScanner.prepareForMessage(mMessage);
    char *unprocessedCharPtr;
    if (mMsgHeaderScanner.scanChunk(sipBuffer,
                                    bytesUncompressed,
                                    &unprocessedCharPtr) !=
        MsgHeaderScanner::scrEnd)
    {
       StackLog(<<"Scanner rejecting compressed message as unparsable");
       StackLog(<< Data(sipBuffer, bytesUncompressed));
       delete mMessage;
       mMessage=0;
    }
  
    unsigned int used = unprocessedCharPtr - sipBuffer;
    if (mMessage && (used < bytesUncompressed))
    {
      mMessage->setBody(sipBuffer+used, bytesUncompressed-used);
    }

    if (mMessage && !transport()->basicCheck(*mMessage))
    {
      delete mMessage;
      mMessage = 0;
    }

    if (mMessage)
    {
      Transport::stampReceived(mMessage);
      // If the message made it this far, we should let it store
      // SigComp state: extract the compartment ID.
      const Via &via = mMessage->header(h_Vias).front();
      if (mMessage->isRequest())
      {
        // For requests, the compartment ID is read out of the
        // top via header field; if not present, we use the
        // TCP connection for identification purposes.
        if (via.exists(p_sigcompId))
        {
          Data compId = via.param(p_sigcompId);
          mSigcompStack->provideCompartmentId(sc, compId.data(), compId.size());
        }
        else
        {
          mSigcompStack->provideCompartmentId(sc, this, sizeof(this));
        }
      }
      else
      {
        // For responses, the compartment ID is supposed to be
        // the same as the compartment ID of the request. We
        // *could* dig down into the transaction layer to try to
        // figure this out, but that's a royal pain, and a rather
        // severe layer violation. In practice, we're going to ferret
        // the ID out of the the Via header field, which is where we
        // squirreled it away when we sent this request in the first place.
        Data compId = via.param(p_branch).getSigcompCompartment();
        mSigcompStack->provideCompartmentId(sc, compId.data(), compId.size());
      }
      fifo.add(mMessage);
      mMessage = 0;
      sc = 0;
    }
    else
    {
      delete sc;
      sc = 0;
    }
  }
  delete uncompressed;

  // If there was a decompression failure, let the other side know.
  osc::SigcompMessage *nack = mSigcompStack->getNack();
  if (nack)
  {
    if (mSendingTransmissionFormat == Compressed)
    {
      mOutstandingSends.push_back(new SendData(
                   who(),
                   Data(nack->getStreamMessage(), nack->getStreamLength()),
                   Data::Empty,
                   Data::Empty,
                   true));
    }
    else
    {
      delete nack;
    }
  }
}
#endif
            
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
ConnectionBase::transport() const
{
   assert(this);
   return mTransport;
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

