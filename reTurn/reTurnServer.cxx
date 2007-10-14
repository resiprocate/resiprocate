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

class reTurnConfig
{
public:
   // TODO
};

#define NUM_THREADS 1  // Do not change this - code is not currently thread safe

int main(int argc, char* argv[])
{
  try
  {
    // Check command line arguments.
    if (argc != 5)
    {
      std::cerr << "Usage: reTurnServer <primaryAddress> <primaryPort> <altAddress> <altPort>\n";
      std::cerr << "  For IPv4, try:\n";
      std::cerr << "    reTurnServer 0.0.0.0 8776 0.0.0.0 8777\n";
      std::cerr << "  For IPv6, try:\n";
      std::cerr << "    reTurnServer 0::0 8776 0::0 8777\n";
      return 1;
    }

    // Initialize server.
    asio::io_service ioService;                       // The one and only ioService for the stunServer
    reTurn::TurnManager turnManager(ioService);         // The one and only Turn Manager
    reTurn::RequestHandler requestHandler(turnManager); // The one and only RequestHandler

    resip::Data primport(argv[2]);
    resip::Data tlsPort(primport.convertUnsignedLong() + 1);
    reTurn::TcpServer tcpServer(ioService, requestHandler, argv[1], argv[2]);
    reTurn::TlsServer tlsServer(ioService, requestHandler, argv[1], tlsPort.c_str());
    reTurn::UdpServer a1p1UdpServer(ioService, requestHandler, argv[1], argv[2]);
    reTurn::UdpServer a1p2UdpServer(ioService, requestHandler, argv[1], argv[4]);
    reTurn::UdpServer a2p1UdpServer(ioService, requestHandler, argv[3], argv[2]);
    reTurn::UdpServer a2p2UdpServer(ioService, requestHandler, argv[3], argv[4]);
    a1p1UdpServer.setAlternateUdpServers(&a1p2UdpServer, &a2p1UdpServer, &a2p2UdpServer);
    a1p2UdpServer.setAlternateUdpServers(&a1p1UdpServer, &a2p2UdpServer, &a2p1UdpServer);
    a2p1UdpServer.setAlternateUdpServers(&a2p2UdpServer, &a1p1UdpServer, &a1p2UdpServer);
    a2p2UdpServer.setAlternateUdpServers(&a2p1UdpServer, &a1p2UdpServer, &a1p1UdpServer);

    // Set console control handler to allow server to be stopped.
    console_ctrl_function = boost::bind(&asio::io_service::stop, &ioService);
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

    // Run the ioService until stopped.
    // Create a pool of threads to run all of the io_services.
    std::vector<boost::shared_ptr<asio::thread> > threads;
    for (std::size_t i = 0; i < NUM_THREADS; ++i)
    {
       boost::shared_ptr<asio::thread> thread(new asio::thread(
          boost::bind(&asio::io_service::run, &ioService)));
       threads.push_back(thread);
    }

    // Wait for all threads in the pool to exit.
    for (std::size_t i = 0; i < threads.size(); ++i)
    {
       threads[i]->join();
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}

#endif // defined(_WIN32)


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

