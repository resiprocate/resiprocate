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
#include <rutil/Timer.hxx>
#include <rutil/Random.hxx>
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace reTurn;
using namespace std;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

static unsigned int NUM_RTP_PACKETS_TO_SIMULATE=1500;  // 30 seconds worth of RTP data
static unsigned int PACKET_TIME_TO_SIMULATE=20;  // 20 ms

// Test Config 1
//#define RECEIVE_ONLY
//#define ALLOC_PORT 50000
//#define OTHER_PORT 50002

// Test Config 2
//#define SEND_ONLY
//#define ALLOC_PORT 50002
//#define OTHER_PORT 50000

// Test Config 3
//#define EXTERNAL_ECHO_SERVER
//#define OTHER_HOST "192.168.1.69"
//#define OTHER_PORT 2000

// Test Config 4
//#define ECHO_SERVER_ONLY

resip::Data address;
unsigned short relayPort = 0;
resip::Data turnAddress;
char rtpPayloadData[172];  // 172 bytes of random data to simulate RTP payload
resip::Data rtpPayload(rtpPayloadData, sizeof(rtpPayloadData));

void sleepMS(unsigned int ms)
{
#ifdef WIN32
   Sleep(ms);
#else
   usleep(ms*1000);
#endif
}

// Simple UDP Echo Server
class TurnPeer : public resip::ThreadIf
{
public:
   TurnPeer() {}

   virtual ~TurnPeer() {}

   virtual void thread()
   {
      asio::error_code rc;
      TurnUdpSocket turnSocket(asio::ip::address::from_string(address.c_str()), relayPort);
      if(relayPort == 0)
      {
         unsigned int socketFd = turnSocket.getSocketDescriptor();
         // Get Port from Fd
         socklen_t len = sizeof(sockaddr);
         sockaddr turnSockAddr;
         if(::getsockname(socketFd, &turnSockAddr, &len) == SOCKET_ERROR)
         {
            std::cerr << "Error querying sockaddr" << std::endl;
            exit(-1);
         }
         if (turnSockAddr.sa_family == AF_INET) // v4   
         {
            relayPort = ntohs(((sockaddr_in*)&turnSockAddr)->sin_port);
         }
         else
         {
#ifdef USE_IPV6
            relayPort = ntohs(((sockaddr_in6*)&turnSockAddr)->sin6_port);
#endif
         }
         std::cout << "relayPort is " << relayPort << std::endl;
      }

      char buffer[1024];
      unsigned int size = sizeof(buffer);
      asio::ip::address sourceAddress;
      unsigned short sourcePort;
      asio::ip::address connectedAddress;
      unsigned short connectedPort=0;

      // Receive Data
      rc=turnSocket.receive(buffer, size, 1000, &sourceAddress, &sourcePort);
      while((!rc || rc.value() == asio::error::operation_aborted) && !isShutdown())
      {
         if(!rc)
         {
            if(connectedAddress != sourceAddress ||
               connectedPort != sourcePort)
            {
               turnSocket.connect(sourceAddress.to_string(), sourcePort);
               connectedAddress = sourceAddress;
               connectedPort = sourcePort;
            }
            //InfoLog(<< "PEER: Received data from " << sourceAddress << ":" << sourcePort << " - [" << resip::Data(buffer, size).c_str() << "]");
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
   MyTurnAsyncSocketHandler(asio::io_service& ioService) : mIOService(ioService), mTimer(ioService), mNumReceives(0), mNumSends(0) {}
   virtual ~MyTurnAsyncSocketHandler() {}

   void sendRtpSimPacket()
   {
      if(++mNumSends <= NUM_RTP_PACKETS_TO_SIMULATE)
      {
         mTimer.expires_from_now(boost::posix_time::milliseconds(PACKET_TIME_TO_SIMULATE));   
         mTimer.async_wait(boost::bind(&MyTurnAsyncSocketHandler::sendRtpSimPacket, this));
         //InfoLog(<< "Sending packet " << mNumReceives << "...");
         mTurnAsyncSocket->send(rtpPayload.data(), rtpPayload.size());  
      }
      else
      {
         InfoLog(<< "Done sending " << NUM_RTP_PACKETS_TO_SIMULATE << " packets (" << mNumReceives << " receives have already been completed).");
         mTurnAsyncSocket->destroyAllocation();
      }
   }

   virtual void onConnectSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onConnectSuccess: socketDest=" << socketDesc << ", address=" << address.to_string() << ", port=" << port);
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

   virtual void onBindSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple, const StunTuple& stunServerTuple)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onBindingSuccess: socketDest=" << socketDesc << ", reflexive=" << reflexiveTuple << ", stunServerTuple=" << stunServerTuple);
      mTurnAsyncSocket->createAllocation(30,       // TurnSocket::UnspecifiedLifetime, 
                                         TurnSocket::UnspecifiedBandwidth, 
                                         StunMessage::PropsPortPair,
                                         TurnAsyncSocket::UnspecifiedToken,
                                         StunTuple::UDP);  
   }
   virtual void onBindFailure(unsigned int socketDesc, const asio::error_code& e, const StunTuple& stunServerTuple)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onBindingFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << "), stunServerTuple=" << stunServerTuple);
   }

   virtual void onAllocationSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple, const StunTuple& relayTuple, unsigned int lifetime, unsigned int bandwidth, UInt64 reservationToken)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onAllocationSuccess: socketDest=" << socketDesc << 
              ", reflexive=" << reflexiveTuple << 
              ", relay=" << relayTuple <<
              ", lifetime=" << lifetime <<
              ", bandwidth=" << bandwidth <<
              ", reservationToken=" << reservationToken);

