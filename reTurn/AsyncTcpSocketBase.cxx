#include <boost/bind.hpp>

#include "AsyncTcpSocketBase.hxx"

using namespace std;

namespace reTurn {

AsyncTcpSocketBase::AsyncTcpSocketBase(asio::io_service& ioService) 
   : AsyncSocketBase(ioService),
     mSocket(ioService) 
{
}

AsyncTcpSocketBase::~AsyncTcpSocketBase() 
{
}

asio::error_code 
AsyncTcpSocketBase::bind(const asio::ip::address& address, unsigned short port)
{
   asio::error_code errorCode;
   mSocket.open(address.is_v6() ? asio::ip::tcp::v6() : asio::ip::tcp::v4(), errorCode);
   if(!errorCode)
   {
      mSocket.set_option(asio::ip::tcp::no_delay(true)); // ?slg? do we want this?
      mSocket.set_option(asio::ip::tcp::socket::reuse_address(true));
      mSocket.bind(asio::ip::tcp::endpoint(address, port), errorCode);
   }
   return errorCode;
}

unsigned int 
AsyncTcpSocketBase::getSocketDescriptor() 
{ 
   return mSocket.native(); 
}

const asio::ip::address 
AsyncTcpSocketBase::getSenderEndpointAddress() 
{ 
   return mSocket.remote_endpoint().address(); 
}

unsigned short 
AsyncTcpSocketBase::getSenderEndpointPort() 
{ 
   return mSocket.remote_endpoint().port(); 
}

void 
AsyncTcpSocketBase::transportSend(const StunTuple& destination, std::vector<asio::const_buffer>& buffers)
{
   // Note: destination is ignored for TCP
   asio::async_write(mSocket, buffers, 
                     boost::bind(&AsyncTcpSocketBase::handleSend, shared_from_this(), asio::placeholders::error));
}

void 
AsyncTcpSocketBase::transportReceive()
{
   mSocket.async_read_some(asio::buffer((void*)mReceiveBuffer->data(), RECEIVE_BUFFER_SIZE),
                           boost::bind(&AsyncTcpSocketBase::handleReceive, shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred));
}

void 
AsyncTcpSocketBase::transportFramedReceive()
{
   asio::async_read(mSocket, asio::buffer((void*)mReceiveBuffer->data(), 4),
                    boost::bind(&AsyncSocketBase::handleReadHeader, shared_from_this(), asio::placeholders::error));
}

void 
AsyncTcpSocketBase::handleReadHeader(const asio::error_code& e)
{
   if (!e)
   {
      /*
      std::cout << "Read header from tcp socket: " << std::endl;
      for(unsigned int i = 0; i < 4; i++)
      {
         std::cout << (char)(*mReceiveBuffer)[i] << "(" << (int)(*mReceiveBuffer)[i] << ") ";
      }
      std::cout << std::endl;
      */

      UInt16 dataLen;
      memcpy(&dataLen, &(*mReceiveBuffer)[2], 2);
      dataLen = ntohs(dataLen);

      if(dataLen+4 < RECEIVE_BUFFER_SIZE)
      {
         asio::async_read(mSocket, asio::buffer(&(*mReceiveBuffer)[4], dataLen),
                          boost::bind(&AsyncTcpSocketBase::handleReceive, shared_from_this(), asio::placeholders::error, dataLen+4));
      }
      else
      {
         std::cout << "Receive buffer (" << RECEIVE_BUFFER_SIZE << ") is not large enough to accomdate incoming framed data (" << dataLen+4 << ") closing connection." << std::endl;
         close();
      }
   }
   else if (e != asio::error::operation_aborted)
   {
      std::cout << "Read header error: " << e.message() << std::endl;
      close();
   }
}

void 
AsyncTcpSocketBase::transportClose()
{
   mSocket.close();
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

