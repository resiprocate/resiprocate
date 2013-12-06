#include "TlsServer.hxx"
#include <boost/bind.hpp>
#include <rutil/Data.hxx>
#include <rutil/WinLeakCheck.hxx>
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

namespace reTurn {

TlsServer::TlsServer(asio::io_service& ioService, RequestHandler& requestHandler, const asio::ip::address& address, unsigned short port)
: mIOService(ioService),
  mAcceptor(ioService),
  mContext(ioService, asio::ssl::context::tlsv1),  // TLSv1.0
  mConnectionManager(),
  mRequestHandler(requestHandler)
{
   // Set Context options 
   asio::error_code ec;
   mContext.set_options(asio::ssl::context::default_workarounds |  // Implement various bug workarounds.
                        asio::ssl::context::no_sslv2 | // Disable SSL v2.
                        asio::ssl::context::single_dh_use);  // enforce recalculation of the DH key for eatch session
   
   mContext.set_password_callback(boost::bind(&TlsServer::getPassword, this));

   // Use a certificate chain from a file.
   mContext.use_certificate_chain_file(mRequestHandler.getConfig().mTlsServerCertificateFilename.c_str(), ec);
   if(ec)
   {
      ErrLog(<< "Unable to load server cert chain file: " << mRequestHandler.getConfig().mTlsServerCertificateFilename << ", error=" << ec.value() << "(" << ec.message() << ")");
      throw asio::system_error(ec);
   }

   // Use a private key from a file.
   resip::Data keyFilename = mRequestHandler.getConfig().mTlsServerPrivateKeyFilename;
   if(keyFilename.empty())
   {
      keyFilename = mRequestHandler.getConfig().mTlsServerCertificateFilename;
   }
   mContext.use_private_key_file(keyFilename.c_str(), asio::ssl::context::pem, ec);
   if(ec)
   {
      ErrLog(<< "Unable to load server private key file: " << keyFilename << ", error=" << ec.value() << "(" << ec.message() << ")");
      throw asio::system_error(ec);
   }

   // Use the specified file to obtain the temporary Diffie-Hellman parameters.
   mContext.use_tmp_dh_file(mRequestHandler.getConfig().mTlsTempDhFilename.c_str(), ec);
   if(ec)
   {
      ErrLog(<< "Unable to load temporary Diffie-Hellman parameters file: " << mRequestHandler.getConfig().mTlsTempDhFilename << ", error=" << ec.value() << "(" << ec.message() << ")");
      throw asio::system_error(ec);
   }

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

   InfoLog(<< "TlsServer started.  Listening on " << address.to_string() << ":" << port);
}

void
TlsServer::start()
{
   mNewConnection.reset(new TlsConnection(mIOService, mConnectionManager, mRequestHandler, mContext));
   mAcceptor.async_accept(((TlsConnection*)mNewConnection.get())->socket(), boost::bind(&TlsServer::handleAccept, this, asio::placeholders::error));
}

std::string 
TlsServer::getPassword() const
{
   return mRequestHandler.getConfig().mTlsPrivateKeyPassword.c_str();
}

void 
TlsServer::handleAccept(const asio::error_code& e)
{
   if (!e)
   {
      mConnectionManager.start(mNewConnection);

      mNewConnection.reset(new TlsConnection(mIOService, mConnectionManager, mRequestHandler, mContext));
      mAcceptor.async_accept(((TlsConnection*)mNewConnection.get())->socket(), boost::bind(&TlsServer::handleAccept, this, asio::placeholders::error));
   }
   else
   {
      ErrLog(<< "Error in handleAccept: " << e.value() << "-" << e.message());
      if(e == asio::error::no_descriptors)
      {
         // Retry if too many open files (ie. out of socket descriptors)
         mNewConnection.reset(new TlsConnection(mIOService, mConnectionManager, mRequestHandler, mContext));
         mAcceptor.async_accept(((TlsConnection*)mNewConnection.get())->socket(), boost::bind(&TlsServer::handleAccept, this, asio::placeholders::error));
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
