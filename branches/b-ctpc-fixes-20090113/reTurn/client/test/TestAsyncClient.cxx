#ifdef WIN32
#pragma warning(disable : 4267)
#endif

#include <iostream>
#include <string>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <rutil/ThreadIf.hxx>

#include "../../StunTuple.hxx"
#include "../../StunMessage.hxx"
#include "../TurnUdpSocket.hxx"
#include "../TurnAsyncTcpSocket.hxx"
#include "../TurnAsyncTlsSocket.hxx"
#include "../TurnAsyncUdpSocket.hxx"
#include "../TurnAsyncSocketHandler.hxx"
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace reTurn;
using namespace std;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

resip::Data address = resip::DnsUtil::getLocalIpAddress();

// Simple UDP Echo Server
class TurnPeer : public resip::ThreadIf
{
public:
   TurnPeer() {}

   virtual ~TurnPeer() {}

   virtual void thread()
   {
      asio::error_code rc;
      TurnUdpSocket turnSocket(asio::ip::address::from_string(address.c_str()), 2000);

      char buffer[1024];
      unsigned int size = sizeof(buffer);
      asio::ip::address sourceAddress;
      unsigned short sourcePort;
      bool connected = false;

      // Receive Data
      rc=turnSocket.receive(buffer, size, 1000, &sourceAddress, &sourcePort);
      while((!rc || rc.value() == asio::error::operation_aborted) && !isShutdown())
      {
         if(!rc)
         {
            if(!connected)
            {
               turnSocket.connect(sourceAddress.to_string(), sourcePort);
               connected = true;
            }
            InfoLog(<< "PEER: Received data from " << sourceAddress << ":" << sourcePort << " - [" << resip::Data(buffer, size).c_str() << "]");
            turnSocket.send(buffer, size);
         }
         size = sizeof(buffer);
         rc=turnSocket.receive(buffer, size, 1000, &sourceAddress, &sourcePort);
      }

      if(rc)
      {
         if(rc.value() != asio::error::operation_aborted)
         {
            ErrLog(<< "PEER: Receive error: " << rc.message());
         }
      }
   }
private:
};

class MyTurnAsyncSocketHandler : public TurnAsyncSocketHandler
{
public:
   MyTurnAsyncSocketHandler() : mNumReceives(0) {}
   virtual ~MyTurnAsyncSocketHandler() {}

