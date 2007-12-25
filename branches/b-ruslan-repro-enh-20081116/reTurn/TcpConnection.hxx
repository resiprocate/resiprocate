#ifndef TCP_CONNECTION_HXX
#define TCP_CONNECTION_HXX

#include <asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "RequestHandler.hxx"
#include "StunTuple.hxx"
#include "AsyncTcpSocketBase.hxx"

namespace reTurn {

class ConnectionManager;

/// Represents a single connection from a client.
class TcpConnection
  : public AsyncTcpSocketBase,
    private boost::noncopyable
{
public:
   /// Construct a connection with the given io_service.
   explicit TcpConnection(asio::io_service& ioService, ConnectionManager& manager, RequestHandler& handler, bool turnFraming);
   ~TcpConnection();

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
   virtual void onReceiveSuccess(const asio::ip::address& address, unsigned short port, resip::SharedPtr<resip::Data> data);
   virtual void onReceiveFailure(const asio::error_code& e);

   /// Handle completion of a send operation
   virtual void onSendSuccess();
   virtual void onSendFailure(const asio::error_code& e);

   /// The manager for this connection.
   ConnectionManager& mConnectionManager;

   /// The handler used to process the incoming request.
   RequestHandler& mRequestHandler;

   bool mTurnFraming;

private:
};

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

