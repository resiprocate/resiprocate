#include "TurnUdpSocket.hxx"
#include <boost/bind.hpp>

using namespace std;

namespace reTurn {

TurnUdpSocket::TurnUdpSocket(const asio::ip::address& address, unsigned short port, bool turnFramingDisabled) : 
   TurnSocket(address,port),
   mSocket(mIOService),
   mTurnFramingDisabled(turnFramingDisabled)
{
   mLocalBinding.setTransportType(StunTuple::UDP);

   asio::error_code errorCode;
   mSocket.open(address.is_v6() ? asio::ip::udp::v6() : asio::ip::udp::v4(), errorCode);
   if(errorCode == 0)
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
   asio::ip::udp::resolver::query query(address, service.c_str());   
   asio::ip::udp::resolver::iterator endpoint_iterator = resolver.resolve(query);
   asio::ip::udp::resolver::iterator end;

   // Use first endpoint in list
   if(endpoint_iterator == end)
   {
      return asio::error::host_not_found;
   }
   
   // Nothing to do for UDP except store the remote endpoint
   mRemoteEndpoint = endpoint_iterator->endpoint();

   mConnectedTuple.setTransportType(StunTuple::UDP);
   mConnectedTuple.setAddress(mRemoteEndpoint.address());
   mConnectedTuple.setPort(mRemoteEndpoint.port());

   return errorCode;
}

asio::error_code 
TurnUdpSocket::rawWrite(const char* buffer, unsigned int size)
{
   asio::error_code errorCode;
   if(mTurnFramingDisabled)
   {
      assert(size > 4);

      // Write everything except turn framing header
      mSocket.send_to(asio::buffer(&buffer[4], size-4), mRemoteEndpoint, 0, errorCode); 
   }
   else
   {
      mSocket.send_to(asio::buffer(buffer, size), mRemoteEndpoint, 0, errorCode); 
   }
   return errorCode;
}

asio::error_code 
TurnUdpSocket::rawWrite(const std::vector<asio::const_buffer>& buffers)
{
   asio::error_code errorCode;
   if(mTurnFramingDisabled)
   {
      // first buffer will be framing - only send second one
      assert(buffers.size() == 2);
      mSocket.send_to(asio::buffer(buffers.back()), mRemoteEndpoint, 0, errorCode); 
   }
   else
   {
      mSocket.send_to(buffers, mRemoteEndpoint, 0, errorCode); 
   }
   return errorCode;
}

asio::error_code 
TurnUdpSocket::rawRead(unsigned int timeout, unsigned int* bytesRead, asio::ip::address* sourceAddress, unsigned short* sourcePort)
{
   startReadTimer(timeout);

   if(mTurnFramingDisabled)
   {
      // If Turn Framing is disabled - fake that framing came from socket
      memset(mReadBuffer, 0, 4);  // set first four bytes (framing) to all 0's, first byte of 0 = stun/turn channel
      mSocket.async_receive_from(asio::buffer(&mReadBuffer[4], sizeof(mReadBuffer)-4), mSenderEndpoint, 0, boost::bind(&TurnUdpSocket::handleRawRead, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
   }
   else
   {
      mSocket.async_receive_from(asio::buffer(mReadBuffer, sizeof(mReadBuffer)), mSenderEndpoint, 0, boost::bind(&TurnUdpSocket::handleRawRead, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
   }

   // Wait for timer and read to end
   mIOService.run();
   mIOService.reset();

   if(mTurnFramingDisabled && mBytesRead > 0)
   {
      // Inflate read size to fake framing
      *bytesRead = (unsigned int)mBytesRead+4;
   }
   else
   {
      *bytesRead = (unsigned int)mBytesRead;
   }

   if(mReadErrorCode == 0)
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
   mSocket.cancel();
}

} // namespace


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

