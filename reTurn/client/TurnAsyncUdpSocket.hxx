#ifndef TURNASYNCUDPSOCKET_HXX
#define TURNASYNCUDPSOCKET_HXX

#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif
#include <boost/bind.hpp>

#include "TurnAsyncSocket.hxx"
#include "reTurn/AsyncUdpSocketBase.hxx"

namespace reTurn {

   class TurnAsyncUdpSocket : public TurnAsyncSocket, public AsyncUdpSocketBase
{
public:
   explicit TurnAsyncUdpSocket(asio::io_service& ioService,
                               TurnAsyncSocketHandler* turnAsyncSocketHandler,
                               const asio::ip::address& address, 
                               unsigned short port); 

   virtual unsigned int getSocketDescriptor() { return (unsigned int)mSocket.native(); }

protected:

private:
   // AsyncUdpSocketBase callbacks
   virtual void onConnectSuccess();
   virtual void onConnectFailure(const asio::error_code& e);
   virtual void onReceiveSuccess(const asio::ip::address& address, unsigned short port, boost::shared_ptr<DataBuffer>& data);
   virtual void onReceiveFailure(const asio::error_code& e);
   virtual void onSendSuccess();
   virtual void onSendFailure(const asio::error_code& e);
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
