#ifndef TLS_CONNECTION_HXX
#define TLS_CONNECTION_HXX

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include "RequestHandler.hxx"
#include "TcpConnection.hxx"

namespace reTurn {

class ConnectionManager;
typedef asio::ssl::stream<asio::ip::tcp::socket> ssl_socket;

/// Represents a single connection from a client.
class TlsConnection
  : public TcpConnection
{
public:
  /// Construct a connection with the given io_service.
   explicit TlsConnection(asio::io_service& ioService, ConnectionManager& manager, RequestHandler& handler, asio::ssl::context& context);
  ~TlsConnection();

  /// Get the socket associated with the connection.
  ssl_socket::lowest_layer_type& socket();

  /// Start the first asynchronous operation for the connection.
  virtual void start();

  /// Start reading the header
  virtual void readHeader();

  /// Read the Stun message body
  virtual void readBody();

  /// Write buffer to socket
  virtual void write();

  /// Stop all asynchronous operations associated with the connection.
  virtual void stop();

  virtual void sendData(const StunTuple& destination, const char* buffer, unsigned int size);

private:
  /// Handle completion of a ssl handshake
  virtual void handleHandshake(const asio::error_code& e);

  /// Socket for the connection.
  ssl_socket mTlsSocket;
};

typedef boost::shared_ptr<TlsConnection> TlsConnectionPtr;

}

#endif 


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

