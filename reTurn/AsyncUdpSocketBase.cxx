#include <boost/bind.hpp>

#include "AsyncUdpSocketBase.hxx"
#include "AsyncSocketBaseHandler.hxx"
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;

namespace reTurn {

AsyncUdpSocketBase::AsyncUdpSocketBase(asio::io_service& ioService) 
   : AsyncSocketBase(ioService),
     mSocket(ioService),
     mResolver(ioService)
{
}

AsyncUdpSocketBase::~AsyncUdpSocketBase() 
{
}

unsigned int 
AsyncUdpSocketBase::getSocketDescriptor() 
{ 
   return (unsigned int)mSocket.native(); 
}

asio::error_code 
AsyncUdpSocketBase::bind(const asio::ip::address& address, unsigned short port)
{
   asio::error_code errorCode;
   mSocket.open(address.is_v6() ? asio::ip::udp::v6() : asio::ip::udp::v4(), errorCode);
   if(!errorCode)
   {
#ifdef USE_IPV6
#ifdef __linux__
      if(address.is_v6())
      {
         asio::ip::v6_only v6_opt(true);
         mSocket.set_option(v6_opt);
      }
#endif
#endif
      mSocket.set_option(asio::ip::udp::socket::reuse_address(true), errorCode);
      mSocket.set_option(asio::socket_base::receive_buffer_size(66560));
      //mSocket.set_option(asio::socket_base::send_buffer_size(66560));
      mSocket.bind(asio::ip::udp::endpoint(address, port), errorCode);
   }
   return errorCode;
}

void 
AsyncUdpSocketBase::connect(const std::string& address, unsigned short port)
{
   // Start an asynchronous resolve to translate the address
   // into a list of endpoints.
   resip::Data service(port);
#ifdef USE_IPV6
   asio::ip::udp::resolver::query query(address, service.c_str());
#else
   asio::ip::udp::resolver::query query(asio::ip::udp::v4(), address, service.c_str());   
#endif
   mResolver.async_resolve(query,
        boost::bind(&AsyncSocketBase::handleUdpResolve, shared_from_this(),
                    asio::placeholders::error,
                    asio::placeholders::iterator));
}

void 
AsyncUdpSocketBase::handleUdpResolve(const asio::error_code& ec,
                                     asio::ip::udp::resolver::iterator endpoint_iterator)
{
   if (!ec)
   {
      // Use the first endpoint in the list. 
      // Nothing to do for UDP except store the connected address/port
      mConnected = true;
      mConnectedAddress = endpoint_iterator->endpoint().address();
      mConnectedPort = endpoint_iterator->endpoint().port();

      onConnectSuccess();
   }
   else
   {
      onConnectFailure(ec);
   }
}

const asio::ip::address 
AsyncUdpSocketBase::getSenderEndpointAddress() 
{ 
   return mSenderEndpoint.address(); 
}

unsigned short 
AsyncUdpSocketBase::getSenderEndpointPort() 
{ 
   return mSenderEndpoint.port(); 
}

void 
AsyncUdpSocketBase::transportSend(const StunTuple& destination, std::vector<asio::const_buffer>& buffers)
{
   //InfoLog(<< "AsyncUdpSocketBase::transportSend " << buffers.size() << " buffer(s) to " << destination << " - buf1 size=" << buffer_size(buffers.front()));
   mSocket.async_send_to(buffers, 
                         asio::ip::udp::endpoint(destination.getAddress(), destination.getPort()), 
                         boost::bind(&AsyncUdpSocketBase::handleSend, shared_from_this(), asio::placeholders::error));
}

void 
AsyncUdpSocketBase::transportReceive()
{
   mSocket.async_receive_from(asio::buffer((void*)mReceiveBuffer->data(), RECEIVE_BUFFER_SIZE), mSenderEndpoint,
               boost::bind(&AsyncUdpSocketBase::handleReceive, shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred));
}

void 
AsyncUdpSocketBase::transportFramedReceive()
{
   // For UDP these two functions are the same
   transportReceive();
}

void 
AsyncUdpSocketBase::transportClose()
{
   if (mOnBeforeSocketCloseFp)
   {
      mOnBeforeSocketCloseFp((unsigned int)mSocket.native());
   }

   asio::error_code ec;
   mSocket.close(ec);
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

