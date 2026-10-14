
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#ifdef USE_SSL
#include <functional>

#include <openssl/err.h>
#include <openssl/opensslv.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include "AsyncTlsSocketBase.hxx"
#include "AsyncSocketBaseHandler.hxx"
#include <rutil/DnsUtil.hxx>
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;

namespace reTurn {

AsyncTlsSocketBase::AsyncTlsSocketBase(asio::io_context& ioService, asio::ssl::context& context, bool validateServerCertificateHostname) 
   : AsyncSocketBase(ioService),
   mSocket(ioService, context),
   mResolver(ioService),
   mHandshakeTimer(ioService),
   mServerHandshakeDone(false),
   mValidateServerCertificateHostname(validateServerCertificateHostname)
{
}

unsigned int 
AsyncTlsSocketBase::getSocketDescriptor() 
{ 
   return (unsigned int)mSocket.lowest_layer().native_handle();
}

asio::error_code 
AsyncTlsSocketBase::bind(const asio::ip::address& address, unsigned short port)
{
   // connect() binds the socket again, for each address it tries
   mLocalTcpEndpoint = asio::ip::tcp::endpoint(address, port);
   return openAndBind(mSocket.next_layer());
}

void 
AsyncTlsSocketBase::connect(const std::string& address, unsigned short port)
{
   mHostname = address;

   // Start an asynchronous resolve to translate the address
   // into a list of endpoints.
   resip::Data service(port);
   mResolver.async_resolve(mLocalTcpEndpoint.protocol(), address, service.c_str(),
        std::bind(&AsyncSocketBase::handleTcpResolve, shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2));
}

void 
AsyncTlsSocketBase::handleTcpResolve(const asio::error_code& ec,
                                     const asio::ip::tcp::resolver::results_type& endpoints)
{
   if (!ec)
   {
      // Attempt a connection to the first endpoint in the list. Each endpoint
      // will be tried until we successfully establish a connection (the handshake is with the
      // first that accepts)
      asyncConnect(mSocket.next_layer(), endpoints);
   }
   else
   {
      onConnectFailure(ec);
   }
}

bool
AsyncTlsSocketBase::setupClientSsl(SSL* ssl, const std::string& hostname, bool validateHostname)
{
   const bool isIpAddress = resip::DnsUtil::isIpAddress(hostname.c_str());

   // RFC 6066 section 3: SNI carries a host name, never an IP address
   if (!hostname.empty() && !isIpAddress && SSL_set_tlsext_host_name(ssl, hostname.c_str()) != 1)
   {
      WarningLog(<< "Failed to set SNI " << hostname << ", continuing without it");
   }

   bool ok = true;
   if (validateHostname)
   {
      // OpenSSL checks the server certificate against the name during the handshake, as RFC 6125
      // describes: a DNS or IP address subjectAltName, or the common name if there are no DNS
      // subjectAltNames, with wildcards only as a whole left-most label
      if (isIpAddress)
      {
         ok = X509_VERIFY_PARAM_set1_ip_asc(SSL_get0_param(ssl), hostname.c_str()) == 1;
      }
      else
      {
         SSL_set_hostflags(ssl, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
         ok = !hostname.empty() && SSL_set1_host(ssl, hostname.c_str()) == 1;
      }
      if (!ok)
      {
         ErrLog(<< "Unable to validate the server certificate against <" << hostname << ">");
      }
   }

   // Leave nothing on the error queue for the handshake: SSL_get_error() checks it first
   ERR_clear_error();
   return ok;
}

void
AsyncTlsSocketBase::logClientHandshakeResult(SSL* ssl, const std::string& hostname, const asio::error_code& ec)
{
   if (!ec)
   {
      const SSL_CIPHER* cipher = SSL_get_current_cipher(ssl);
      InfoLog(<< "TLS session set up with " << hostname << ": " << SSL_get_version(ssl)
              << " " << (cipher ? SSL_CIPHER_get_name(cipher) : "unknown cipher"));
   }
   else if (SSL_get_verify_result(ssl) != X509_V_OK)
   {
      // eg. X509_V_ERR_HOSTNAME_MISMATCH when the certificate isn't for hostname
      WarningLog(<< "TLS handshake with " << hostname << " failed, server certificate rejected: "
                 << X509_verify_cert_error_string(SSL_get_verify_result(ssl)));
   }
}

void
AsyncTlsSocketBase::handleConnect(const asio::error_code& ec,
                                  const asio::ip::tcp::endpoint& endpoint)
{
   if (!ec)
   {
      if (!setupClientSsl(mSocket.native_handle(), mHostname, mValidateServerCertificateHostname))
      {
         onConnectFailure(asio::error::invalid_argument);
         return;
      }

      // The connection was successful - now do handshake.
      mSocket.async_handshake(asio::ssl::stream_base::client,
                              std::bind(&AsyncSocketBase::handleClientHandshake, shared_from_this(),
                                          std::placeholders::_1, endpoint));
   }
   else
   {
      onConnectFailure(ec);
   }
}

void
AsyncTlsSocketBase::handleClientHandshake(const asio::error_code& ec,
                                          const asio::ip::tcp::endpoint& endpoint)
{
   logClientHandshakeResult(mSocket.native_handle(), mHostname, ec);
   if (!ec)
   {
      // The handshake was successful - and so was the check of the server certificate's name,
      // when that was asked for
      mConnected = true;
      mConnectedAddress = endpoint.address();
      mConnectedPort = endpoint.port();
      onConnectSuccess();
   }
   else
   {
      onConnectFailure(ec);
   }
}

void
AsyncTlsSocketBase::doHandshake(unsigned int timeoutSeconds)
{
   if (timeoutSeconds > 0)
   {
      // Don't let a client hold the connection open without ever completing the handshake
      std::shared_ptr<AsyncSocketBase> self = shared_from_this();
      mHandshakeTimer.expires_after(std::chrono::seconds(timeoutSeconds));
      mHandshakeTimer.async_wait([this, self](const asio::error_code& e)
      {
         if (e != asio::error::operation_aborted && !mServerHandshakeDone)
         {
            WarningLog(<< "TLS handshake not completed in time, closing the connection");
            asio::error_code ec;
            mSocket.lowest_layer().close(ec);  // the handshake then fails with operation_aborted
         }
      });
   }
   mSocket.async_handshake(asio::ssl::stream_base::server,
                           std::bind(&AsyncSocketBase::handleServerHandshake, shared_from_this(), std::placeholders::_1));
}

void
AsyncTlsSocketBase::handleServerHandshake(const asio::error_code& e)
{
   mServerHandshakeDone = true;
   mHandshakeTimer.cancel();
   if(e)
   {
      onServerHandshakeFailure(e);
   }
   else
   {
      asio::error_code ec;
      mConnectedAddress = mSocket.lowest_layer().remote_endpoint(ec).address();
      mConnectedPort = mSocket.lowest_layer().remote_endpoint(ec).port();

      onServerHandshakeSuccess();
   }
}

asio::ip::address 
AsyncTlsSocketBase::getSenderEndpointAddress() 
{ 
   return mConnectedAddress; 
}

unsigned short 
AsyncTlsSocketBase::getSenderEndpointPort() 
{ 
   return mConnectedPort; 
}

void 
AsyncTlsSocketBase::transportSend(const StunTuple& destination, std::vector<asio::const_buffer>& buffers)
{
   // Note: destination is ignored for TLS
   asio::async_write(mSocket, buffers, 
                     std::bind(&AsyncTlsSocketBase::handleSend, shared_from_this(), std::placeholders::_1));
}

void 
AsyncTlsSocketBase::transportReceive()
{
   mSocket.async_read_some(asio::buffer((void*)mReceiveBuffer->data(), RECEIVE_BUFFER_SIZE),
                           std::bind(&AsyncTlsSocketBase::handleReceive, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

}

void 
AsyncTlsSocketBase::transportFramedReceive()
{
   asio::async_read(mSocket, asio::buffer((void*)mReceiveBuffer->data(), 4),
                    std::bind(&AsyncSocketBase::handleReadHeader, shared_from_this(), std::placeholders::_1));
}

void 
AsyncTlsSocketBase::transportClose()
{
   if (mOnBeforeSocketCloseFp)
   {
      mOnBeforeSocketCloseFp((unsigned int)mSocket.lowest_layer().native_handle());
   }

   asio::error_code ec;
   //mSocket.shutdown(ec);  // ?slg? Should we use async_shutdown? !slg! note: this fn gives a stack overflow since ASIO 1.0.0 for some reason
   mSocket.lowest_layer().close(ec);
}

void 
AsyncTlsSocketBase::handleReadHeader(const asio::error_code& e)
{
   if (!e)
   {
      /*
      std::cout << "Read header from tls socket: " << std::endl;
      for(unsigned int i = 0; i < 4; i++)
      {
         std::cout << (char)(*mReceiveBuffer)[i] << "(" << (int)(*mReceiveBuffer)[i] << ") ";
      }
      std::cout << std::endl;
      */

      // Note:  For both StunMessages and ChannelData messages the length in bytes 3 and 4
      uint16_t dataLen;
      memcpy(&dataLen, &(*mReceiveBuffer)[2], 2);
      dataLen = ntohs(dataLen);

      // ChannelData's padding is read with it, but not passed on
      size_t padding = 0;
      if(((*mReceiveBuffer)[0] & 0xC0) == 0)  // If first 2 bits are 00 then this is a stun message
      {
         dataLen += 16;  // There are 20 bytes in total in the header, and we have already read 4 - read the rest of the header + the body
      }
      else
      {
         padding = channelDataPadding(dataLen);
      }

      if (dataLen + padding + 4U < RECEIVE_BUFFER_SIZE)
      {
         asio::async_read(mSocket, asio::buffer(&(*mReceiveBuffer)[4], dataLen + padding),
                          std::bind(&AsyncTlsSocketBase::handleReceive, shared_from_this(), std::placeholders::_1, dataLen+4));
      }
      else
      {
         WarningLog(<< "Receive buffer (" << RECEIVE_BUFFER_SIZE << ") is not large enough to accommodate incoming framed data (" << dataLen+4 << ") closing connection.");
         close();
      }
   }
   else if (e != asio::error::operation_aborted)
   {
      if(e != asio::error::eof && e != asio::error::connection_reset)
      {
         WarningLog(<< "Read header error: " << e.value() << "-" << e.message());
      }
      close();
   }
}

}
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

