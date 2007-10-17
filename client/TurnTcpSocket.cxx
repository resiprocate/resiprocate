#include "TurnTcpSocket.hxx"
#include <boost/bind.hpp>

using namespace std;

namespace reTurn {

TurnTcpSocket::TurnTcpSocket(const asio::ip::address& address, unsigned short port) : 
   TurnSocket(address,port),
   mSocket(mIOService)
{
   mLocalBinding.setTransportType(StunTuple::TCP);

   asio::error_code errorCode;
   mSocket.open(address.is_v6() ? asio::ip::tcp::v6() : asio::ip::tcp::v4(), errorCode);
   if(errorCode == 0)
   {
      mSocket.set_option(asio::ip::tcp::no_delay(true)); // ?slg? do we want this?
      mSocket.set_option(asio::ip::tcp::socket::reuse_address(true));
      mSocket.bind(asio::ip::tcp::endpoint(mLocalBinding.getAddress(), mLocalBinding.getPort()), errorCode);
   }
}

asio::error_code 
TurnTcpSocket::connect(const asio::ip::address& address, unsigned short port)
{
   asio::error_code errorCode;
   mSocket.connect(asio::ip::tcp::endpoint(address, port), errorCode);
   return errorCode;
}

asio::error_code 
TurnTcpSocket::rawWrite(const char* buffer, unsigned int size)
{
   asio::error_code errorCode;
   asio::write(mSocket, asio::buffer(buffer, size), asio::transfer_all(), errorCode);
   return errorCode;
}

asio::error_code 
TurnTcpSocket::rawWrite(const std::vector<asio::const_buffer>& buffers)
{
   asio::error_code errorCode;
   asio::write(mSocket, buffers, asio::transfer_all(), errorCode);
   return errorCode;
}

asio::error_code 
TurnTcpSocket::rawRead(char* buffer, unsigned int size, unsigned int timeout, unsigned int* bytesRead, asio::ip::address* sourceAddress, unsigned short* sourcePort)
{
   // !slg! Note: Only handles response comming back contiguously
   
   startReadTimer(timeout);
   mSocket.async_read_some(asio::buffer(buffer, size), boost::bind(&TurnTcpSocket::handleRawRead, this, asio::placeholders::error, asio::placeholders::bytes_transferred));

   // Wait for timer and read to end
   mIOService.run();
   mIOService.reset();

   *bytesRead = (unsigned int)mBytesRead;
   if(mReadErrorCode == 0)
   {
      if(sourceAddress)
      {
         *sourceAddress = mSocket.remote_endpoint().address();
      }
      if(sourcePort)
      {
         *sourcePort = mSocket.remote_endpoint().port();
      }
   }
   return mReadErrorCode;
}

void
TurnTcpSocket::cancelSocket()
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

