#include <boost/bind.hpp>

#include "AsyncTlsSocketBase.hxx"
#include "AsyncSocketBaseHandler.hxx"
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;

namespace reTurn {

AsyncTlsSocketBase::AsyncTlsSocketBase(asio::io_service& ioService, asio::ssl::context& context) 
   : AsyncSocketBase(ioService),
   mSocket(ioService, context),
   mResolver(ioService)
{
}

AsyncTlsSocketBase::~AsyncTlsSocketBase() 
{
}

unsigned int 
AsyncTlsSocketBase::getSocketDescriptor() 
{ 
   return mSocket.lowest_layer().native(); 
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
AsyncTlsSocketBase::connect(const std::string& address, unsigned short port)
{
   // Start an asynchronous resolve to translate the address
   // into a list of endpoints.
   resip::Data service(port);
   asio::ip::tcp::resolver::query query(address, service.c_str());   
   mResolver.async_resolve(query,
        boost::bind(&AsyncSocketBase::handleTcpResolve, shared_from_this(),
                    asio::placeholders::error,
                    asio::placeholders::iterator));
}

void 
AsyncTlsSocketBase::handleTcpResolve(const asio::error_code& ec,
                                  asio::ip::tcp::resolver::iterator endpoint_iterator)
{
   if (!ec)
   {
      // Attempt a connection to the first endpoint in the list. Each endpoint
      // will be tried until we successfully establish a connection.
      //asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
      mSocket.lowest_layer().async_connect(endpoint_iterator->endpoint(),
                            boost::bind(&AsyncSocketBase::handleConnect, shared_from_this(),
                            asio::placeholders::error, endpoint_iterator));
   }
   else
   {
      onConnectFailure(ec);
   }
}

void 
AsyncTlsSocketBase::handleConnect(const asio::error_code& ec,
                                  asio::ip::tcp::resolver::iterator endpoint_iterator)
{
   if (!ec)
   {
      // The connection was successful - now do handshake.
      mSocket.async_handshake(asio::ssl::stream_base::client, 
                              boost::bind(&AsyncSocketBase::handleClientHandshake, shared_from_this(), 
                                          asio::placeholders::error, endpoint_iterator));
   }
   else if (++endpoint_iterator != asio::ip::tcp::resolver::iterator())
   {
      // The connection failed. Try the next endpoint in the list.
      mSocket.lowest_layer().close();
      mSocket.lowest_layer().async_connect(endpoint_iterator->endpoint(),
                            boost::bind(&AsyncSocketBase::handleConnect, shared_from_this(),
                            asio::placeholders::error, endpoint_iterator));
   }
   else
   {
      onConnectFailure(ec);
   }
}

void 
AsyncTlsSocketBase::handleClientHandshake(const asio::error_code& ec,
                                          asio::ip::tcp::resolver::iterator endpoint_iterator)
{
   if (!ec)
   {
      // The handshake was successful.
      mConnected = true;
      mConnectedAddress = endpoint_iterator->endpoint().address();
      mConnectedPort = endpoint_iterator->endpoint().port();

      onConnectSuccess();
   }
   else if (++endpoint_iterator != asio::ip::tcp::resolver::iterator())
   {
      // The handshake failed. Try the next endpoint in the list.
      mSocket.lowest_layer().close();
      mSocket.lowest_layer().async_connect(endpoint_iterator->endpoint(),
                            boost::bind(&AsyncSocketBase::handleConnect, shared_from_this(),
                            asio::placeholders::error, endpoint_iterator));
   }
   else
   {
      onConnectFailure(ec);
   }
}

void
AsyncTlsSocketBase::doHandshake()
{
   mSocket.async_handshake(asio::ssl::stream_base::server, 
                           boost::bind(&AsyncSocketBase::handleServerHandshake, shared_from_this(), asio::placeholders::error));  
}

void 
AsyncTlsSocketBase::handleServerHandshake(const asio::error_code& e)
{
   if(e)
   {
      onServerHandshakeFailure(e);
   }
   else
   {
      mConnectedAddress = mSocket.lowest_layer().remote_endpoint().address();
      mConnectedPort = mSocket.lowest_layer().remote_endpoint().port();

      onServerHandshakeSuccess();
   }
}

const asio::ip::address 
AsyncTlsSocketBase::getSenderEndpointAddress() 
{ 
   return mConnectedAddress; 
}

unsigned short 
AsyncTlsSocketBase::getSenderEndpointPort() 
{ 
   return mConnectedPort; 
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
   asio::error_code ec;
   //mSocket.shutdown(ec);  // ?slg? Should we use async_shutdown? !slg! note: this fn gives a stack overflow since ASIO 1.0.0 for some reason
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

      if(((*mReceiveBuffer)[0] & 0xC0) == 0)  // If first 2 bits are 00 then this is a stun message
      {
         dataLen += 16;  // There are 20 bytes in total in the header, and we have already read 4 - read the rest of the header + the body
      }

      if(dataLen+4 < RECEIVE_BUFFER_SIZE)
      {
         asio::async_read(mSocket, asio::buffer(&(*mReceiveBuffer)[4], dataLen),
                          boost::bind(&AsyncTlsSocketBase::handleReceive, shared_from_this(), asio::placeholders::error, dataLen+4));
      }
      else
      {
         WarningLog(<< "Receive buffer (" << RECEIVE_BUFFER_SIZE << ") is not large enough to accomdate incoming framed data (" << dataLen+4 << ") closing connection.");
         close();
      }
   }
   else if (e != asio::error::operation_aborted)
   {
      if(e != asio::error::eof && e != asio::error::connection_reset)
      {
         WarningLog(<< "Read header error: " << e.value() << "-" << e.message());
      }
      close();
   }
}

}


/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */

