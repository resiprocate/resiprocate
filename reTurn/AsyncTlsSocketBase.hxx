
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#ifdef USE_SSL
#ifndef ASYNC_TLS_SOCKET_BASE_HXX
#define ASYNC_TLS_SOCKET_BASE_HXX

#include <asio.hpp>
#include <asio/ssl.hpp>

#include "AsyncSocketBase.hxx"

namespace reTurn {

class AsyncTlsSocketBase : public AsyncSocketBase
{
public:
   AsyncTlsSocketBase(asio::io_context& ioService, asio::ssl::context& context, bool validateServerCertificateHostname);

   /// Sets up a client SSL to connect to hostname (a host name or IP address): SNI for a host
   /// name, and - when validateHostname - has OpenSSL check during the handshake that the
   /// server certificate is for hostname, so the handshake fails if it isn't.  The context's
   /// verify mode must include verify_peer for that check to apply.  Returns false if hostname
   /// can't be checked against.
   static bool setupClientSsl(SSL* ssl, const std::string& hostname, bool validateHostname);

   /// Logs a client handshake's outcome: the TLS version and cipher, or why the server
   /// certificate was rejected
   static void logClientHandshakeResult(SSL* ssl, const std::string& hostname, const asio::error_code& ec);

   unsigned int getSocketDescriptor() override;

   asio::error_code bind(const asio::ip::address& address, unsigned short port) override;
   void connect(const std::string& address, unsigned short port) override;

   /// Starts the server side handshake.  If timeoutSeconds isn't 0, the connection is closed if
   /// the handshake hasn't completed by then.
   void doHandshake(unsigned int timeoutSeconds = 0);

   void transportReceive() override;
   void transportFramedReceive() override;
   void transportSend(const StunTuple& destination, std::vector<asio::const_buffer>& buffers) override;
   void transportClose() override;

   asio::ip::address getSenderEndpointAddress() override;
   unsigned short getSenderEndpointPort() override;

protected:
   void handleReadHeader(const asio::error_code& e) override;
   void handleServerHandshake(const asio::error_code& e) override;
   void handleTcpResolve(const asio::error_code& ec, const asio::ip::tcp::resolver::results_type& endpoints) override;
   void handleConnect(const asio::error_code& ec, const asio::ip::tcp::endpoint& endpoint) override;
   void handleClientHandshake(const asio::error_code& ec, const asio::ip::tcp::endpoint& endpoint) override;

   virtual void onServerHandshakeSuccess() { resip_assert(false); }
   virtual void onServerHandshakeFailure(const asio::error_code& e) { resip_assert(false); }

   asio::ssl::stream<asio::ip::tcp::socket> mSocket;
   asio::ip::tcp::resolver mResolver;

private:
   asio::steady_timer mHandshakeTimer;
   bool mServerHandshakeDone;
   std::string mHostname;
   bool mValidateServerCertificateHostname;
};

}

#endif 
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
