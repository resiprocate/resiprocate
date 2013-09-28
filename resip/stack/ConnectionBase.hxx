#ifndef RESIP_ConnectionBase_hxx
#define RESIP_ConnectionBase_hxx

#include <deque>
#include <list>

#include "rutil/Timer.hxx"
// #include "rutil/Fifo.hxx"
#include "resip/stack/Transport.hxx"
#include "resip/stack/MsgHeaderScanner.hxx"
#include "resip/stack/SendData.hxx"
#include "resip/stack/WsFrameExtractor.hxx"
#include "resip/stack/Cookie.hxx"

namespace osc
{
   class Stack;
   class TcpStream;
}

namespace resip
{

class TransactionMessage;
class Compression;

/**
   @internal

   @brief Abstracts some of the connection functionality. 
   
   Managed connections (see ConnectionManager) derive from Connection. 
   Non-managed connections may be derived from ConnectionBase.

   @todo check id connectionId makes sense here
*/
class ConnectionBase
{
      friend EncodeStream& operator<<(EncodeStream& strm, const resip::ConnectionBase& c);
   public:
      ConnectionBase(Transport* transport,
                     const Tuple& who,
                     Compression &compression = Compression::Disabled);
      FlowKey getFlowKey() const;
      
      /// @todo should be reference
      virtual Transport* transport() const;

      Tuple& who() { return mWho; }
      const UInt64& whenLastUsed() { return mLastUsed; }
      void resetLastUsed() { mLastUsed = Timer::getTimeMs(); }

      enum { ChunkSize = 8192 }; // !jf! what is the optimal size here?
         // !dp! 8192 seems to be consistent with a multiple of a page size and
         //      also good for the larger SDP coming in with ICE attributes,
         //      multiple media streams, etc

   protected:
      enum ConnState
      {
         NewMessage = 0,
         ReadingHeaders,
         PartialBody,
         SigComp, // This indicates that incoming bytes are compressed.
         WebSocket,
         MAX
      };

      typedef enum
      {
         Unknown,
         Uncompressed,
         Compressed,
         WebSocketHandshake,
         WebSocketData,
      } TransmissionFormat;

      ConnState getCurrentState() const { return mConnState; }
      bool preparseNewBytes(int bytesRead);
      bool wsProcessHandshake(int bytesRead, bool &dropConnection);
      bool wsProcessData(int bytesRead);
      void wsParseCookies(CookieList& cookieList, const SipMessage* message);
      void decompressNewBytes(int bytesRead);
      std::pair<char*, size_t> getWriteBuffer();
      std::pair<char*, size_t> getCurrentWriteBuffer();
      char* getWriteBufferForExtraBytes(int extraBytes);
      
      // for avoiding copies in external transports--not used in core resip
      void setBuffer(char* bytes, int count);

      Data::size_type mSendPos;
      std::list<SendData*> mOutstandingSends; // !jacob! intrusive queue?

      void setFailureReason(TransportFailure::FailureReason failReason, int subCode);

      virtual ~ConnectionBase();
      // no value semantics
   private:
      ConnectionBase();
      ConnectionBase(const Connection&);
      ConnectionBase& operator=(const Connection&);
      bool scanMsgHeader(int bytesRead);
      std::auto_ptr<Data> makeWsHandshakeResponse();
      bool isUsingSecWebSocketKey();
      bool isUsingDeprecatedSecWebSocketKeys();
   protected:
      virtual void onDoubleCRLF(){}
      virtual void onSingleCRLF(){}
      Transport* mTransport;
      Tuple mWho;
      TransportFailure::FailureReason mFailureReason;      
      int mFailureSubCode;
      Compression &mCompression;
      osc::Stack *mSigcompStack;
      osc::TcpStream *mSigcompFramer;
      TransmissionFormat mSendingTransmissionFormat;
      TransmissionFormat mReceivingTransmissionFormat;

   private:
      SipMessage* mMessage;
      char* mBuffer;
      size_t mBufferPos;
      size_t mBufferSize;
      WsFrameExtractor mWsFrameExtractor;

      static char connectionStates[MAX][32];
      UInt64 mLastUsed;
      ConnState mConnState;
      MsgHeaderScanner mMsgHeaderScanner;

      static size_t messageSizeMax;

   public:
      static void setMessageSizeMax(size_t max)
         { messageSizeMax = max; };
};

EncodeStream& 
operator<<(EncodeStream& strm, const resip::ConnectionBase& c);

}

#endif
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
