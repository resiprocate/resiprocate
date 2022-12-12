#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifndef TLS_SERVER_HXX
#define TLS_SERVER_HXX

#ifdef USE_SSL

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <string>
#include "TlsConnection.hxx"
#include "ConnectionManager.hxx"
#include "RequestHandler.hxx"

namespace reTurn {

class TlsServer
{
public:
  // noncopyable
  TlsServer(const TlsServer&) = delete;
  TlsServer& operator=(const TlsServer&) = delete;

  /// Create the server to listen on the specified TCP address and port
  explicit TlsServer(asio::io_service& ioService, RequestHandler& requestHandler, const asio::ip::address& address, unsigned short port);

  void start();

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
  ConnectionPtr mNewConnection;

  /// The handler for all incoming requests.
  RequestHandler& mRequestHandler;
};

}

#endif 
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
