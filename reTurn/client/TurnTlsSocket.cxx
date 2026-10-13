
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#ifdef USE_SSL
#include <functional>

#include "TurnTlsSocket.hxx"
#include "../AsyncTlsSocketBase.hxx"
#include <openssl/ssl.h>
#include <rutil/Logger.hxx>
#include "../ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;

namespace reTurn {

// The client's context, fully set up before mSocket is created from it: the SSL object takes the
// context's verify mode when it's created, not later changes
static asio::ssl::context
makeClientContext(const std::string& rootCertificatesFile)
{
   asio::ssl::context context(asio::ssl::context::sslv23);  // SSLv23 (actually chooses TLS version dynamically)



   // Enable certificate validation
   context.set_verify_mode(asio::ssl::context::verify_peer |   // Verify the peer.
                           asio::ssl::context::verify_fail_if_no_peer_cert);  // Fail verification if the peer has no certificate.

   // File that should contain all required root certificates
   asio::error_code ec;
   context.load_verify_file(rootCertificatesFile, ec);
   if (ec)
   {
      ErrLog(<< "Unable to load TLS root certificates from " << rootCertificatesFile << ", error=" << ec.value() << "(" << ec.message() << ")");
   }
   return context;
}

TurnTlsSocket::TurnTlsSocket(bool validateServerCertificateHostname, 
                             const asio::ip::address& address, 
                             unsigned short port,
                             const std::string& rootCertificatesFile) : 
   TurnTcpSocket(address,port),
   mSslContext(makeClientContext(rootCertificatesFile)),
   mSocket(mIOService, mSslContext),
   mValidateServerCertificateHostname(validateServerCertificateHostname)
{
   mLocalBinding.setTransportType(StunTuple::TLS);

   asio::error_code errorCode;
   mSocket.lowest_layer().open(address.is_v6() ? asio::ip::tcp::v6() : asio::ip::tcp::v4(), errorCode);
   if(!errorCode)
   {
      mSocket.lowest_layer().set_option(asio::ip::tcp::socket::reuse_address(true));
      mSocket.lowest_layer().set_option(asio::ip::tcp::no_delay(true)); // ?slg? do we want this?
      mSocket.lowest_layer().bind(asio::ip::tcp::endpoint(mLocalBinding.getAddress(), mLocalBinding.getPort()), errorCode);
   }
}

asio::error_code 
TurnTlsSocket::connect(const std::string& address, unsigned short port)
{
   // Get a list of endpoints corresponding to the server name.
   asio::ip::tcp::resolver resolver(mIOService);
   resip::Data service(port);
   asio::ip::tcp::resolver::results_type results = resolver.resolve(mSocket.lowest_layer().local_endpoint().protocol(), address, service.c_str());
   asio::ip::tcp::resolver::results_type::const_iterator endpoint_iterator = results.begin();
   asio::ip::tcp::resolver::results_type::const_iterator end = results.end();

   // SNI, and the check that the server certificate is for address, made during the handshake
   if (!AsyncTlsSocketBase::setupClientSsl(mSocket.native_handle(), address, mValidateServerCertificateHostname))
   {
      return asio::error::invalid_argument;
   }

   // Try each endpoint until we successfully establish a connection.
   asio::error_code errorCode = asio::error::host_not_found;
   while (errorCode && endpoint_iterator != end)
   {
      mSocket.lowest_layer().close();
      mSocket.lowest_layer().connect(*endpoint_iterator, errorCode);
      if(!errorCode)
      {
         DebugLog(<< "Connected!");
         mSocket.handshake(asio::ssl::stream_base::client, errorCode);
         AsyncTlsSocketBase::logClientHandshakeResult(mSocket.native_handle(), address, errorCode);
         if(!errorCode)
         {
            mConnected = true;
            mConnectedTuple.setTransportType(StunTuple::TLS);
            mConnectedTuple.setAddress(endpoint_iterator->endpoint().address());
            mConnectedTuple.setPort(endpoint_iterator->endpoint().port());
         }
      }
      ++endpoint_iterator;
   }

   return errorCode;
}

asio::error_code 
TurnTlsSocket::rawWrite(const char* buffer, unsigned int size)
{
   asio::error_code errorCode;
   asio::write(mSocket, asio::buffer(buffer, size), asio::transfer_all(), errorCode); 
   return errorCode;
}

asio::error_code 
TurnTlsSocket::rawWrite(const std::vector<asio::const_buffer>& buffers)
{
   asio::error_code errorCode;
   asio::write(mSocket, buffers, asio::transfer_all(), errorCode);
   return errorCode;
}

void 
TurnTlsSocket::readHeader()
{
   asio::async_read(mSocket, asio::buffer(mReadBuffer, 4),
                    std::bind(&TurnTlsSocket::handleReadHeader, this, std::placeholders::_1));
}

void 
TurnTlsSocket::readBody(unsigned int len)
{
   asio::async_read(mSocket, asio::buffer(&mReadBuffer[4], len),
                    std::bind(&TurnTlsSocket::handleRawRead, this, std::placeholders::_1, std::placeholders::_2));
}

void
TurnTlsSocket::cancelSocket()
{
   asio::error_code ec;
   mSocket.lowest_layer().cancel(ec);
}

} // namespace
#endif

/* ====================================================================

 Copyright (c) 2008-2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
