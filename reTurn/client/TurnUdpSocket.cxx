#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "TurnUdpSocket.hxx"

using namespace std;

namespace reTurn {

TurnUdpSocket::TurnUdpSocket(const asio::ip::address& address, unsigned short port) : 
   TurnSocket(address,port),
   mSocket(mIOService)   
{
   mLocalBinding.setTransportType(StunTuple::UDP);

   asio::error_code errorCode;
   mSocket.open(address.is_v6() ? asio::ip::udp::v6() : asio::ip::udp::v4(), errorCode);
   if(!errorCode)
   {
      mSocket.set_option(asio::ip::udp::socket::reuse_address(true));
      mSocket.bind(asio::ip::udp::endpoint(mLocalBinding.getAddress(), mLocalBinding.getPort()), errorCode);
   }
}

asio::error_code 
TurnUdpSocket::connect(const std::string& address, unsigned short port)
{
   asio::error_code errorCode;

   // Get a list of endpoints corresponding to the server name.
   asio::ip::udp::resolver resolver(mIOService);
   resip::Data service(port);
#ifdef USE_IPV6
   asio::ip::udp::resolver::query query(address, service.c_str());   
#else
   asio::ip::udp::resolver::query query(asio::ip::udp::v4(), address, service.c_str());   
#endif
   asio::ip::udp::resolver::iterator endpoint_iterator = resolver.resolve(query);
   asio::ip::udp::resolver::iterator end;

   // Use first endpoint in list
   if(endpoint_iterator == end)
   {
      return asio::error::host_not_found;
   }
   
   // Nothing to do for UDP except store the remote endpoint
   mRemoteEndpoint = endpoint_iterator->endpoint();

   mConnected = true;
   mConnectedTuple.setTransportType(StunTuple::UDP);
   mConnectedTuple.setAddress(mRemoteEndpoint.address());
   mConnectedTuple.setPort(mRemoteEndpoint.port());

   return errorCode;
}

asio::error_code 
TurnUdpSocket::rawWrite(const char* buffer, unsigned int size)
{
   asio::error_code errorCode;
   mSocket.send_to(asio::buffer(buffer, size), mRemoteEndpoint, 0, errorCode); 
   return errorCode;
}

asio::error_code 
TurnUdpSocket::rawWrite(const std::vector<asio::const_buffer>& buffers)
{
   asio::error_code errorCode;
   mSocket.send_to(buffers, mRemoteEndpoint, 0, errorCode); 
   return errorCode;
}

asio::error_code 
TurnUdpSocket::rawRead(unsigned int timeout, unsigned int* bytesRead, asio::ip::address* sourceAddress, unsigned short* sourcePort)
{
   startReadTimer(timeout);

   mSocket.async_receive_from(asio::buffer(mReadBuffer, sizeof(mReadBuffer)), mSenderEndpoint, 0, boost::bind(&TurnUdpSocket::handleRawRead, this, asio::placeholders::error, asio::placeholders::bytes_transferred));

   // Wait for timer and read to end
   mIOService.run();
   mIOService.reset();

   *bytesRead = (unsigned int)mBytesRead;

   if(!mReadErrorCode)
   {
      if(sourceAddress)
      {
         *sourceAddress = mSenderEndpoint.address();
      }
      if(sourcePort)
      {
         *sourcePort = mSenderEndpoint.port();
      }
   }
   return mReadErrorCode;
}

void
TurnUdpSocket::cancelSocket()
{
   asio::error_code ec;
   mSocket.cancel(ec);
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
