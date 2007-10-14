#ifdef WIN32
#pragma warning(disable : 4267)
#endif

#include "TlsConnection.hxx"
#include <vector>
#include <boost/bind.hpp>
#include "ConnectionManager.hxx"
#include "RequestHandler.hxx"

namespace reTurn {

TlsConnection::TlsConnection(asio::io_service& ioService,
                             ConnectionManager& manager, RequestHandler& handler, asio::ssl::context& context)
  : TcpConnection(ioService, manager, handler),
    mTlsSocket(ioService, context)
{
   mTransportType = StunTuple::TLS;  // Override TCP transport type
}

TlsConnection::~TlsConnection()
{
   std::cout << "TlsConnection destroyed." << std::endl;
}

ssl_socket::lowest_layer_type& 
TlsConnection::socket()
{
  return mTlsSocket.lowest_layer();
}

void 
TlsConnection::start()
{
   std::cout << "TlsConnection started." << std::endl;

   mLocalEndpoint = mTlsSocket.lowest_layer().local_endpoint();
   mRemoteEndpoint = mTlsSocket.lowest_layer().remote_endpoint();

   mTlsSocket.async_handshake(asio::ssl::stream_base::server, 
                              boost::bind(&TlsConnection::handleHandshake, this, asio::placeholders::error));  // !slg! using this instead of shared_from_this()
}

void
TlsConnection::readHeader()
{
   asio::async_read(mTlsSocket, asio::buffer(mBuffer, 4),
                    boost::bind(&TlsConnection::handleReadHeader, shared_from_this(), asio::placeholders::error));
}

void
TlsConnection::readBody()
{
   asio::async_read(mTlsSocket, asio::buffer(&mBuffer[mReadBufferPos], mBufferLen-mReadBufferPos),
                    boost::bind(&TlsConnection::handleReadBody, shared_from_this(), asio::placeholders::error));
}

void
TlsConnection::write()
{
   async_write(mTlsSocket, asio::buffer(mBuffer, mBufferLen),  
               boost::bind(&TlsConnection::handleWrite, shared_from_this(), asio::placeholders::error));
}

void 
TlsConnection::stop()
{
  asio::error_code ec;
  mTlsSocket.shutdown(ec);
  std::cout << "TlsConnection shutdown, e=" << ec << std::endl;
}

void 
TlsConnection::handleHandshake(const asio::error_code& e)
{
   std::cout << "TlsConnection: handleHandshake, e=" << e << std::endl;

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
TlsConnection::sendData(const StunTuple& destination, const char* buffer, unsigned int size)
{
   async_write(mTlsSocket, asio::buffer(buffer, size),  
               boost::bind(&TurnTransportBase::handleSendData, this, asio::placeholders::error));  // !slg! using this instead of shared_from_this()
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

