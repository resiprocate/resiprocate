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
   if(!errorCode)
   {
      mSocket.set_option(asio::ip::tcp::no_delay(true)); // ?slg? do we want this?
      mSocket.set_option(asio::ip::tcp::socket::reuse_address(true));
      mSocket.bind(asio::ip::tcp::endpoint(mLocalBinding.getAddress(), mLocalBinding.getPort()), errorCode);
   }
}

asio::error_code 
TurnTcpSocket::connect(const std::string& address, unsigned short port)
{
   // Get a list of endpoints corresponding to the server name.
   asio::ip::tcp::resolver resolver(mIOService);
   resip::Data service(port);
   asio::ip::tcp::resolver::query query(address, service.c_str());   
   asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
   asio::ip::tcp::resolver::iterator end;

   // Try each endpoint until we successfully establish a connection.
   asio::error_code errorCode = asio::error::host_not_found;
   while (errorCode && endpoint_iterator != end)
   {
      mSocket.close();
      mSocket.connect(*endpoint_iterator, errorCode);

      if(!errorCode)
      {
         mConnected = true;
         mConnectedTuple.setTransportType(StunTuple::TCP);
         mConnectedTuple.setAddress(endpoint_iterator->endpoint().address());
         mConnectedTuple.setPort(endpoint_iterator->endpoint().port());
      }
      endpoint_iterator++;
   }

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
TurnTcpSocket::rawRead(unsigned int timeout, unsigned int* bytesRead, asio::ip::address* sourceAddress, unsigned short* sourcePort)
{
   startReadTimer(timeout);   
   readHeader();

   // Wait for timer and read to end
   mIOService.run();
   mIOService.reset();

   *bytesRead = (unsigned int)mBytesRead+4;
   if(!mReadErrorCode)
   {
      if(sourceAddress)
      {
         *sourceAddress = mConnectedTuple.getAddress();
      }
      if(sourcePort)
      {
         *sourcePort = mConnectedTuple.getPort();
      }
   }
   return mReadErrorCode;
}

void 
TurnTcpSocket::readHeader()
{
   asio::async_read(mSocket, asio::buffer(mReadBuffer, 4),
                    boost::bind(&TurnTcpSocket::handleReadHeader, this, asio::placeholders::error));
}

void 
TurnTcpSocket::readBody(unsigned int len)
{
   asio::async_read(mSocket, asio::buffer(&mReadBuffer[4], len),
                    boost::bind(&TurnTcpSocket::handleRawRead, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
}

void
TurnTcpSocket::cancelSocket()
{
   mSocket.cancel();
}

void 
TurnTcpSocket::handleReadHeader(const asio::error_code& e)
{
   if (!e)
   {
      /*
      std::cout << "Read header from turn tcp socket: " << std::endl;
      for(unsigned int i = 0; i < 4; i++)
      {
         std::cout << (char)mReadBuffer[i] << "(" << (int)mReadBuffer[i] << ") ";
      }
      std::cout << std::endl;
      */

      // All Turn messaging will be framed
      UInt16 dataLen;
      memcpy(&dataLen, &mReadBuffer[2], 2);
      dataLen = ntohs(dataLen);

      readBody(dataLen);
   }
   else 
   {
      mBytesRead = 0;
      mReadErrorCode = e;
      if (e != asio::error::operation_aborted)
      {
         std::cout << "Read header error: " << e.message() << std::endl;
         mReadTimer.cancel();
      }
   }
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

