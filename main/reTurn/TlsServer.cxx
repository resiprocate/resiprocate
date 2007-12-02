#include "TlsServer.hxx"
#include <boost/bind.hpp>
#include <rutil/WinLeakCheck.hxx>

namespace reTurn {

TlsServer::TlsServer(asio::io_service& ioService, RequestHandler& requestHandler, const asio::ip::address& address, unsigned short port, bool turnFraming)
: mIOService(ioService),
  mAcceptor(ioService),
  mContext(ioService, asio::ssl::context::tlsv1),
  mConnectionManager(),
  mRequestHandler(requestHandler),
  mTurnFraming(turnFraming)
{
   // Set Context options - TODO make into configuration settings
   mContext.set_options(asio::ssl::context::default_workarounds | 
                        asio::ssl::context::no_sslv2 |
                        asio::ssl::context::single_dh_use);
   mContext.set_password_callback(boost::bind(&TlsServer::getPassword, this));
   mContext.use_certificate_chain_file("server.pem");
   mContext.use_private_key_file("server.pem", asio::ssl::context::pem);
   mContext.use_tmp_dh_file("dh512.pem");

   // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
   asio::ip::tcp::endpoint endpoint(address, port);

   mAcceptor.open(endpoint.protocol());
   mAcceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
   mAcceptor.bind(endpoint);
   mAcceptor.listen();

   std::cout << (mTurnFraming ? "TURN" : "STUN") << " TlsServer started.  Listening on " << address << ":" << port << std::endl;
}

void
TlsServer::start()
{
   mNewConnection.reset(new TlsConnection(mIOService, mConnectionManager, mRequestHandler, mTurnFraming, mContext));
   mAcceptor.async_accept(((TlsConnection*)mNewConnection.get())->socket(), boost::bind(&TlsServer::handleAccept, this, asio::placeholders::error));
}

std::string 
TlsServer::getPassword() const
{
   return "test";
}

void 
TlsServer::handleAccept(const asio::error_code& e)
{
   if (!e)
   {
      mConnectionManager.start(mNewConnection);

      mNewConnection.reset(new TlsConnection(mIOService, mConnectionManager, mRequestHandler, mTurnFraming, mContext));
      mAcceptor.async_accept(((TlsConnection*)mNewConnection.get())->socket(), boost::bind(&TlsServer::handleAccept, this, asio::placeholders::error));
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

