#include <iostream>
#include <csignal>
#include <string>
#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <rutil/Data.hxx>
#include "reTurnServer.hxx"
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
   reTurn::ReTurnServerProcess proc;
   return proc.main(argc, argv);
}

reTurn::ReTurnServerProcess::ReTurnServerProcess()
{
}

reTurn::ReTurnServerProcess::~ReTurnServerProcess()
{
}

int
reTurn::ReTurnServerProcess::main(int argc, char* argv[])
{
#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   resip::FindMemoryLeaks fml;
#endif

   resip::Data defaultConfig("reTurnServer.config");
   reTurn::ReTurnConfig reTurnConfig;
   try
   {
      reTurnConfig.parseConfig(argc, argv, defaultConfig);
   }
   catch(std::exception& e)
   {
      ErrLog(<< "Exception parsing configuration: " << e.what());
      exit(-1);
   }

   setPidFile(reTurnConfig.mPidFile);
   if(isAlreadyRunning())
   {
      std::cerr << "Already running, will not start two instances.  Please stop existing process and/or delete PID file.";
#ifndef WIN32
      syslog(LOG_DAEMON | LOG_CRIT, "Already running, will not start two instances.  Please stop existing process and/or delete PID file.");
#endif
      return false;
   }

   // Daemonize if necessary
   if(reTurnConfig.mDaemonize)
   {
      daemonize();
   }

   try
   {
      // Initialize Logging
      resip::Log::initialize(reTurnConfig.mLoggingType, reTurnConfig.mLoggingLevel, "reTurnServer", reTurnConfig.mLoggingFilename.c_str(), 0, reTurnConfig.mSyslogFacility);
      resip::GenericLogImpl::MaxLineCount = reTurnConfig.mLoggingFileMaxLineCount;

      // Initialize server.
      asio::io_service ioService;                                // The one and only ioService for the stunServer
      reTurn::TurnManager turnManager(ioService, reTurnConfig);  // The one and only Turn Manager

      boost::shared_ptr<reTurn::UdpServer> udpTurnServer;  // also a1p1StunUdpServer
      boost::shared_ptr<reTurn::TcpServer> tcpTurnServer;
      boost::shared_ptr<reTurn::TlsServer> tlsTurnServer;
      boost::shared_ptr<reTurn::UdpServer> a1p2StunUdpServer;
      boost::shared_ptr<reTurn::UdpServer> a2p1StunUdpServer;
      boost::shared_ptr<reTurn::UdpServer> a2p2StunUdpServer;

#ifdef USE_IPV6
      boost::shared_ptr<reTurn::UdpServer> udpV6TurnServer;
      boost::shared_ptr<reTurn::TcpServer> tcpV6TurnServer;
      boost::shared_ptr<reTurn::TlsServer> tlsV6TurnServer;
#endif

      // The one and only RequestHandler - if altStunPort is non-zero, then assume RFC3489 support is enabled and pass settings to request handler
      reTurn::RequestHandler requestHandler(turnManager, 
         reTurnConfig.mAltStunPort != 0 ? &reTurnConfig.mTurnAddress : 0, 
         reTurnConfig.mAltStunPort != 0 ? &reTurnConfig.mTurnPort : 0, 
         reTurnConfig.mAltStunPort != 0 ? &reTurnConfig.mAltStunAddress : 0, 
         reTurnConfig.mAltStunPort != 0 ? &reTurnConfig.mAltStunPort : 0); 

      udpTurnServer.reset(new reTurn::UdpServer(ioService, requestHandler, reTurnConfig.mTurnAddress, reTurnConfig.mTurnPort));
      tcpTurnServer.reset(new reTurn::TcpServer(ioService, requestHandler, reTurnConfig.mTurnAddress, reTurnConfig.mTurnPort));
      if(reTurnConfig.mTlsTurnPort != 0)
      {
         tlsTurnServer.reset(new reTurn::TlsServer(ioService, requestHandler, reTurnConfig.mTurnAddress, reTurnConfig.mTlsTurnPort));
      }

#ifdef USE_IPV6
      udpV6TurnServer.reset(new reTurn::UdpServer(ioService, requestHandler, reTurnConfig.mTurnV6Address, reTurnConfig.mTurnPort));
      tcpV6TurnServer.reset(new reTurn::TcpServer(ioService, requestHandler, reTurnConfig.mTurnV6Address, reTurnConfig.mTurnPort));
      if(reTurnConfig.mTlsTurnPort != 0)
      {
         tlsV6TurnServer.reset(new reTurn::TlsServer(ioService, requestHandler, reTurnConfig.mTurnV6Address, reTurnConfig.mTlsTurnPort));
      }
#endif

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
      if(tlsTurnServer)
      {
         tlsTurnServer->start();
      }

#ifdef USE_IPV6
      udpV6TurnServer->start();
      tcpV6TurnServer->start();
      if(tlsV6TurnServer)
      {
         tlsV6TurnServer->start();
      }
#endif

      // Drop privileges (can do this now that sockets are bound)
      if(!reTurnConfig.mRunAsUser.empty())
      {
         InfoLog( << "Trying to drop privileges, configured uid = " << reTurnConfig.mRunAsUser << " gid = " << reTurnConfig.mRunAsGroup);
         dropPrivileges(reTurnConfig.mRunAsUser, reTurnConfig.mRunAsGroup);
      }

      ReTurnUserFileScanner userFileScanner(ioService, reTurnConfig);
      userFileScanner.start();

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
