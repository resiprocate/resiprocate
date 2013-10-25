#include "TcpServer.hxx"
#include <boost/bind.hpp>
#include <rutil/WinLeakCheck.hxx>
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

namespace reTurn {

TcpServer::TcpServer(asio::io_service& ioService, RequestHandler& requestHandler, const asio::ip::address& address, unsigned short port)
: mIOService(ioService),
  mAcceptor(ioService),
  mConnectionManager(),
  mNewConnection(new TcpConnection(ioService, mConnectionManager, requestHandler)),
  mRequestHandler(requestHandler)
{
   // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
   asio::ip::tcp::endpoint endpoint(address, port);

   mAcceptor.open(endpoint.protocol());
   mAcceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
#ifdef USE_IPV6
#ifdef __linux__
   if(address.is_v6())
   {
      asio::ip::v6_only v6_opt(true);
      mAcceptor.set_option(v6_opt);
   }
#endif
#endif
   mAcceptor.bind(endpoint);
   mAcceptor.listen();

   InfoLog(<< "TcpServer started.  Listening on " << address.to_string() << ":" << port);
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

      mNewConnection.reset(new TcpConnection(mIOService, mConnectionManager, mRequestHandler));
      mAcceptor.async_accept(((TcpConnection*)mNewConnection.get())->socket(), boost::bind(&TcpServer::handleAccept, this, asio::placeholders::error));
   }
   else
   {
      ErrLog(<< "Error in handleAccept: " << e.value() << "-" << e.message());
      if(e == asio::error::no_descriptors)
      {
         // Retry if too many open files (ie. out of socket descriptors)
         mNewConnection.reset(new TcpConnection(mIOService, mConnectionManager, mRequestHandler));
         mAcceptor.async_accept(((TcpConnection*)mNewConnection.get())->socket(), boost::bind(&TcpServer::handleAccept, this, asio::placeholders::error));
      }
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
