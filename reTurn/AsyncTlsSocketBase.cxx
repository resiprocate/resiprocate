#include <boost/bind.hpp>

#include "AsyncTlsSocketBase.hxx"
#include "AsyncSocketBaseHandler.hxx"

using namespace std;

namespace reTurn {

AsyncTlsSocketBase::AsyncTlsSocketBase(asio::io_service& ioService, asio::ssl::context& context) 
   : AsyncSocketBase(ioService),
   mSocket(mIOService, context)
{
}

AsyncTlsSocketBase::~AsyncTlsSocketBase() 
{
}

asio::error_code 
AsyncTlsSocketBase::bind(const asio::ip::address& address, unsigned short port)
{
   asio::error_code errorCode;
   mSocket.lowest_layer().open(address.is_v6() ? asio::ip::tcp::v6() : asio::ip::tcp::v4(), errorCode);
   if(!errorCode)
   {
      mSocket.lowest_layer().set_option(asio::ip::tcp::socket::reuse_address(true));
      mSocket.lowest_layer().set_option(asio::ip::tcp::no_delay(true)); // ?slg? do we want this?
      mSocket.lowest_layer().bind(asio::ip::tcp::endpoint(address, port), errorCode);
   }
   return errorCode;
}

void
AsyncTlsSocketBase::doHandshake()
{
   mSocket.async_handshake(asio::ssl::stream_base::server, 
                           boost::bind(&AsyncSocketBase::handleHandshake, shared_from_this(), asio::placeholders::error));  
}

void 
AsyncTlsSocketBase::handleHandshake(const asio::error_code& e)
{
   if(e)
   {
      if(mAsyncSocketBaseHandler) mAsyncSocketBaseHandler->onHandshakeFailure(getSocketDescriptor(), e);
   }
   else
   {
      if(mAsyncSocketBaseHandler) mAsyncSocketBaseHandler->onHandshakeSuccess(getSocketDescriptor());
   }
}

unsigned int 
AsyncTlsSocketBase::getSocketDescriptor() 
{ 
   return mSocket.lowest_layer().native(); 
}

const asio::ip::address 
AsyncTlsSocketBase::getSenderEndpointAddress() 
{ 
   return mSocket.lowest_layer().remote_endpoint().address(); 
}

unsigned short 
AsyncTlsSocketBase::getSenderEndpointPort() 
{ 
   return mSocket.lowest_layer().remote_endpoint().port(); 
}

void 
AsyncTlsSocketBase::transportSend(const StunTuple& destination, std::vector<asio::const_buffer>& buffers)
{
   // Note: destination is ignored for TLS
   asio::async_write(mSocket, buffers, 
                     boost::bind(&AsyncTlsSocketBase::handleSend, shared_from_this(), asio::placeholders::error));
}

void 
AsyncTlsSocketBase::transportReceive()
{
   mSocket.async_read_some(asio::buffer((void*)mReceiveBuffer->data(), RECEIVE_BUFFER_SIZE),
                           boost::bind(&AsyncTlsSocketBase::handleReceive, shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred));

}

void 
AsyncTlsSocketBase::transportFramedReceive()
{
   asio::async_read(mSocket, asio::buffer((void*)mReceiveBuffer->data(), 4),
                    boost::bind(&AsyncSocketBase::handleReadHeader, shared_from_this(), asio::placeholders::error));
}

void 
AsyncTlsSocketBase::transportClose()
{
   mSocket.shutdown();  // ?slg? Should we use async_shutdown?
   mSocket.lowest_layer().close();
}

void 
AsyncTlsSocketBase::handleReadHeader(const asio::error_code& e)
{
   if (!e)
   {
      /*
      std::cout << "Read header from tls socket: " << std::endl;
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
                          boost::bind(&AsyncTlsSocketBase::handleReceive, shared_from_this(), asio::placeholders::error, dataLen+4));
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