#ifdef RECEIVE_ONLY
       // Send one packet of data so that it opens permission
       mTurnAsyncSocket->sendTo(asio::ip::address::from_string(turnAddress.c_str()), OTHER_PORT, rtpPayload.data(), rtpPayload.size());  
#else
#ifdef SEND_ONLY
       mTurnAsyncSocket->setActiveDestination(asio::ip::address::from_string(turnAddress.c_str()), OTHER_PORT);
#else
#ifdef EXTERNAL_ECHO_SERVER
       mTurnAsyncSocket->setActiveDestination(asio::ip::address::from_string(OTHER_HOST), OTHER_PORT);
#else
       mTurnAsyncSocket->setActiveDestination(asio::ip::address::from_string(address.c_str()), relayPort);
#endif
#endif
#endif
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
         InfoLog(<< "It took " << (time(0) - mStartTime) << " seconds to do " << NUM_RTP_PACKETS_TO_SIMULATE << " receives paced at " << PACKET_TIME_TO_SIMULATE << "ms apart.");

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
      InfoLog(<< "Sending RTP payload...");
      mStartTime = time(0);
      sendRtpSimPacket();
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
   virtual void onChannelBindRequestSent(unsigned int socketDesc, unsigned short channelNumber)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onChannelBindRequestSent: socketDest=" << socketDesc << " channelNumber=" << channelNumber);
   }
   virtual void onChannelBindSuccess(unsigned int socketDesc, unsigned short channelNumber)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onChannelBindSuccess: socketDest=" << socketDesc << " channelNumber=" << channelNumber);
   }
   virtual void onChannelBindFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onChannelBindFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
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
      //InfoLog( << "MyTurnAsyncSocketHandler::onReceiveSuccess: socketDest=" << socketDesc << ", fromAddress=" << address << ", fromPort=" << port << ", size=" << data->size() << ", data=" << data->data()); 
      if(++mNumReceives == NUM_RTP_PACKETS_TO_SIMULATE)
      {
         InfoLog(<< "Done receiving " << NUM_RTP_PACKETS_TO_SIMULATE << " packets.");
         //mTurnAsyncSocket->destroyAllocation();
      }
   }
   virtual void onReceiveFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onReceiveFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   }

   virtual void onIncomingBindRequestProcessed(unsigned int socketDesc, const StunTuple& sourceTuple)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onIncomingBindRequestProcessed: socketDest=" << socketDesc << " sourceTuple=" << sourceTuple);
   }

   void setTurnAsyncSocket(TurnAsyncSocket* turnAsyncSocket) { mTurnAsyncSocket = turnAsyncSocket; }

private:
   asio::io_service& mIOService;
   asio::deadline_timer mTimer;
   TurnAsyncSocket* mTurnAsyncSocket;
   unsigned int mNumReceives;
   unsigned int mNumSends;
   time_t mStartTime;
   UInt64 mRTPSendTime;
};

int main(int argc, char* argv[])
{
#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
  resip::FindMemoryLeaks fml;
#endif
  resip::Log::initialize("cout", "INFO", "TestRtpLoad");

  try
  {
    //if (argc == 2)
    //{
    //   InfoLog(<< "Starting Echo server only...");
    //   TurnPeer turnPeer;
    //   turnPeer.run();
    //   turnPeer.join();
    //   return 0;
    //}
    
    address = resip::DnsUtil::getLocalIpAddress();

    if (argc < 3)
    {
      std::cerr << "Usage: TestRtpLoad <host> <port> <loaclAddress> [<PacketTime>]\n";
      return 1;
    }
    turnAddress = argv[1];
    unsigned int port = resip::Data(argv[2]).convertUnsignedLong();
    address = argv[3];

    if(argc == 5)
    {
       PACKET_TIME_TO_SIMULATE = atoi(argv[3]);
    }
    InfoLog(<< "Using " << address << " as local IP address.");

    resip::Random::initialize();

    asio::error_code rc;
    char username[256] = "test";
    char password[256] = "1234";
#ifndef OTHER_PORT
    TurnPeer turnPeer;
    turnPeer.run();
#endif

#ifndef ECHO_SERVER_ONLY
    asio::io_service ioService;
    MyTurnAsyncSocketHandler handler(ioService);

    asio::ssl::context sslContext(ioService, asio::ssl::context::tlsv1);
    // Setup SSL context
    sslContext.set_verify_mode(asio::ssl::context::verify_peer);
    sslContext.load_verify_file("ca.pem");

    boost::shared_ptr<TurnAsyncSocket> turnSocket(new TurnAsyncUdpSocket(ioService, &handler, asio::ip::address::from_string(address.c_str()), 0));
    //boost::shared_ptr<TurnAsyncSocket> turnSocket(new TurnAsyncTcpSocket(ioService, &handler, asio::ip::address::from_string(address.c_str()), 0));
    //boost::shared_ptr<TurnAsyncSocket> turnSocket(new TurnAsyncTlsSocket(ioService, sslContext, &handler, asio::ip::address::from_string(address.c_str()), 0)); port++;

    handler.setTurnAsyncSocket(turnSocket.get());

    // Connect to Stun/Turn Server
    turnSocket->connect(turnAddress.c_str(), port);

    // Set the username and password
    turnSocket->setUsernameAndPassword(username, password);

    ioService.run();

#ifndef OTHER_PORT
    turnPeer.shutdown();
    turnPeer.join();
#endif
#else
    turnPeer.join();
#endif
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
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
