#ifndef TLS_CONNECTION_HXX
#define TLS_CONNECTION_HXX

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include "RequestHandler.hxx"
#include "AsyncTlsSocketBase.hxx"

namespace reTurn {

class ConnectionManager;

typedef asio::ssl::stream<asio::ip::tcp::socket> ssl_socket;

/// Represents a single connection from a client.
class TlsConnection
  : public AsyncTlsSocketBase,
    private boost::noncopyable
{
public:
  /// Construct a connection with the given io_service.
   explicit TlsConnection(asio::io_service& ioService, ConnectionManager& manager, RequestHandler& handler, asio::ssl::context& context);
  ~TlsConnection();

   /// Get the socket associated with the connection.
   ssl_socket::lowest_layer_type& socket();

   /// Start the first asynchronous operation for the connection.
   virtual void start();

   /// Override close fn in AsyncTcpSocketBase so that we can remove ourselves from connection manager
   virtual void close();

   /// Stop all asynchronous operations associated with the connection.
   virtual void stop();

protected:
   /// Handle completion of a handshake operation
   virtual void onServerHandshakeSuccess();
   virtual void onServerHandshakeFailure(const asio::error_code& e);

   /// Handle completion of a receive operation
   virtual void onReceiveSuccess(const asio::ip::address& address, unsigned short port, boost::shared_ptr<DataBuffer>& data);
   virtual void onReceiveFailure(const asio::error_code& e);

   /// Handle completion of a send operation
   virtual void onSendSuccess();
   virtual void onSendFailure(const asio::error_code& e);

   /// The manager for this connection.
   ConnectionManager& mConnectionManager;

   /// Manages turn allocations
   TurnAllocationManager mTurnAllocationManager;

   /// The handler used to process the incoming request.
   RequestHandler& mRequestHandler;

   // Stores the local address and port
   asio::ip::address mLocalAddress;
   unsigned short mLocalPort;

private:
};

}

#endif 


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
