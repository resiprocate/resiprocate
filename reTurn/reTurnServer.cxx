#include <iostream>
#include <string>
#include <asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <rutil/Data.hxx>
#include "TcpServer.hxx"
#include "TlsServer.hxx"
#include "UdpServer.hxx"
#include "ReTurnConfig.hxx"
#include "RequestHandler.hxx"
#include "TurnManager.hxx"
#include <rutil/WinLeakCheck.hxx>
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

#if defined(_WIN32)

boost::function0<void> console_ctrl_function;

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
  switch (ctrl_type)
  {
  case CTRL_C_EVENT:
  case CTRL_BREAK_EVENT:
  case CTRL_CLOSE_EVENT:
  case CTRL_SHUTDOWN_EVENT:
    console_ctrl_function();
    return TRUE;
  default:
    return FALSE;
  }
}
#endif // defined(_WIN32)

int main(int argc, char* argv[])
{
#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   resip::FindMemoryLeaks fml;
#endif

   reTurn::ReTurnConfig reTurnConfig;

   try
   {
      // Check command line arguments.
      if (argc != 1 && argc != 6)
      {
         std::cerr << "Usage: reTurnServer <address> <turnPort> <tlsPort> <altAddress> \n";
         std::cerr << "                    <altPort>\n";
         std::cerr << "  IPv4 Example (with RFC3489 support):\n";
         std::cerr << "    reTurnServer 192.168.1.10 3478 5349 192.168.1.11 3479\n\n";
         std::cerr << "  IPv6 Example (with RFC3489 support):\n";
         std::cerr << "    reTurnServer 3ffe:0501:0008:0000:0260:97ff:fe40:efab 3478\n";
         std::cerr << "                 5349 3ffe:0501:0008:0000:0260:97ff:fe40:efac 3479\n\n";
         std::cerr << "  Note:  For RFC3489 legacy support define altPort as non-zero and\n";
         std::cerr << "         ensure you don't use INADDR_ANY for the IP addresses.\n";
         std::cerr << "         Both addresses should terminate on the machine running\n";
         std::cerr << "         reTurn.";
         return 1;
      }

      // Initialize Logging
      resip::Log::initialize(reTurnConfig.mLoggingType, reTurnConfig.mLoggingLevel, "reTurnServer", reTurnConfig.mLoggingFilename.c_str());
      resip::GenericLogImpl::MaxLineCount = reTurnConfig.mLoggingFileMaxLineCount;

      // Initialize server.
      asio::io_service ioService;                                // The one and only ioService for the stunServer
      reTurn::TurnManager turnManager(ioService, reTurnConfig);  // The one and only Turn Manager

      if(argc == 6)
      {
         reTurnConfig.mTurnPort = (unsigned short)resip::Data(argv[2]).convertUnsignedLong();
         reTurnConfig.mTlsTurnPort = (unsigned short)resip::Data(argv[3]).convertUnsignedLong();
         reTurnConfig.mAltStunPort = (unsigned short)resip::Data(argv[5]).convertUnsignedLong();
         reTurnConfig.mTurnAddress = asio::ip::address::from_string(argv[1]);
         reTurnConfig.mAltStunAddress = asio::ip::address::from_string(argv[4]);
      }

      boost::shared_ptr<reTurn::UdpServer> udpTurnServer;  // also a1p1StunUdpServer
      boost::shared_ptr<reTurn::TcpServer> tcpTurnServer;
      boost::shared_ptr<reTurn::TlsServer> tlsTurnServer;
      boost::shared_ptr<reTurn::UdpServer> a1p2StunUdpServer;
      boost::shared_ptr<reTurn::UdpServer> a2p1StunUdpServer;
      boost::shared_ptr<reTurn::UdpServer> a2p2StunUdpServer;

      // The one and only RequestHandler - if altStunPort is non-zero, then assume RFC3489 support is enabled and pass settings to request handler
      reTurn::RequestHandler requestHandler(turnManager, 
         reTurnConfig.mAltStunPort != 0 ? &reTurnConfig.mTurnAddress : 0, 
         reTurnConfig.mAltStunPort != 0 ? &reTurnConfig.mTurnPort : 0, 
         reTurnConfig.mAltStunPort != 0 ? &reTurnConfig.mAltStunAddress : 0, 
         reTurnConfig.mAltStunPort != 0 ? &reTurnConfig.mAltStunPort : 0); 

      udpTurnServer.reset(new reTurn::UdpServer(ioService, requestHandler, reTurnConfig.mTurnAddress, reTurnConfig.mTurnPort));
      tcpTurnServer.reset(new reTurn::TcpServer(ioService, requestHandler, reTurnConfig.mTurnAddress, reTurnConfig.mTurnPort));
      tlsTurnServer.reset(new reTurn::TlsServer(ioService, requestHandler, reTurnConfig.mTurnAddress, reTurnConfig.mTlsTurnPort));

      if(reTurnConfig.mAltStunPort != 0) // if alt stun port is non-zero, then RFC3489 support is enabled
      {
         a1p2StunUdpServer.reset(new reTurn::UdpServer(ioService, requestHandler, reTurnConfig.mTurnAddress, reTurnConfig.mAltStunPort));
         a2p1StunUdpServer.reset(new reTurn::UdpServer(ioService, requestHandler, reTurnConfig.mAltStunAddress, reTurnConfig.mTurnPort));
         a2p2StunUdpServer.reset(new reTurn::UdpServer(ioService, requestHandler, reTurnConfig.mAltStunAddress, reTurnConfig.mAltStunPort));
         udpTurnServer->setAlternateUdpServers(a1p2StunUdpServer.get(), a2p1StunUdpServer.get(), a2p2StunUdpServer.get());
         a1p2StunUdpServer->setAlternateUdpServers(udpTurnServer.get(), a2p2StunUdpServer.get(), a2p1StunUdpServer.get());
         a2p1StunUdpServer->setAlternateUdpServers(a2p2StunUdpServer.get(), udpTurnServer.get(), a1p2StunUdpServer.get());
         a2p2StunUdpServer->setAlternateUdpServers(a2p1StunUdpServer.get(), a1p2StunUdpServer.get(), udpTurnServer.get());
         a1p2StunUdpServer->start();
         a2p1StunUdpServer->start();
         a2p2StunUdpServer->start();
      }

      udpTurnServer->start();
      tcpTurnServer->start();
      tlsTurnServer->start();

#ifdef _WIN32
      // Set console control handler to allow server to be stopped.
      console_ctrl_function = boost::bind(&asio::io_service::stop, &ioService);
      SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#else
      // Block all signals for background thread.
      sigset_t new_mask;
      sigfillset(&new_mask);
      sigset_t old_mask;
      pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);
#endif

      // Run the ioService until stopped.
      // Create a pool of threads to run all of the io_services.
      boost::shared_ptr<asio::thread> thread(new asio::thread(
         boost::bind(&asio::io_service::run, &ioService)));

#ifndef _WIN32
      // Restore previous signals.
      pthread_sigmask(SIG_SETMASK, &old_mask, 0);

      // Wait for signal indicating time to shut down.
      sigset_t wait_mask;
      sigemptyset(&wait_mask);
      sigaddset(&wait_mask, SIGINT);
      sigaddset(&wait_mask, SIGQUIT);
      sigaddset(&wait_mask, SIGTERM);
      pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
      int sig = 0;
      sigwait(&wait_mask, &sig);
      ioService.stop();
#endif

      // Wait for thread to exit
      thread->join();
   }
   catch (std::exception& e)
   {
      ErrLog(<< "exception: " << e.what());
   }

   return 0;
}


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
