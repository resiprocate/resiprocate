#ifndef ASYNC_TCP_SOCKET_BASE_HXX
#define ASYNC_TCP_SOCKET_BASE_HXX

#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif
#include <boost/bind.hpp>

#include "AsyncSocketBase.hxx"

namespace reTurn {

class AsyncTcpSocketBase : public AsyncSocketBase
{
public:
   AsyncTcpSocketBase(asio::io_service& ioService); 
   virtual ~AsyncTcpSocketBase();

   virtual unsigned int getSocketDescriptor();

   virtual asio::error_code bind(const asio::ip::address& address, unsigned short port);
   virtual void connect(const std::string& address, unsigned short port);  

   virtual void transportReceive();
   virtual void transportFramedReceive();
   virtual void transportSend(const StunTuple& destination, std::vector<asio::const_buffer>& buffers);
   virtual void transportClose();

   virtual void setConnectedAddressAndPort();  // Used by server side so that get fn's will work
   virtual const asio::ip::address getSenderEndpointAddress();
   virtual unsigned short getSenderEndpointPort();

protected:
   virtual void handleReadHeader(const asio::error_code& e);
   virtual void handleTcpResolve(const asio::error_code& ec, asio::ip::tcp::resolver::iterator endpoint_iterator);
   virtual void handleConnect(const asio::error_code& ec, asio::ip::tcp::resolver::iterator endpoint_iterator);

   asio::ip::tcp::socket mSocket;
   asio::ip::tcp::resolver mResolver;

private:
};

}

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
