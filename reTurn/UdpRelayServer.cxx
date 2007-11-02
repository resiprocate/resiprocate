#include <boost/bind.hpp>

#include "UdpRelayServer.hxx"
#include "StunMessage.hxx"
#include "TurnAllocation.hxx"
#include "StunTuple.hxx"

using namespace std;

namespace reTurn {

UdpRelayServer::UdpRelayServer(asio::io_service& ioService, TurnAllocation& turnAllocation)
: TurnTransportBase(ioService),
  mSocket(ioService, asio::ip::udp::endpoint(turnAllocation.getRequestedTuple().getAddress(), turnAllocation.getRequestedTuple().getPort())),
  mTurnAllocation(turnAllocation)
{
   std::cout << "UdpRelayServer started.  Listening on " << mTurnAllocation.getRequestedTuple().getAddress() << ":" << mTurnAllocation.getRequestedTuple().getPort() << std::endl;
}

UdpRelayServer::~UdpRelayServer()
{
   cout << "~UdpRelayServer - socket destroyed" << endl;
}

void 
UdpRelayServer::start()
{
   // Note:  This function is required, since you cannot call shared_from_this in the constructor: shared_from_this requires that at least one shared ptr exists already
   mSocket.async_receive_from(asio::buffer(mBuffer), mSenderEndpoint,
      boost::bind(&UdpRelayServer::handleReceiveFrom, shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred));
}

void 
UdpRelayServer::stop()
{
   mSocket.close();
   mStopping = true;
}

asio::ip::udp::socket& 
UdpRelayServer::getSocket()
{
   return mSocket;
}

void 
UdpRelayServer::handleReceiveFrom(const asio::error_code& e, std::size_t bytesTransferred)
{
   if(mStopping)
   {
      return;
   }
   if (!e && bytesTransferred > 0)
   {      
      std::cout << "Read " << (int)bytesTransferred << " bytes from udp relay socket (" << mSenderEndpoint.address().to_string() << ":" << mSenderEndpoint.port() << "): " << std::hex << std::endl;
      /*
      for(int i = 0; i < (int)bytesTransferred; i++)
      {
         std::cout << (char)mBuffer[i] << "(" << int(mBuffer[i]) << ") ";
      }
      std::cout << std::dec << std::endl;
      */

      // If no permission then just drop packet
      if(mTurnAllocation.existsPermission(mSenderEndpoint.address())) 
      {
         // If active destination is not set, then send to client as a DataInd, otherwise send packet as is
         mTurnAllocation.sendDataToClient(StunTuple(StunTuple::UDP, mSenderEndpoint.address(), mSenderEndpoint.port()),
                                          resip::Data(resip::Data::Share, (const char*)mBuffer.data(), (int)bytesTransferred));
      }
   }

   if(e != asio::error::operation_aborted)
   {
      mSocket.async_receive_from(asio::buffer(mBuffer), mSenderEndpoint,
         boost::bind(&UdpRelayServer::handleReceiveFrom, shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred));
   }
}

void 
UdpRelayServer::sendData(const StunTuple& destination, const char* buffer, unsigned int size)
{
   std::cout << "UdpRelayServer: sending " << size << " bytes to " << destination << std::endl;

   mSocket.async_send_to(asio::buffer(buffer, size), 
                                 asio::ip::udp::endpoint(destination.getAddress(), destination.getPort()), 
                                 boost::bind(&TurnTransportBase::handleSendData, shared_from_this(), asio::placeholders::error));
}

}


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

