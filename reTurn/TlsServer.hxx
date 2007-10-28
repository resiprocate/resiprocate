#ifndef TLS_SERVER_HXX
#define TLS_SERVER_HXX

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <string>
#include <boost/noncopyable.hpp>
#include "TlsConnection.hxx"
#include "ConnectionManager.hxx"
#include "RequestHandler.hxx"

namespace reTurn {

class TlsServer
  : private boost::noncopyable
{
public:
  /// Create the server to listen on the specified TCP address and port
  explicit TlsServer(asio::io_service& ioService, RequestHandler& requestHandler, const asio::ip::address& address, unsigned short port);

private:
  /// Handle completion of an asynchronous accept operation.
  void handleAccept(const asio::error_code& e);

  /// Callback for private key password
  std::string getPassword() const;

  /// The io_service used to perform asynchronous operations.
  asio::io_service& mIOService;

  /// Acceptor used to listen for incoming connections.
  asio::ip::tcp::acceptor mAcceptor;

  /// OpenSSL context
  asio::ssl::context mContext;

  /// The connection manager which owns all live connections.
  ConnectionManager mConnectionManager;

  /// The next connection to be accepted.
  TlsConnectionPtr mNewConnection;

  /// The handler for all incoming requests.
  RequestHandler& mRequestHandler;
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