   virtual void onConnectSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onConnectSuccess: socketDest=" << socketDesc << ", address=" << address << ", port=" << port);
      mTurnAsyncSocket->bindRequest();
   }
   virtual void onConnectFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onConnectFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   }

   virtual void onSharedSecretSuccess(unsigned int socketDesc, const char* username, unsigned int usernameSize, const char* password, unsigned int passwordSize)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onSharedSecretSuccess: socketDest=" << socketDesc << ", username=" << username << ", password=" << password);
   }
   
   virtual void onSharedSecretFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onSharedSecretFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   }

   virtual void onBindSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onBindingSuccess: socketDest=" << socketDesc << ", reflexive=" << reflexiveTuple);
      mTurnAsyncSocket->createAllocation(30,       // TurnAsyncSocket::UnspecifiedLifetime, 
                                         TurnAsyncSocket::UnspecifiedBandwidth, 
                                         StunMessage::PropsPortPair,
                                         TurnAsyncSocket::UnspecifiedToken,
                                         StunTuple::UDP);  
   }
   virtual void onBindFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onBindingFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   }

   virtual void onAllocationSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple, const StunTuple& relayTuple, unsigned int lifetime, unsigned int bandwidth, UInt64 reservationToken)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onAllocationSuccess: socketDest=" << socketDesc << 
              ", reflexive=" << reflexiveTuple << 
              ", relay=" << relayTuple <<
              ", lifetime=" << lifetime <<
              ", bandwidth=" << bandwidth <<
              ", reservationToken=" << reservationToken);

       // Test Data sending and receiving over allocation
       resip::Data turnData("This test is for wrapped Turn Data!");
       InfoLog( << "CLIENT: Sending: " << turnData);
       mTurnAsyncSocket->sendTo(asio::ip::address::from_string(address.c_str()), 2000, turnData.c_str(), turnData.size()+1);

       turnData = "This test should be in ChannelData message in TCP/TLS but not in UDP - since ChannelBindResponse is not yet received.";
       InfoLog( << "CLIENT: Sending: " << turnData);
       mTurnAsyncSocket->setActiveDestination(asio::ip::address::from_string(address.c_str()), 2000);
       mTurnAsyncSocket->send(turnData.c_str(), turnData.size()+1);       
   }
   virtual void onAllocationFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onAllocationFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   }
   virtual void onRefreshSuccess(unsigned int socketDesc, unsigned int lifetime)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onRefreshSuccess: socketDest=" << socketDesc << ", lifetime=" << lifetime);
      if(lifetime == 0)
      {
         mTurnAsyncSocket->close();
      }
   }
   virtual void onRefreshFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onRefreshFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   }

   virtual void onSetActiveDestinationSuccess(unsigned int socketDesc)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onSetActiveDestinationSuccess: socketDest=" << socketDesc);
   }
   virtual void onSetActiveDestinationFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onSetActiveDestinationFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   }
   virtual void onClearActiveDestinationSuccess(unsigned int socketDesc)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onClearActiveDestinationSuccess: socketDest=" << socketDesc);
   }
   virtual void onClearActiveDestinationFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onClearActiveDestinationFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   }

   virtual void onSendSuccess(unsigned int socketDesc)
   {
      //InfoLog( << "MyTurnAsyncSocketHandler::onSendSuccess: socketDest=" << socketDesc);
   }
   virtual void onSendFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onSendFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   }

   virtual void onReceiveSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port, boost::shared_ptr<DataBuffer>& data)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onReceiveSuccess: socketDest=" << socketDesc << ", fromAddress=" << address << ", fromPort=" << port << ", size=" << data->size() << ", data=" << data->data());

      switch(++mNumReceives)
      {
      case 1:
         break;
      case 2:
         {
            resip::Data turnData("This test is for ChannelData message!");
            InfoLog( << "CLIENT: Sending: " << turnData);
            mTurnAsyncSocket->send(turnData.c_str(), turnData.size()+1);       
         }
         break;
      case 3:
         mTurnAsyncSocket->destroyAllocation();
         //mTurnAsyncSocket->close();
         break;
      }
   }
   virtual void onReceiveFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onReceiveFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   }

   void setTurnAsyncSocket(TurnAsyncSocket* turnAsyncSocket) { mTurnAsyncSocket = turnAsyncSocket; }

private:
   TurnAsyncSocket* mTurnAsyncSocket;
   unsigned int mNumReceives;
};

int main(int argc, char* argv[])
{
#ifdef WIN32
  resip::FindMemoryLeaks fml;
#endif
  //resip::Log::initialize(resip::Log::Cout, resip::Log::Stack, argv[0]);

  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: TestAsyncClient <turn host> <turn port>\n";
      return 1;
    }
    unsigned int port = resip::Data(argv[2]).convertUnsignedLong();

    InfoLog(<< "Using " << address << " as local IP address.");

    asio::error_code rc;
    char username[256] = "test";
    char password[256] = "1234";
    TurnPeer turnPeer;
    turnPeer.run();
    asio::io_service ioService;
    MyTurnAsyncSocketHandler handler;

    asio::ssl::context sslContext(ioService, asio::ssl::context::tlsv1);
    // Setup SSL context
    sslContext.set_verify_mode(asio::ssl::context::verify_peer);
    sslContext.load_verify_file("ca.pem");

    boost::shared_ptr<TurnAsyncSocket> turnSocket(new TurnAsyncUdpSocket(ioService, &handler, asio::ip::address::from_string(address.c_str()), 0));
    //boost::shared_ptr<TurnAsyncSocket> turnSocket(new TurnAsyncTcpSocket(ioService, &handler, asio::ip::address::from_string(address.c_str()), 0));
    //boost::shared_ptr<TurnAsyncSocket> turnSocket(new TurnAsyncTlsSocket(ioService, sslContext, &handler, asio::ip::address::from_string(address.c_str()), 0)); port++;

    handler.setTurnAsyncSocket(turnSocket.get());

    // Connect to Stun/Turn Server
    turnSocket->connect(argv[1], port);

    // Set the username and password
    turnSocket->setUsernameAndPassword(username, password);

    ioService.run();

    turnPeer.shutdown();
    turnPeer.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
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

