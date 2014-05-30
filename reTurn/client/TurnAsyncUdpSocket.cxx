#include "TurnAsyncUdpSocket.hxx"

// Remove warning about 'this' use in initiator list - pointer is only stored
#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4355 ) // using this in base member initializer list 
#endif

using namespace std;

#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4355 )
#endif

namespace reTurn {

TurnAsyncUdpSocket::TurnAsyncUdpSocket(asio::io_service& ioService,
                                       TurnAsyncSocketHandler* turnAsyncSocketHandler,
                                       const asio::ip::address& address, 
                                       unsigned short port) : 
   TurnAsyncSocket(ioService, *this, turnAsyncSocketHandler, address, port),
   AsyncUdpSocketBase(ioService)
{
   mLocalBinding.setTransportType(StunTuple::UDP);
   bind(address, port);
}

void 
TurnAsyncUdpSocket::onConnectSuccess()
{
   if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onConnectSuccess(getSocketDescriptor(), mConnectedAddress, mConnectedPort);
   turnReceive();
}

void 
TurnAsyncUdpSocket::onConnectFailure(const asio::error_code& e)
{
   if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onConnectFailure(getSocketDescriptor(), e);
}

void 
TurnAsyncUdpSocket::onReceiveSuccess(const asio::ip::address& address, unsigned short port, boost::shared_ptr<DataBuffer>& data)
{
   handleReceivedData(address, port, data);
   turnReceive();
}

void 
TurnAsyncUdpSocket::onReceiveFailure(const asio::error_code& e)
{
   if (e.value() == 234) // ?jjg? ERROR_MORE_DATA -- is this right on all platforms?
   {
      turnReceive();
   }
   else
   {
      if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onReceiveFailure(getSocketDescriptor(), e);
   }
}
 
void 
TurnAsyncUdpSocket::onSendSuccess()
{
   if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onSendSuccess(getSocketDescriptor());
}
 
void 
TurnAsyncUdpSocket::onSendFailure(const asio::error_code& e)
{
   if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onSendFailure(getSocketDescriptor(), e);
}

} // namespace


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
