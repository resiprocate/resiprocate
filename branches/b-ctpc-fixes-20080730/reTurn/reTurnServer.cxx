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


class reTurnConfig
{
public:
   // TODO
};

int main(int argc, char* argv[])
{
#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
  resip::FindMemoryLeaks fml;
#endif

  try
  {
    // Check command line arguments.
    if (argc != 5)
    {
      std::cerr << "Usage: reTurnServer <address> <turnPort> <altAddress> <altPort>\n";
      std::cerr << "  For IPv4, try:\n";
      std::cerr << "    reTurnServer 0.0.0.0 8777 0.0.0.0 3589\n";
      std::cerr << "  For IPv6, try:\n";
      std::cerr << "    reTurnServer 0::0 8777 0::0 3589\n";
      return 1;
    }

    // Initialize Logging - TODO make configurable
    resip::Log::initialize(resip::Log::Cout, resip::Log::Info, argv[0]);
    //resip::Log::initialize(resip::Log::Cout, resip::Log::Stack, argv[0]);

    // Initialize server.
    asio::io_service ioService;                       // The one and only ioService for the stunServer
    reTurn::TurnManager turnManager(ioService);         // The one and only Turn Manager

    unsigned short turnPort = (unsigned short)resip::Data(argv[2]).convertUnsignedLong();
    unsigned short altStunPort = (unsigned short)resip::Data(argv[4]).convertUnsignedLong();
    unsigned short tlsTurnPort = turnPort + 1;
    asio::ip::address turnAddress = asio::ip::address::from_string(argv[1]);
    asio::ip::address altStunAddress = asio::ip::address::from_string(argv[3]);

    boost::shared_ptr<reTurn::UdpServer> udpTurnServer;  // also a1p1StunUdpServer
    boost::shared_ptr<reTurn::TcpServer> tcpTurnServer;
    boost::shared_ptr<reTurn::TlsServer> tlsTurnServer;
    boost::shared_ptr<reTurn::UdpServer> a1p2StunUdpServer;
    boost::shared_ptr<reTurn::UdpServer> a2p1StunUdpServer;
    boost::shared_ptr<reTurn::UdpServer> a2p2StunUdpServer;

    // The one and only RequestHandler - if altStunPort is non-zero, then assume RFC3489 support is enabled and pass settings to request handler
    reTurn::RequestHandler requestHandler(turnManager, 
                                          altStunPort != 0 ? &turnAddress : 0, 
                                          altStunPort != 0 ? &turnPort : 0, 
                                          altStunPort != 0 ? &altStunAddress : 0, 
                                          altStunPort != 0 ? &altStunPort : 0); 

    udpTurnServer.reset(new reTurn::UdpServer(ioService, requestHandler, turnAddress, turnPort));
    tcpTurnServer.reset(new reTurn::TcpServer(ioService, requestHandler, turnAddress, turnPort));
    tlsTurnServer.reset(new reTurn::TlsServer(ioService, requestHandler, turnAddress, tlsTurnPort));

    if(altStunPort != 0) // if alt stun port is non-zero, then RFC3489 support is enabled
    {
       a1p2StunUdpServer.reset(new reTurn::UdpServer(ioService, requestHandler, turnAddress, altStunPort));
       a2p1StunUdpServer.reset(new reTurn::UdpServer(ioService, requestHandler, altStunAddress, turnPort));
       a2p2StunUdpServer.reset(new reTurn::UdpServer(ioService, requestHandler, altStunAddress, altStunPort));
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

