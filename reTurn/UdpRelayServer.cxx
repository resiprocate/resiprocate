#include <boost/bind.hpp>

#include "UdpRelayServer.hxx"
#include "StunMessage.hxx"
#include "TurnAllocation.hxx"
#include "StunTuple.hxx"
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;

namespace reTurn {

UdpRelayServer::UdpRelayServer(asio::io_service& ioService, TurnAllocation& turnAllocation)
: AsyncUdpSocketBase(ioService),
  mTurnAllocation(turnAllocation),
  mStopping(false),
  mBindSuccess(false)
{
   asio::error_code ec = bind(turnAllocation.getRequestedTuple().getAddress(), turnAllocation.getRequestedTuple().getPort());
   if(ec)
   {
      ErrLog(<< "UdpRelayServer failed to start properly.  [" << mTurnAllocation.getRequestedTuple().getAddress().to_string() << ":" << mTurnAllocation.getRequestedTuple().getPort() << "], error=" << ec.value() << "-" << ec.message());
   }
   else
   {
      mBindSuccess = true;
      InfoLog(<< "UdpRelayServer started.  [" << mTurnAllocation.getRequestedTuple().getAddress().to_string() << ":" << mTurnAllocation.getRequestedTuple().getPort() << "]");
   }
}

UdpRelayServer::~UdpRelayServer()
{
   InfoLog(<< "~UdpRelayServer - destroyed.  [" << mTurnAllocation.getRequestedTuple().getAddress().to_string() << ":" << mTurnAllocation.getRequestedTuple().getPort() << "]");
}

bool
UdpRelayServer::startReceiving()
{
   if(mBindSuccess)
   {
      // Note:  This function is required, since you cannot call shared_from_this in the constructor: shared_from_this requires that at least one shared ptr exists already
      doReceive();
      return true;
   }
   else
   {
      stop();
      return false;
   }
}

void 
UdpRelayServer::stop()
{
   asio::error_code ec;
   mStopping = true;
   mSocket.close(ec);
}

void 
UdpRelayServer::onReceiveSuccess(const asio::ip::address& address, unsigned short port, boost::shared_ptr<DataBuffer>& data)
{
   if(mStopping)
   {
      return;
   }
   if (data->size() > 0)
   {      
      DebugLog(<< "Read " << (int)data->size() << " bytes from udp relay socket (" << address.to_string() << ":" << port << "): ");
      /*
      cout << std::hex;
      for(int i = 0; i < (int)data->size(); i++)
      {
         std::cout << (char)(*data)[i] << "(" << int((*data)[i]) << ") ";
      }
      std::cout << std::dec << std::endl;
      */

      // If active destination is not set, then send to client as a DataInd, otherwise send packet as is
      mTurnAllocation.sendDataToClient(StunTuple(StunTuple::UDP, address, port), data);
   }
   doReceive();
}

void 
UdpRelayServer::onReceiveFailure(const asio::error_code& e)
{
   if(!mStopping && e != asio::error::operation_aborted && e != asio::error::bad_descriptor)
   {
      doReceive();
   }
}

void
UdpRelayServer::onSendSuccess()
{
}

void
UdpRelayServer::onSendFailure(const asio::error_code& error)
{
   if(!mStopping && error != asio::error::operation_aborted)
   {
      if(mLastSendErrorCode == error)
      {
         DebugLog(<< "UdpRelayServer::onSendFailure: [" << mTurnAllocation.getRequestedTuple().getAddress().to_string() << ":" << mTurnAllocation.getRequestedTuple().getPort() << "] " << error.value() << "-" << error.message());
      }
      else
      {
         mLastSendErrorCode = error;
         WarningLog(<< "UdpRelayServer::onSendFailure: [" << mTurnAllocation.getRequestedTuple().getAddress().to_string() << ":" << mTurnAllocation.getRequestedTuple().getPort() << "] " << error.value() << "-" << error.message());
      }
   }
}

}


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
