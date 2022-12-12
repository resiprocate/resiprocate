
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
   AsyncTlsSocketBase(asio::io_service& ioService, asio::ssl::context& context, bool validateServerCertificateHostname);

   unsigned int getSocketDescriptor() override;

   asio::error_code bind(const asio::ip::address& address, unsigned short port) override;
   void connect(const std::string& address, unsigned short port) override;

   void doHandshake();

   void transportReceive() override;
   void transportFramedReceive() override;
   void transportSend(const StunTuple& destination, std::vector<asio::const_buffer>& buffers) override;
   void transportClose() override;

   asio::ip::address getSenderEndpointAddress() override;
   unsigned short getSenderEndpointPort() override;

protected:
   void handleReadHeader(const asio::error_code& e) override;
   void handleServerHandshake(const asio::error_code& e) override;
   void handleTcpResolve(const asio::error_code& ec, asio::ip::tcp::resolver::iterator endpoint_iterator) override;
   void handleConnect(const asio::error_code& ec, asio::ip::tcp::resolver::iterator endpoint_iterator) override;
   void handleClientHandshake(const asio::error_code& ec, asio::ip::tcp::resolver::iterator endpoint_iterator) override;
   virtual bool validateServerCertificateHostname();

   virtual void onServerHandshakeSuccess() { resip_assert(false); }
   virtual void onServerHandshakeFailure(const asio::error_code& e) { resip_assert(false); }

   asio::ssl::stream<asio::ip::tcp::socket> mSocket;
   asio::ip::tcp::resolver mResolver;

private:
   std::string mHostname;
   bool mValidateServerCertificateHostname;
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
