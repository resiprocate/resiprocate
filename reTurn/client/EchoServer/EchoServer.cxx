#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifdef WIN32
#pragma warning(disable : 4267)
#endif

#include <iostream>
#include <string>
#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif
#include <rutil/ThreadIf.hxx>

#include "../TurnUdpSocket.hxx"
#include <rutil/Time.hxx>
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace reTurn;
using namespace std;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

// Simple Echo Server that can be used in TURN testing
class EchoServer : public resip::ThreadIf
{
public:
   EchoServer(const Data& localAddress, unsigned int port)
      : mPort(port),
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
      read();
   }

   void thread() override
   {
      mTimeOfFirstRead = chrono::steady_clock::now();
      mIOContext.run();
   }

   void shutdown() override
   {
      mIOContext.stop();
   }

private:
   void read() 
   {
      mSocket.async_receive_from(asio::buffer(mReceiveBuffer, sizeof(mReceiveBuffer)), mRemoteEndpoint,
         [this](asio::error_code ec, std::size_t length) 
         {
            if (ec)
            {
               if (ec.value() == 10054 ||  // Ignore 10054 errors
                   ec.value() == 10061)    // Ignore 10061 errors
               {
                  InfoLog(<< "EchoServer[" << mPort << "]: Received 10054 read error, ignoring!");
                  read();
                  return;
               }

               ErrLog(<< "EchoServer[" << mPort << "]: error calling async_receive_from: error=" << ec.value() << ", message=" << ec.message());
               return;
            }

            mIntervalReadCount++;
            checkLog();

            mReceiveSize = length;
            DebugLog(<< "EchoServer[" << mPort << "]: Received data from: " << mRemoteEndpoint << ", size=" << length);
            if (!isShutdown())
            {
               write();
            }
         }
      );
   }

   void write() 
   {
      mSocket.async_send_to(asio::buffer(mReceiveBuffer, mReceiveSize),
         mRemoteEndpoint,
         [this](asio::error_code ec, std::size_t length) 
         {
            if (ec)
            {
               ErrLog(<< "EchoServer[" << mPort << "]: error calling async_send_to: error=" << ec.value() << ", message=" << ec.message());
               return;
            }
            if (!isShutdown())
            {
               read();
            }
         }
      );
   }

   void checkLog()
   {
      std::chrono::time_point<chrono::steady_clock> now = chrono::steady_clock::now();
      long long secondsPassed = chrono::duration_cast<chrono::seconds>(now - mTimeOfFirstRead).count();
      if (secondsPassed > 30)
      {
         InfoLog(<< "EchoServer[" << mPort << "]: Echo'd " << mIntervalReadCount << " packets in the last " << secondsPassed << " seconds.");
         mIntervalReadCount = 0;
         mTimeOfFirstRead = now;
      }
   }

   unsigned int mPort;
   asio::io_context mIOContext;
   asio::ip::udp::socket mSocket;
   asio::ip::udp::endpoint mRemoteEndpoint;
   char mReceiveBuffer[1500];
   int mReceiveSize;

   unsigned int mIntervalReadCount;
   std::chrono::time_point<chrono::steady_clock> mTimeOfFirstRead;
};

class EchoServerConfig : public ConfigParse
{
   void printHelpText(int argc, char** argv) override
   {
      cout << "Command line format: " << endl;
      cout << "  EchoServer [config_filename] [-{setting} = {value}] ...." << endl;
      cout << "  First argument is the configuration filename - it is optional and is never proceeded with a - or /." << endl;
      cout << "  Following that, config file options can be overridden." << endl;
      cout << "  Note: / can be used instead of - , and : can be used instead of = " << endl;
      cout << "Example:" << endl;
      cout << "  EchoServer myconfig.config -LocalIPAddress=192.168.1.1" << endl;
   }
};

int main(int argc, char* argv[])
{
#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   resip::FindMemoryLeaks fml;
#endif

   // Parse command line and configuration file
   EchoServerConfig config;
   Data defaultConfigFilename("echoserver.config");
   try
   {
      config.parseConfig(argc, argv, defaultConfigFilename);
   }
   catch (BaseException& ex)
   {
      std::cerr << "Error parsing configuration: " << ex << std::endl;
      return false;
   }

   Log::initialize(config, "EchoServer");

   try
   {
      Data localAddress = config.getConfigData("LocalIPAddress", DnsUtil::getLocalIpAddress(), true);
      unsigned int startingPort = config.getConfigInt("StartingPort", 8000);
      unsigned int portIncrement = config.getConfigInt("PortIncrement", 2);
      unsigned int numPorts = config.getConfigInt("NumPorts", 4);

      InfoLog(<< "Using: LocalAddress=" << localAddress << ", StartingPort=" << startingPort << ", PortIncrement=" << portIncrement << ", NumPorts=" << numPorts);

      // Start Echo servers
      std::list<EchoServer*> echoServers;
      for (unsigned int i = 0; i < numPorts; i++)
      {

         unsigned int port = startingPort + (portIncrement * i);
         EchoServer* echoServer = new EchoServer(localAddress, port);
         echoServer->run();
         echoServers.push_back(echoServer);
         InfoLog(<< "EchoServer started " << localAddress << ":" << port);
      }

      cout << "Press enter to exit!\n";

      // Wait for key press
      getchar();

      // Stop Echo servers
      for (auto echoServer : echoServers)
      {
         echoServer->shutdown();
      }

      // Wait for echo servers to stop
      for (auto echoServer : echoServers)
      {
         echoServer->join();
         delete echoServer;
      }

      // Clear list
      echoServers.clear();

      InfoLog(<< "EchoServer shutdown.");
   }
   catch (const std::exception& e)
   {
      std::cerr << "Exception: " << e.what() << "\n";
   }

   return 0;
}


/* ====================================================================

 Copyright (c) 2023, SIP Spectrum, Inc. http://sipspectrum.com
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
