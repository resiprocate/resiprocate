#ifndef TCP_CONNECTION_HXX
#define TCP_CONNECTION_HXX

#include <asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "RequestHandler.hxx"
#include "StunTuple.hxx"
#include "TurnTransportBase.hxx"

namespace reTurn {

class ConnectionManager;

/// Represents a single connection from a client.
class TcpConnection
  : public TurnTransportBase,
    public boost::enable_shared_from_this<TcpConnection>,
    private boost::noncopyable
{
public:
  /// Construct a connection with the given io_service.
  explicit TcpConnection(asio::io_service& ioService, ConnectionManager& manager, RequestHandler& handler);
  ~TcpConnection();

  /// Get the socket associated with the connection.
  asio::ip::tcp::socket& socket();

  /// Start the first asynchronous operation for the connection.
  virtual void start();

  /// Start reading the header
  virtual void readHeader();

  /// Read the message body
  virtual void readBody();

  /// Write buffer to socket
  virtual void write();

  /// Stop all asynchronous operations associated with the connection.
  virtual void stop();

  /// Send Data
  virtual void sendData(const StunTuple& destination, const char* buffer, unsigned int size);

protected:
  /// Handle completion of a read operation for first 32 bits
  virtual void handleReadHeader(const asio::error_code& e);

  /// Handle completion of a read operation for entire message
  virtual void handleReadBody(const asio::error_code& e);

  /// Handle completion of a write operation.
  virtual void handleWrite(const asio::error_code& e);

  /// Socket for the connection.
  asio::ip::tcp::socket mSocket;

  /// The manager for this connection.
  ConnectionManager& mConnectionManager;

  /// The handler used to process the incoming request.
  RequestHandler& mRequestHandler;

  /// Buffer for incoming data.
  boost::array<char, 8192> mBuffer;

  /// Amount of data in buffer
  unsigned int mBufferLen;
  bool mReadingStunMessage;
  unsigned char mChannelNumber;

  /// Transport Type
  StunTuple::TransportType mTransportType;

  /// Endpoint info
  asio::ip::tcp::endpoint mLocalEndpoint;
  asio::ip::tcp::endpoint mRemoteEndpoint;

private:
};

typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

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

