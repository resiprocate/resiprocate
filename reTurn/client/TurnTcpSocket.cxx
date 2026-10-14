#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "TurnTcpSocket.hxx"
#include "../AsyncSocketBase.hxx"
#include <rutil/Logger.hxx>
#include "../ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;

namespace reTurn {

TurnTcpSocket::TurnTcpSocket(const asio::ip::address& address, unsigned short port) : 
   TurnSocket(address,port),
   mSocket(mIOService)
{
   mLocalBinding.setTransportType(StunTuple::TCP);
   // connect() opens the socket and binds it to address and port
}

asio::ip::tcp
TurnTcpSocket::protocol() const
{
   return mLocalBinding.getAddress().is_v6() ? asio::ip::tcp::v6() : asio::ip::tcp::v4();
}

asio::error_code
TurnTcpSocket::openAndBind(asio::ip::tcp::socket& socket)
{
   asio::error_code errorCode;
   socket.open(protocol(), errorCode);
   if(!errorCode)
   {
      asio::error_code ignored;
      socket.set_option(asio::ip::tcp::no_delay(true), ignored); // ?slg? do we want this?
      socket.set_option(asio::ip::tcp::socket::reuse_address(true), ignored);
      socket.bind(asio::ip::tcp::endpoint(mLocalBinding.getAddress(), mLocalBinding.getPort()), errorCode);
   }
   return errorCode;
}

asio::error_code
TurnTcpSocket::waitForConnectStep(asio::error_code& result, const std::function<void()>& cancel)
{
   bool timedOut = false;
   asio::steady_timer timer(mIOService);
   timer.expires_after(std::chrono::milliseconds(mConnectTimeout));
   timer.async_wait([&timedOut, &cancel](const asio::error_code& e)
   {
      if(!e)
      {
         timedOut = true;
         cancel();  // the operation then completes, with operation_aborted
      }
   });

   mIOService.restart();
   while(result == asio::error::would_block && mIOService.run_one() > 0)
   {
   }
   timer.cancel();
   mIOService.run();  // lets the timer's handler finish
   mIOService.restart();
   return timedOut ? asio::error_code(asio::error::timed_out) : result;
}

asio::error_code
TurnTcpSocket::connect(const std::string& address, unsigned short port)
{
   // Get a list of endpoints corresponding to the server name.
   asio::ip::tcp::resolver resolver(mIOService);
   resip::Data service(port);
   asio::error_code errorCode;
   asio::ip::tcp::resolver::results_type results = resolver.resolve(protocol(), address, service.c_str(), errorCode);
   if(errorCode)
   {
      return errorCode;
   }
   asio::ip::tcp::resolver::results_type::const_iterator endpoint_iterator = results.begin();
   asio::ip::tcp::resolver::results_type::const_iterator end = results.end();

   // Try each endpoint until we successfully establish a connection, each on a new socket bound
   // to the local binding
   errorCode = asio::error::host_not_found;
   while (errorCode && endpoint_iterator != end)
   {
      mSocket.close();
      errorCode = openAndBind(mSocket);
      if(!errorCode)
      {
         asio::error_code result = asio::error::would_block;
         mSocket.async_connect(*endpoint_iterator, [&result](const asio::error_code& ec) { result = ec; });
         errorCode = waitForConnectStep(result, [this]() { asio::error_code ignored; mSocket.close(ignored); });
      }

      if(!errorCode)
      {
         mConnected = true;
         mConnectedTuple.setTransportType(StunTuple::TCP);
         mConnectedTuple.setAddress(endpoint_iterator->endpoint().address());
         mConnectedTuple.setPort(endpoint_iterator->endpoint().port());
      }
      ++endpoint_iterator;
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
   mIOService.restart();

   *bytesRead = (unsigned int)(mBytesRead + 4 - (mBytesRead >= mPadding ? mPadding : 0));
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
                    std::bind(&TurnTcpSocket::handleReadHeader, this, std::placeholders::_1));
}

void 
TurnTcpSocket::readBody(unsigned int len)
{
   asio::async_read(mSocket, asio::buffer(&mReadBuffer[4], len),
                    std::bind(&TurnTcpSocket::handleRawRead, this, std::placeholders::_1, std::placeholders::_2));
}

void
TurnTcpSocket::cancelSocket()
{
   asio::error_code ec;
   mSocket.cancel(ec);
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

      // Note:  For both StunMessages and ChannelData messages the length in bytes 3 and 4
      uint16_t dataLen;
      memcpy(&dataLen, &mReadBuffer[2], 2);
      dataLen = ntohs(dataLen);

      // ChannelData's padding is read with it, but not counted in what rawRead reports
      mPadding = 0;
      if((mReadBuffer[0] & 0xC0) == 0)  // If first 2 bits are 00 then this is a stun message
      {
         dataLen += 16;  // There are 20 bytes in total in the header, and we have already read 4 - read the rest of the header + the body
      }
      else
      {
         mPadding = AsyncSocketBase::channelDataPadding(dataLen);
      }

      readBody(dataLen + (unsigned int)mPadding);
   }
   else 
   {
      mBytesRead = 0;
      mReadErrorCode = e;
      if (e != asio::error::operation_aborted)
      {
         WarningLog(<< "Read header error: " << e.value() << "-" << e.message());
         mReadTimer.cancel();
      }
   }
}

} // namespace


/* ====================================================================

 Copyright (c) 2008-2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
