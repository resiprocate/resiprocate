#ifndef TURNASYNCTCPSOCKET_HXX
#define TURNASYNCTCPSOCKET_HXX

#include <asio.hpp>

#include "TurnAsyncSocket.hxx"
#include "../AsyncTcpSocketBase.hxx"

namespace reTurn {

class TurnAsyncTcpSocket : public TurnAsyncSocket, public AsyncTcpSocketBase
{
public:
   explicit TurnAsyncTcpSocket(asio::io_service& ioService, 
                               TurnAsyncSocketHandler* turnAsyncSocketHandler,
                               const asio::ip::address& address = UnspecifiedIpAddress, 
                               unsigned short port = 0,
                               bool turnFraming = true);

   virtual unsigned int getSocketDescriptor() { return mSocket.native(); }

   virtual bool isConnected() const { return mConnected; }
   virtual const asio::ip::address& getConnectedAddress() const { return mConnectedAddress; }
   virtual unsigned short getConnectedPort() const { return mConnectedPort; }

   virtual void send(resip::SharedPtr<resip::Data> data);  // Send unframed data
   virtual void send(unsigned short channel, resip::SharedPtr<resip::Data> data);  // send with turn framing
   virtual void receive();

   virtual void close();

protected:

private:
   // AsyncTcpSocketBase callbacks
   virtual void onConnectSuccess();
   virtual void onConnectFailure(const asio::error_code& e);
   virtual void onReceiveSuccess(const asio::ip::address& address, unsigned short port, resip::SharedPtr<resip::Data> data);
   virtual void onReceiveFailure(const asio::error_code& e);
   virtual void onSendSuccess();
   virtual void onSendFailure(const asio::error_code& e);
};

} 

#endif


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

