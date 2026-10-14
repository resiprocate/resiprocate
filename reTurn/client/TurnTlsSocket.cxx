
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
   mSocket(new asio::ssl::stream<asio::ip::tcp::socket>(mIOService, mSslContext)),
   mValidateServerCertificateHostname(validateServerCertificateHostname)
{
   mLocalBinding.setTransportType(StunTuple::TLS);
   // connect() opens the socket and binds it to address and port
}

asio::error_code
TurnTlsSocket::connect(const std::string& address, unsigned short port)
{
   // Get a list of endpoints corresponding to the server name.
   asio::ip::tcp::resolver resolver(mIOService);
   resip::Data service(port);
   asio::error_code errorCode;
   asio::ip::tcp::resolver::results_type results = resolver.resolve(protocol(), address, service.c_str(), errorCode);
   if(errorCode)
   {
      return errorCode;
   }
   asio::ip::tcp::resolver::results_type::const_iterator endpoint_iterator = results.begin();
   asio::ip::tcp::resolver::results_type::const_iterator end = results.end();

   // Try each endpoint until we successfully establish a connection
   errorCode = asio::error::host_not_found;
   while (errorCode && endpoint_iterator != end)
   {
      // A new SSL stream, on a new socket bound to the local binding: after a failed handshake
      // the SSL object can't be used again, nor anything left in the stream's buffers
      mSocket.reset(new asio::ssl::stream<asio::ip::tcp::socket>(mIOService, mSslContext));

      // SNI, and the check that the server certificate is for address, made during the handshake
      if (!AsyncTlsSocketBase::setupClientSsl(mSocket->native_handle(), address, mValidateServerCertificateHostname))
      {
         return asio::error::invalid_argument;
      }

      errorCode = openAndBind(mSocket->next_layer());
      if(!errorCode)
      {
         mSocket->lowest_layer().connect(*endpoint_iterator, errorCode);
      }
      if(!errorCode)
      {
         DebugLog(<< "Connected!");
         mSocket->handshake(asio::ssl::stream_base::client, errorCode);
         AsyncTlsSocketBase::logClientHandshakeResult(mSocket->native_handle(), address, errorCode);
         if(!errorCode)
         {
            mConnected = true;
            mConnectedTuple.setTransportType(StunTuple::TLS);
            mConnectedTuple.setAddress(endpoint_iterator->endpoint().address());
            mConnectedTuple.setPort(endpoint_iterator->endpoint().port());
         }
         // As the async client: only a failed TCP connect moves on to the next address.  A
         // handshake that fails (eg. the certificate isn't for the name) would fail with the
         // others too - and the handshake has no timeout, so trying one that accepts the TCP
         // connection but never answers would block.
         return errorCode;
      }
      ++endpoint_iterator;
   }

   return errorCode;
}

asio::error_code 
TurnTlsSocket::rawWrite(const char* buffer, unsigned int size)
{
   asio::error_code errorCode;
   asio::write(*mSocket, asio::buffer(buffer, size), asio::transfer_all(), errorCode); 
   return errorCode;
}

asio::error_code 
TurnTlsSocket::rawWrite(const std::vector<asio::const_buffer>& buffers)
{
   asio::error_code errorCode;
   asio::write(*mSocket, buffers, asio::transfer_all(), errorCode);
   return errorCode;
}

void 
TurnTlsSocket::readHeader()
{
   asio::async_read(*mSocket, asio::buffer(mReadBuffer, 4),
                    std::bind(&TurnTlsSocket::handleReadHeader, this, std::placeholders::_1));
}

void 
TurnTlsSocket::readBody(unsigned int len)
{
   asio::async_read(*mSocket, asio::buffer(&mReadBuffer[4], len),
                    std::bind(&TurnTlsSocket::handleRawRead, this, std::placeholders::_1, std::placeholders::_2));
}

void
TurnTlsSocket::cancelSocket()
{
   asio::error_code ec;
   mSocket->lowest_layer().cancel(ec);
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
