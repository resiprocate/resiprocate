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

#include "../../StunTuple.hxx"
#include "../../StunMessage.hxx"
#include "../TurnUdpSocket.hxx"
#include "../TurnAsyncTcpSocket.hxx"
#include "../TurnAsyncTlsSocket.hxx"
#include "../TurnAsyncUdpSocket.hxx"
#include "../TurnAsyncSocketHandler.hxx"
#include "../UdpEchoServer.hxx"
#include "TurnLoadGenAsyncSocketHandler.hxx"
#include <rutil/Timer.hxx>
#include <rutil/Random.hxx>
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/WinLeakCheck.hxx>

#ifdef BOOST_ASIO_HAS_STD_CHRONO
using namespace std::chrono;
#else
#include <chrono>
using namespace std::chrono;
#endif

using namespace reTurn;
using namespace std;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

void sleepMS(unsigned int ms)
{
#ifdef WIN32
   Sleep(ms);
#else
   usleep(ms*1000);
#endif
}

class TurnLoadGenConfig : public ConfigParse
{
   void printHelpText(int argc, char** argv) override
   {
      cout << "Command line format: " << endl;
      cout << "  TurnLoadGenClient [config_filename][-{setting} = {value}] ...." << endl;
      cout << "  First argument is the configuration filename - it is optional and is never proceeded with a - or /." << endl;
      cout << "  Following that config file options can be overridden." << endl;
      cout << "  Note: / can be used instead of - , and : can be used instead of = " << endl;
      cout << "Example:" << endl;
      cout << "  TurnLoadGenClient myconfig.config -LocalIPAddress=192.168.1.1" << endl;
   }
};

int main(int argc, char* argv[])
{
#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   resip::FindMemoryLeaks fml;
#endif

   // Parse command line and configuration file
   TurnLoadGenConfig config;
   Data defaultConfigFilename("turnloadgenclient.config");
   try
   {
      config.parseConfig(argc, argv, defaultConfigFilename);
   }
   catch (BaseException& ex)
   {
      std::cerr << "Error parsing configuration: " << ex << std::endl;
      return false;
   }

   Log::initialize(config, "TurnLoadTestGen");

   try
   {
      Data localAddress = config.getConfigData("LocalIPAddress", DnsUtil::getLocalIpAddress(), true);
      Data turnAddress = config.getConfigData("TurnServerIPAddress", localAddress, true);
      unsigned int turnPort = config.getConfigInt("TurnServerPort", 3478);
      Data turnProtocol = config.getConfigData("TurnServerProtocol", "UDP", true);
      unsigned int echoServerPort = config.getConfigInt("RelayPort", 2000);

      InfoLog(<< "Using: " << localAddress << " --" << turnProtocol << "--> " << turnAddress << ":" << turnPort);

      resip::Random::initialize();

      Data username = config.getConfigData("TurnServerUsername", "test", false);
      Data password = config.getConfigData("TurnServerPassword", "pwd", false);

      // Create payload to send
      int bufferSize = config.getConfigInt("PayloadSizeBytes", 172);
      if (bufferSize > 1400)
      {
         WarningLog(<< "Max PlayloadSizeBytes=" << bufferSize << " exceeds max of 1400, using 1400 instead.");
         bufferSize = 1400;
      }
      char* buffer = new char[bufferSize];  // Contents is not important - leave uninitialized
      g_Payload = new resip::Data(buffer, bufferSize);

      // Start Echo server - just need one
      UdpEchoServer udpEchoServer(localAddress, echoServerPort);
      udpEchoServer.run();

      asio::io_service ioService;

      int numClientsToSimulate = config.getConfigInt("NumClientsToSimulate", 1);
      std::list<TurnLoadGenAsyncSocketHandler*> mClients;
      for (int clientNum = 1; clientNum <= numClientsToSimulate; clientNum++)
      {
         TurnLoadGenAsyncSocketHandler* client = new TurnLoadGenAsyncSocketHandler(clientNum, ioService, localAddress, turnAddress, turnPort, udpEchoServer.getPort(), config);

         std::shared_ptr<TurnAsyncSocket> turnSocket;

#ifdef USE_SSL
         asio::ssl::context sslContext(asio::ssl::context::tlsv1);
         // Setup SSL context
         sslContext.set_verify_mode(asio::ssl::context::verify_peer);
         sslContext.load_verify_file(config.getConfigData("TLSRootCertFile", "ca.pem", true).c_str());

         if (isEqualNoCase(turnProtocol, "TLS"))
         {
            turnSocket = std::make_shared<TurnAsyncTlsSocket>(ioService, sslContext, false, client, asio::ip::address::from_string(localAddress.c_str()), 0);
         }
         else
#endif
         if (isEqualNoCase(turnProtocol, "TCP"))
         {
           turnSocket = std::make_shared<TurnAsyncTcpSocket>(ioService, client, asio::ip::address::from_string(localAddress.c_str()), 0);
         }
         else
         {
           turnSocket = std::make_shared<TurnAsyncUdpSocket>(ioService, client, asio::ip::address::from_string(localAddress.c_str()), 0);
         }

         // Set the username and password
         if (username.size() > 0 && password.size() > 0)
         {
            turnSocket->setUsernameAndPassword(username.c_str(), password.c_str());
         }

         client->setTurnAsyncSocket(turnSocket);

         mClients.push_back(client);
      }

      ioService.run();

      udpEchoServer.shutdown();
      udpEchoServer.join();
   }
   catch (const std::exception& e)
   {
      std::cerr << "Exception: " << e.what() << "\n";
   }

   return 0;
}


/* ====================================================================

 Copyright (c) 2023-2024, SIP Spectrum, Inc. http://sipspectrum.com
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
