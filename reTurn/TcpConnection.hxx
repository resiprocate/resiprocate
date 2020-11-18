#ifndef TCP_CONNECTION_HXX
#define TCP_CONNECTION_HXX

#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif
#include "RequestHandler.hxx"
#include "StunTuple.hxx"
#include "AsyncTcpSocketBase.hxx"
#include "TurnAllocationManager.hxx"

namespace reTurn {

class ConnectionManager;

/// Represents a single connection from a client.
class TcpConnection
  : public AsyncTcpSocketBase
{
public:
   /// Construct a connection with the given io_service.
   explicit TcpConnection(asio::io_service& ioService, ConnectionManager& manager, RequestHandler& handler);
   TcpConnection(const TcpConnection&) = delete;
   TcpConnection(TcpConnection&&) = delete;
   ~TcpConnection();

   TcpConnection& operator=(const TcpConnection&) = delete;
   TcpConnection& operator=(TcpConnection&&) = delete;

   /// Get the socket associated with the connection.
   asio::ip::tcp::socket& socket();

   /// Start the first asynchronous operation for the connection.
   virtual void start();

   /// Override close fn in AsyncTcpSocketBase so that we can remove ourselves from connection manager
   virtual void close();

   /// Stop all asynchronous operations associated with the connection.
   virtual void stop();

protected:
   /// Handle completion of a receive operation
   virtual void onReceiveSuccess(const asio::ip::address& address, unsigned short port, const std::shared_ptr<DataBuffer>& data);
   virtual void onReceiveFailure(const asio::error_code& e);

   /// Handle completion of a send operation
   virtual void onSendSuccess();
   virtual void onSendFailure(const asio::error_code& e);

   /// The connection manager for this connection.
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
