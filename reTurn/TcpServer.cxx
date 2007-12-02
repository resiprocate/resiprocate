#include "TcpServer.hxx"
#include <boost/bind.hpp>
#include <rutil/WinLeakCheck.hxx>

namespace reTurn {

TcpServer::TcpServer(asio::io_service& ioService, RequestHandler& requestHandler, const asio::ip::address& address, unsigned short port, bool turnFraming)
: mIOService(ioService),
  mAcceptor(ioService),
  mConnectionManager(),
  mNewConnection(new TcpConnection(ioService, mConnectionManager, requestHandler, turnFraming)),
  mRequestHandler(requestHandler),
  mTurnFraming(turnFraming)
{
   // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
   asio::ip::tcp::endpoint endpoint(address, port);

   mAcceptor.open(endpoint.protocol());
   mAcceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
   mAcceptor.bind(endpoint);
   mAcceptor.listen();

   std::cout << (mTurnFraming ? "TURN" : "STUN") << " TcpServer started.  Listening on " << address << ":" << port << std::endl;
}

void 
TcpServer::start()
{
   mAcceptor.async_accept(((TcpConnection*)mNewConnection.get())->socket(), boost::bind(&TcpServer::handleAccept, this, asio::placeholders::error));
}

void 
TcpServer::handleAccept(const asio::error_code& e)
{
   if (!e)
   {
      mConnectionManager.start(mNewConnection);

      mNewConnection.reset(new TcpConnection(mIOService, mConnectionManager, mRequestHandler, mTurnFraming));
      mAcceptor.async_accept(((TcpConnection*)mNewConnection.get())->socket(), boost::bind(&TcpServer::handleAccept, this, asio::placeholders::error));
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

