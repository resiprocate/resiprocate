#ifndef UDPECHOSERVER_HXX
#define UDPECHOSERVER_HXX

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif
#include <rutil/ThreadIf.hxx>

using namespace std;
using namespace resip;

namespace reTurn {

   class UdpEchoServer : public resip::ThreadIf
   {
   public:
      UdpEchoServer(const Data& localAddress, unsigned int port);
      virtual ~UdpEchoServer() {}

      unsigned int getPort() { return mPort; }  // If port is passed in as 0 to constructor, this can be used to query the ephemeral port assigned

      void thread() override;
      void shutdown() override;

   private:
      void read();
      void write();
      void checkLog();

      unsigned int mPort;
      asio::io_context mIOContext;
      asio::ip::udp::socket mSocket;
      asio::ip::udp::endpoint mRemoteEndpoint;
      char mReceiveBuffer[1500];
      std::size_t mReceiveSize;

      unsigned int mIntervalReadCount;
      std::chrono::time_point<chrono::steady_clock> mTimeOfFirstRead;
   };
}

#endif


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
