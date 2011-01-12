#include "TurnTcpSocket.hxx"
#include <boost/bind.hpp>
#include <rutil/Logger.hxx>
#include "../ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;

namespace reTurn {

TurnTcpSocket::TurnTcpSocket(const boost::asio::ip::address& address, unsigned short port) : 
   TurnSocket(address,port),
   mSocket(mIOService)
{
   mLocalBinding.setTransportType(StunTuple::TCP);

   boost::system::error_code errorCode;
   mSocket.open(address.is_v6() ? boost::asio::ip::tcp::v6() : boost::asio::ip::tcp::v4(), errorCode);
   if(!errorCode)
   {
      mSocket.set_option(boost::asio::ip::tcp::no_delay(true)); // ?slg? do we want this?
      mSocket.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
      mSocket.bind(boost::asio::ip::tcp::endpoint(mLocalBinding.getAddress(), mLocalBinding.getPort()), errorCode);
   }
}

boost::system::error_code 
TurnTcpSocket::connect(const std::string& address, unsigned short port)
{
   // Get a list of endpoints corresponding to the server name.
   boost::asio::ip::tcp::resolver resolver(mIOService);
   resip::Data service(port);
#ifdef USE_IPV6
   boost::asio::ip::tcp::resolver::query query(address, service.c_str());   
#else
   boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), address, service.c_str());   
#endif
   boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
   boost::asio::ip::tcp::resolver::iterator end;

   // Try each endpoint until we successfully establish a connection.
   boost::system::error_code errorCode = boost::asio::error::host_not_found;
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

boost::system::error_code 
TurnTcpSocket::rawWrite(const char* buffer, unsigned int size)
{
   boost::system::error_code errorCode;
   boost::asio::write(mSocket, boost::asio::buffer(buffer, size), boost::asio::transfer_all(), errorCode);
   return errorCode;
}

boost::system::error_code 
TurnTcpSocket::rawWrite(const std::vector<boost::asio::const_buffer>& buffers)
{
   boost::system::error_code errorCode;
   boost::asio::write(mSocket, buffers, boost::asio::transfer_all(), errorCode);
   return errorCode;
}

boost::system::error_code 
TurnTcpSocket::rawRead(unsigned int timeout, unsigned int* bytesRead, boost::asio::ip::address* sourceAddress, unsigned short* sourcePort)
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
   boost::asio::async_read(mSocket, boost::asio::buffer(mReadBuffer, 4),
                    boost::bind(&TurnTcpSocket::handleReadHeader, this, boost::asio::placeholders::error));
}

void 
TurnTcpSocket::readBody(unsigned int len)
{
   boost::asio::async_read(mSocket, boost::asio::buffer(&mReadBuffer[4], len),
                    boost::bind(&TurnTcpSocket::handleRawRead, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void
TurnTcpSocket::cancelSocket()
{
   boost::system::error_code ec;
   mSocket.cancel(ec);
}

void 
TurnTcpSocket::handleReadHeader(const boost::system::error_code& e)
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

      // Note:  For both StunMessages and ChannelData messages the length in bytes 3 and 4
      UInt16 dataLen;
      memcpy(&dataLen, &mReadBuffer[2], 2);
      dataLen = ntohs(dataLen);

      if((mReadBuffer[0] & 0xC0) == 0)  // If first 2 bits are 00 then this is a stun message
      {
         dataLen += 16;  // There are 20 bytes in total in the header, and we have already read 4 - read the rest of the header + the body
      }

      readBody(dataLen);
   }
   else 
   {
      mBytesRead = 0;
      mReadErrorCode = e;
      if (e != boost::asio::error::operation_aborted)
      {
         WarningLog(<< "Read header error: " << e.value() << "-" << e.message());
         mReadTimer.cancel();
      }
   }
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
