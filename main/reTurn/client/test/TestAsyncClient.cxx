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

using namespace reTurn;
using namespace std;

#define NO_AUTHENTICATION

// Simple UDP Echo Server
class TurnPeer : public resip::ThreadIf
{
public:
   TurnPeer() {}

   virtual ~TurnPeer() {}

   virtual void thread()
   {
      asio::error_code rc;
      TurnUdpSocket turnSocket(asio::ip::address::from_string("127.0.0.1"), 2000);

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
            std::cout << "PEER: Received data from " << sourceAddress << ":" << sourcePort << " - [" << resip::Data(buffer, size).c_str() << "]" << std::endl;
            turnSocket.send(buffer, size);
         }
         size = sizeof(buffer);
         rc=turnSocket.receive(buffer, size, 1000, &sourceAddress, &sourcePort);
      }

      if(rc)
      {
         if(rc.value() != asio::error::operation_aborted)
         {
            std::cout << "PEER: Receive error: " << rc.message() << std::endl;
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
      cout << "MyTurnAsyncSocketHandler::onConnectSuccess: socketDest=" << socketDesc << ", address=" << address << ", port=" << port << endl;
      mTurnAsyncSocket->bindRequest();
   }
   virtual void onConnectFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      cout << "MyTurnAsyncSocketHandler::onConnectFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ")."  << endl;
   }

   virtual void onSharedSecretSuccess(unsigned int socketDesc, const char* username, unsigned int usernameSize, const char* password, unsigned int passwordSize)
   {
      cout << "MyTurnAsyncSocketHandler::onSharedSecretSuccess: socketDest=" << socketDesc << ", username=" << username << ", password=" << password << endl;
   }
   
   virtual void onSharedSecretFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      cout << "MyTurnAsyncSocketHandler::onSharedSecretFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ")."  << endl;
   }

   virtual void onBindSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple)
   {
      cout << "MyTurnAsyncSocketHandler::onBindingSuccess: socketDest=" << socketDesc << ", reflexive=" << reflexiveTuple << endl;
      mTurnAsyncSocket->createAllocation(30,       // TurnSocket::UnspecifiedLifetime, 
                                         TurnSocket::UnspecifiedBandwidth, 
                                         StunMessage::PortPropsEvenPair,
                                         TurnSocket::UnspecifiedPort,
                                         StunTuple::UDP);  
   }
   virtual void onBindFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      cout << "MyTurnAsyncSocketHandler::onBindingFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ")."  << endl;
   }

   virtual void onAllocationSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple, const StunTuple& relayTuple, unsigned int lifetime, unsigned int bandwidth)
   {
      cout << "MyTurnAsyncSocketHandler::onAllocationSuccess: socketDest=" << socketDesc << 
              ", reflexive=" << reflexiveTuple << 
              ", relay=" << relayTuple <<
              ", lifetime=" << lifetime <<
              ", bandwidth=" << bandwidth << endl;

       // Test Data sending and receiving over allocation
       resip::Data turnData("This test is for wrapped Turn Data!");
       cout << "CLIENT: Sending: " << turnData << endl;
       mTurnAsyncSocket->sendTo(asio::ip::address::from_string("127.0.0.1"), 2000, turnData.c_str(), turnData.size()+1);

       turnData = "This test should be framed in TCP/TLS but not in UDP - since ChannelConfirmed is not yet received.";
       cout << "CLIENT: Sending: " << turnData << endl;
       mTurnAsyncSocket->setActiveDestination(asio::ip::address::from_string("127.0.0.1"), 2000);
       mTurnAsyncSocket->send(turnData.c_str(), turnData.size()+1);       
   }
   virtual void onAllocationFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      cout << "MyTurnAsyncSocketHandler::onAllocationFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ")."  << endl;
   }
   virtual void onRefreshSuccess(unsigned int socketDesc, unsigned int lifetime)
   {
      cout << "MyTurnAsyncSocketHandler::onRefreshSuccess: socketDest=" << socketDesc << ", lifetime=" << lifetime <<endl;
      if(lifetime == 0)
      {
         mTurnAsyncSocket->close();
      }
   }
   virtual void onRefreshFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      cout << "MyTurnAsyncSocketHandler::onRefreshFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ")."  << endl;
   }

   virtual void onSetActiveDestinationSuccess(unsigned int socketDesc)
   {
      cout << "MyTurnAsyncSocketHandler::onSetActiveDestinationSuccess: socketDest=" << socketDesc << endl;
   }
   virtual void onSetActiveDestinationFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      cout << "MyTurnAsyncSocketHandler::onSetActiveDestinationFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ")."  << endl;
   }
   virtual void onClearActiveDestinationSuccess(unsigned int socketDesc)
   {
      cout << "MyTurnAsyncSocketHandler::onClearActiveDestinationSuccess: socketDest=" << socketDesc << endl;
   }
   virtual void onClearActiveDestinationFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      cout << "MyTurnAsyncSocketHandler::onClearActiveDestinationFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ")."  << endl;
   }

   virtual void onSendSuccess(unsigned int socketDesc)
   {
      cout << "MyTurnAsyncSocketHandler::onSendSuccess: socketDest=" << socketDesc << endl;
   }
   virtual void onSendFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      cout << "MyTurnAsyncSocketHandler::onSendFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ")."  << endl;
   }

   virtual void onReceiveSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port, const char* buffer, unsigned int size)
   {
      cout << "MyTurnAsyncSocketHandler::onReceiveSuccess: socketDest=" << socketDesc << ", fromAddress=" << address << ", fromPort=" << port << ", size=" << size << ", data=" << buffer << endl;

      switch(++mNumReceives)
      {
      case 1:
         break;
      case 2:
         {
            resip::Data turnData("This test is for framed turn data!");
            cout << "CLIENT: Sending: " << turnData << endl;
            mTurnAsyncSocket->send(turnData.c_str(), turnData.size());       
         }
         break;
      case 3:
         mTurnAsyncSocket->destroyAllocation();
         break;
      }
   }
   virtual void onReceiveFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      cout << "MyTurnAsyncSocketHandler::onReceiveFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ")."  << endl;
   }

   void setTurnAsyncSocket(TurnAsyncSocket* turnAsyncSocket) { mTurnAsyncSocket = turnAsyncSocket; }

private:
   TurnAsyncSocket* mTurnAsyncSocket;
   unsigned int mNumReceives;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: stunTestClient <host> <port>\n";
      return 1;
    }
    unsigned int port = resip::Data(argv[2]).convertUnsignedLong();

    asio::error_code rc;
    char username[256] = "";
    char password[256] = "";
    TurnPeer turnPeer;
    turnPeer.run();
    asio::io_service ioService;
    MyTurnAsyncSocketHandler handler;

    asio::ssl::context sslContext(ioService, asio::ssl::context::tlsv1);
    // Setup SSL context
    sslContext.set_verify_mode(asio::ssl::context::verify_peer);
    sslContext.load_verify_file("ca.pem");

    //boost::shared_ptr<TurnAsyncUdpSocket> turnSocket(new TurnAsyncUdpSocket(ioService, &handler, asio::ip::address::from_string("127.0.0.1"), 0));
    //boost::shared_ptr<TurnAsyncTcpSocket> turnSocket(new TurnAsyncTcpSocket(ioService, &handler, asio::ip::address::from_string("127.0.0.1"), 0));
    boost::shared_ptr<TurnAsyncTlsSocket> turnSocket(new TurnAsyncTlsSocket(ioService, sslContext, &handler, asio::ip::address::from_string("127.0.0.1"), 0)); port++;

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

