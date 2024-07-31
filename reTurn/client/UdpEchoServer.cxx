#include <iostream>
#include <string>
#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif

#include "UdpEchoServer.hxx"
#include "../ReTurnSubsystem.hxx"
#include <rutil/Logger.hxx>
#include <rutil/WinLeakCheck.hxx>

#ifdef BOOST_ASIO_HAS_STD_CHRONO
using namespace std::chrono;
#else
#include <chrono>
using namespace std::chrono;
#endif

using namespace std;
using namespace resip;
using namespace reTurn;

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

// Simple UDP Echo Server
UdpEchoServer::UdpEchoServer(const Data& localAddress, unsigned int port) :
   mPort(port),
   mSocket(mIOContext),
   mIntervalReadCount(0)
{
   asio::ip::address address = asio::ip::address::from_string(localAddress.c_str());
   asio::error_code errorCode;
   mSocket.open(address.is_v6() ? asio::ip::udp::v6() : asio::ip::udp::v4(), errorCode);
   if (!errorCode)
   {
      mSocket.set_option(asio::ip::udp::socket::reuse_address(true));
      mSocket.bind(asio::ip::udp::endpoint(address, port));
   }
   mIntervalReadCount = 0;

   if (mPort == 0)
   {
      mPort = mSocket.local_endpoint().port();
   }
   InfoLog(<< "UdpEchoServer: Started, localAddress=" << localAddress << ", localPort=" << mPort);
   
   read();
}

void UdpEchoServer::thread()
{
   mTimeOfFirstRead = chrono::steady_clock::now();
   mIOContext.run();
   InfoLog(<< "UdpEchoServer: thread completed");
}

void UdpEchoServer::shutdown()
{
   mIOContext.stop();
}

void UdpEchoServer::read()
{
   mSocket.async_receive_from(asio::buffer(mReceiveBuffer, sizeof(mReceiveBuffer)), mRemoteEndpoint,
      [this](asio::error_code ec, std::size_t length)
      {
         if (ec)
         {
            if (ec.value() == 10054 ||  // Ignore 10054 errors
               ec.value() == 10061)    // Ignore 10061 errors
            {
               InfoLog(<< "UdpEchoServer[" << mPort << "]: Received 10054 read error, ignoring!");
               read();
               return;
            }

            ErrLog(<< "UdpEchoServer[" << mPort << "]: error calling async_receive_from: error=" << ec.value() << ", message=" << ec.message());
            return;
         }

         mIntervalReadCount++;
         checkLog();

         mReceiveSize = length;
         DebugLog(<< "UdpEchoServer[" << mPort << "]: Received data from: " << mRemoteEndpoint << ", size=" << length);
         if (!isShutdown())
         {
            write();
         }
      }
   );
}

void UdpEchoServer::write()
{
   mSocket.async_send_to(asio::buffer(mReceiveBuffer, mReceiveSize),
      mRemoteEndpoint,
      [this](asio::error_code ec, std::size_t length)
      {
         if (ec)
         {
            ErrLog(<< "UdpEchoServer[" << mPort << "]: error calling async_send_to: error=" << ec.value() << ", message=" << ec.message());
            return;
         }
         if (!isShutdown())
         {
            read();
         }
      }
   );
}

void UdpEchoServer::checkLog()
{
   std::chrono::time_point<chrono::steady_clock> now = chrono::steady_clock::now();
   long long secondsPassed = chrono::duration_cast<chrono::seconds>(now - mTimeOfFirstRead).count();
   if (secondsPassed > 30)
   {
      InfoLog(<< "UdpEchoServer[" << mPort << "]: Echo'd " << mIntervalReadCount << " packets in the last " << secondsPassed << " seconds.");
      mIntervalReadCount = 0;
      mTimeOfFirstRead = now;
   }
}


/* ====================================================================

 Copyright (c) 2024 SIP Spectrum, Inc http://www.sipspectrum.com
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
