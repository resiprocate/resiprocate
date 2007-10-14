#include "TcpConnection.hxx"
#include <vector>
#include <boost/bind.hpp>
#include "ConnectionManager.hxx"
#include "RequestHandler.hxx"

namespace reTurn {

TcpConnection::TcpConnection(asio::io_service& ioService,
    ConnectionManager& manager, RequestHandler& handler)
  : TurnTransportBase(ioService),
    mSocket(ioService),
    mConnectionManager(manager),
    mRequestHandler(handler),
    mBufferLen(0),
    mTransportType(StunTuple::TCP)
{
}

TcpConnection::~TcpConnection()
{
   std::cout << "TcpConnection destroyed." << std::endl;
}

asio::ip::tcp::socket& 
TcpConnection::socket()
{
  return mSocket;
}

void 
TcpConnection::start()
{
   std::cout << "TcpConnection started." << std::endl;
   mLocalEndpoint = mSocket.local_endpoint();
   mRemoteEndpoint = mSocket.remote_endpoint();
   readHeader();
}

void
TcpConnection::readHeader()
{
   asio::async_read(mSocket, asio::buffer(mBuffer, 4),
                    boost::bind(&TcpConnection::handleReadHeader, shared_from_this(), asio::placeholders::error));
}

void
TcpConnection::readBody()
{
   asio::async_read(mSocket, asio::buffer(&mBuffer[mReadBufferPos], mBufferLen-mReadBufferPos),
                    boost::bind(&TcpConnection::handleReadBody, shared_from_this(), asio::placeholders::error));
}

void
TcpConnection::write()
{
   async_write(mSocket, asio::buffer(mBuffer, mBufferLen),  
               boost::bind(&TcpConnection::handleWrite, shared_from_this(), asio::placeholders::error));
}

void 
TcpConnection::stop()
{
  mSocket.close();
}

void 
TcpConnection::handleReadHeader(const asio::error_code& e)
{
   if (!e)
   {
      /*
      std::cout << "Read header from tcp socket: " << std::endl;
      for(unsigned int i = 0; i < 4; i++)
      {
         std::cout << (char)mBuffer[i] << "(" << (int)mBuffer[i] << ") ";
      }
      std::cout << std::endl;
      */

      // Note:  we only accept the following:
      //        1.  Unframed Stun Requests - first octet 0x00
      //        2.  Framed Stun Requests - first octet 0x02
      //        3.  Framed Data - first octet 0x03
      if(mBuffer[0] == 0x00)  // Unframed Stun
      {
         // This is likely a StunMessage - length will be in bytes 3 and 4
         UInt16 stunMsgLen;
         memcpy(&stunMsgLen, &mBuffer[2], 2);
         stunMsgLen = ntohs(stunMsgLen) + 20;  // 20 bytes for header
         mReadBufferPos = 4;  // Don't overwrite part of StunMessage header already read
         mBufferLen = stunMsgLen;
         if(stunMsgLen >= 0)
         {
            std::cout << "Reading StunMessage with length=" << stunMsgLen << std::endl;
            mReadingStunMessage = true;
            readBody();
         }
      }
      else if(mBuffer[0] == 0x02) // Framed Stun Request
      {
         // This is a StunMessage - length will be in bytes 3 and 4
         UInt16 stunMsgLen;
         memcpy(&stunMsgLen, &mBuffer[2], 2);
         stunMsgLen = ntohs(stunMsgLen);  // Framed length will be entire size of StunMessage
         mReadBufferPos = 0;   // Overwrite framing info and read in entire StunMessage
         mBufferLen = stunMsgLen;
         if(stunMsgLen >= 0)
         {
            std::cout << "Reading StunMessage with length=" << stunMsgLen << std::endl;
            mReadingStunMessage = true;
            readBody();
         }
      }
      else if(mBuffer[0] == 0x03) // Framed Data
      {
         UInt16 dataLen;
         memcpy(&dataLen, &mBuffer[2], 2);
         dataLen = ntohs(dataLen);
         mReadBufferPos = 0;
         mBufferLen = dataLen;
         mReadingStunMessage = false;
         readBody();
      }
      else
      {
         std::cout << "Invalid data on TCP connection." << std::endl;
         mConnectionManager.stop(shared_from_this());
      }
   }
   else if (e != asio::error::operation_aborted)
   {
      std::cout << "Read header error: " << e << std::endl;
      mConnectionManager.stop(shared_from_this());
   }
}

void 
TcpConnection::handleReadBody(const asio::error_code& e)
{
   if (!e)
   {
      /*
      std::cout << "Read " << mBufferLen << " bytes from tcp socket: " << std::endl;
      for(unsigned int i = 0; i < mBufferLen; i++)
      {
         std::cout << (char)mBuffer[i] << "(" << (int)mBuffer[i] << ") ";
      }
      std::cout << std::endl;
      */

      if(mReadingStunMessage)
      {
         StunMessage request(StunTuple(mTransportType, mLocalEndpoint.address(), mLocalEndpoint.port()),
                             StunTuple(mTransportType, mRemoteEndpoint.address(), mRemoteEndpoint.port()),
                             (char*)mBuffer.c_array(), mBufferLen, 0, 0, 0);
         if(request.isValid())
         {
            StunMessage response;
            RequestHandler::ProcessResult result = mRequestHandler.processStunMessage(this, request, response);

            switch(result)
            {
            case RequestHandler::NoResponseToSend:
               // No response to send - just receive next message
               readHeader();
               return;
            case RequestHandler::RespondFromAlternatePort:
            case RequestHandler::RespondFromAlternateIp:
            case RequestHandler::RespondFromAlternateIpPort:
               // These only happen for UDP server for RFC3489 backwards compatibility
               assert(false);
               break;
            case RequestHandler::RespondFromReceiving:
            default:
               break;
            }
            mBufferLen = response.stunEncodeMessage((char*)mBuffer.c_array(), (unsigned int)mBuffer.size());

            write();
         }
         else
         {
            std::cout << "Invalid StunMessage received, ending connection." << std::endl;
            mConnectionManager.stop(shared_from_this());
         }
      }
      else  // reading turn data
      {
         mRequestHandler.processTurnData(StunTuple(mTransportType, mLocalEndpoint.address(), mLocalEndpoint.port()),
                                         StunTuple(mTransportType, mRemoteEndpoint.address(), mRemoteEndpoint.port()),
                                         (char*)mBuffer.c_array(), mBufferLen);
         readHeader();
      }
   }
   else if (e != asio::error::operation_aborted)
   {
      std::cout << "Read body error: " << e << std::endl;
      mConnectionManager.stop(shared_from_this());
   }
}

void 
TcpConnection::handleWrite(const asio::error_code& e)
{
   if(!e)
   {
      readHeader();
   }
   else if (e != asio::error::operation_aborted)
   {
      mConnectionManager.stop(shared_from_this());
   }
}

void 
TcpConnection::sendData(const StunTuple& destination, const char* buffer, unsigned int size)
{
   async_write(mSocket, asio::buffer(buffer, size),  
               boost::bind(&TurnTransportBase::handleSendData, this, asio::placeholders::error));  // !slg! note:  not using shared_from_this()
}

} 


/* ====================================================================

 Original contribution Copyright (C) 2007 Plantronics, Inc.
 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */

