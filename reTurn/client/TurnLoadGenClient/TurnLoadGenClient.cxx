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

static unsigned int NUM_RTP_PACKETS_TO_SIMULATE=1500;  // 30 seconds worth of RTP data
static unsigned int PACKET_TIME_TO_SIMULATE=20;  // 20 ms

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
   TurnPeer(const Data& localAddress, unsigned int port) :
      mLocalAddress(localAddress),
      mPort(port)
   {
      mUdpSocket = new TurnUdpSocket(asio::ip::address::from_string(mLocalAddress.c_str()), mPort);
      if (mPort == 0)
      {
         unsigned int socketFd = mUdpSocket->getSocketDescriptor();
         // Get Port from Fd
         socklen_t len = sizeof(sockaddr);
         sockaddr turnSockAddr;
         if (::getsockname(socketFd, &turnSockAddr, &len) == SOCKET_ERROR)
         {
            ErrLog(<< "TurnPeer: Error querying sockaddr");
            exit(-1);
         }
         if (turnSockAddr.sa_family == AF_INET) // v4   
         {
            mPort = ntohs(((sockaddr_in*)&turnSockAddr)->sin_port);
         }
         else
         {
#ifdef USE_IPV6
            mPort = ntohs(((sockaddr_in6*)&turnSockAddr)->sin6_port);
#endif
         }
      }
      InfoLog(<< "TurnPeer: RelayPort is " << mPort);
   }

   virtual ~TurnPeer() 
   {
      delete mUdpSocket;
   }

   virtual unsigned int getRelayPort() { return mPort; }

   virtual void thread()
   {
      asio::error_code rc;

      char buffer[1024];
      unsigned int size = sizeof(buffer);
      asio::ip::address sourceAddress;
      unsigned short sourcePort;
      asio::ip::address connectedAddress;
      unsigned short connectedPort=0;

      // Receive Data
      rc=mUdpSocket->receive(buffer, size, 1000, &sourceAddress, &sourcePort);
      while((!rc || rc.value() == asio::error::operation_aborted) && !isShutdown())
      {
         if(!rc)
         {
            if(connectedAddress != sourceAddress ||
               connectedPort != sourcePort)
            {
               mUdpSocket->connect(sourceAddress.to_string(), sourcePort);
               connectedAddress = sourceAddress;
               connectedPort = sourcePort;
            }
            //InfoLog(<< "PEER: Received data from " << sourceAddress << ":" << sourcePort << " - [" << resip::Data(buffer, size).c_str() << "]");
            mUdpSocket->send(buffer, size);
         }
         size = sizeof(buffer);
         rc= mUdpSocket->receive(buffer, size, 1000, &sourceAddress, &sourcePort);
      }

      if(rc)
      {
         if(rc.value() != asio::error::operation_aborted)
         {
            ErrLog(<< "TurnPeer: Receive error: " << rc.message());
         }
      }
   }

private:
   Data mLocalAddress;
   unsigned int mPort;
   TurnUdpSocket* mUdpSocket;
};

class MyTurnAsyncSocketHandler : public TurnAsyncSocketHandler
{
public:
   MyTurnAsyncSocketHandler(asio::io_service& ioService, const Data& localAddress, unsigned int relayPort, const ConfigParse& config) :
      mTimer(ioService),
      mLocalAddress(asio::ip::address::from_string(localAddress.c_str())),
      mRelayPort(relayPort),
      mConfig(config),
      mNumReceives(0), 
      mNumSends(0) 
   {
   }

   virtual ~MyTurnAsyncSocketHandler() {}

   void sendBindRequest()
   {
      mTurnAsyncSocket->bindRequest();
   }

   void sendAllocationRequest()
   {
      mTurnAsyncSocket->createAllocation(30,       // TurnSocket::UnspecifiedLifetime, 
         TurnSocket::UnspecifiedBandwidth,
         StunMessage::PropsPortPair,
         TurnAsyncSocket::UnspecifiedToken,
         StunTuple::UDP);
   }

   void startSendingData()
   {
      InfoLog(<< "Sending RTP payload...");
      mStartTime = time(0);
      sendRtpSimPacket();
   }

   void sendRtpSimPacket()
   {
      if(++mNumSends <= NUM_RTP_PACKETS_TO_SIMULATE)
      {
         mTimer.expires_from_now(milliseconds(PACKET_TIME_TO_SIMULATE));
         mTimer.async_wait(std::bind(&MyTurnAsyncSocketHandler::sendRtpSimPacket, this));
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
      InfoLog( << "MyTurnAsyncSocketHandler::onConnectSuccess: socketDest=" << socketDesc << ", address=" << address.to_string() << ", turnPort=" << port);

      if (mConfig.getConfigBool("SendInitialBind", true))
      {
         sendBindRequest();
      }
      else
      {
         sendAllocationRequest();
      }
   }

   virtual void onConnectFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      ErrLog( << "MyTurnAsyncSocketHandler::onConnectFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   }

   virtual void onSharedSecretSuccess(unsigned int socketDesc, const char* username, unsigned int usernameSize, const char* password, unsigned int passwordSize)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onSharedSecretSuccess: socketDest=" << socketDesc << ", username=" << username << ", password=" << password);
   }
   
   virtual void onSharedSecretFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      ErrLog( << "MyTurnAsyncSocketHandler::onSharedSecretFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   }

   virtual void onBindSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple, const StunTuple& stunServerTuple)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onBindingSuccess: socketDest=" << socketDesc << ", reflexive=" << reflexiveTuple << ", stunServerTuple=" << stunServerTuple);
      sendAllocationRequest();
   }
   virtual void onBindFailure(unsigned int socketDesc, const asio::error_code& e, const StunTuple& stunServerTuple)
   {
      ErrLog( << "MyTurnAsyncSocketHandler::onBindingFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << "), stunServerTuple=" << stunServerTuple);
   }

   virtual void onAllocationSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple, const StunTuple& relayTuple, unsigned int lifetime, unsigned int bandwidth, uint64_t reservationToken)
   {
      InfoLog(<< "MyTurnAsyncSocketHandler::onAllocationSuccess: socketDest=" << socketDesc <<
         ", reflexive=" << reflexiveTuple <<
         ", relay=" << relayTuple <<
         ", lifetime=" << lifetime <<
         ", bandwidth=" << bandwidth <<
         ", reservationToken=" << reservationToken);

      mTurnAsyncSocket->setActiveDestination(mLocalAddress, mRelayPort);
   }

   virtual void onAllocationFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      ErrLog( << "MyTurnAsyncSocketHandler::onAllocationFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
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
      ErrLog( << "MyTurnAsyncSocketHandler::onRefreshFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   }

   virtual void onSetActiveDestinationSuccess(unsigned int socketDesc)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onSetActiveDestinationSuccess: socketDest=" << socketDesc);
      startSendingData();
   }

   virtual void onSetActiveDestinationFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      ErrLog( << "MyTurnAsyncSocketHandler::onSetActiveDestinationFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   }

   virtual void onClearActiveDestinationSuccess(unsigned int socketDesc)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onClearActiveDestinationSuccess: socketDest=" << socketDesc);
   }

   virtual void onClearActiveDestinationFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      ErrLog( << "MyTurnAsyncSocketHandler::onClearActiveDestinationFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
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
      ErrLog( << "MyTurnAsyncSocketHandler::onChannelBindFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   }

   virtual void onSendSuccess(unsigned int socketDesc)
   {
      //InfoLog( << "MyTurnAsyncSocketHandler::onSendSuccess: socketDest=" << socketDesc);
   }

   virtual void onSendFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      ErrLog( << "MyTurnAsyncSocketHandler::onSendFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   }

   virtual void onReceiveSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port, const std::shared_ptr<reTurn::DataBuffer>& data)
   {
      //InfoLog( << "MyTurnAsyncSocketHandler::onReceiveSuccess: socketDest=" << socketDesc << ", fromAddress=" << address << ", fromPort=" << turnPort << ", size=" << data->size() << ", data=" << data->data()); 
      if(++mNumReceives == NUM_RTP_PACKETS_TO_SIMULATE)
      {
         InfoLog(<< "Done receiving " << NUM_RTP_PACKETS_TO_SIMULATE << " packets.");
         // Note:  Sending logic will destroyAllocation when the send period expires after sending the last packet
      }
   }

   virtual void onReceiveFailure(unsigned int socketDesc, const asio::error_code& e)
   {
      ErrLog( << "MyTurnAsyncSocketHandler::onReceiveFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   }

   virtual void onIncomingBindRequestProcessed(unsigned int socketDesc, const StunTuple& sourceTuple)
   {
      InfoLog( << "MyTurnAsyncSocketHandler::onIncomingBindRequestProcessed: socketDest=" << socketDesc << " sourceTuple=" << sourceTuple);
   }

   void setTurnAsyncSocket(TurnAsyncSocket* turnAsyncSocket) { mTurnAsyncSocket = turnAsyncSocket; }

private:
   asio::steady_timer mTimer;
   asio::ip::address mLocalAddress;
   unsigned int mRelayPort;
   const ConfigParse& mConfig;
   TurnAsyncSocket* mTurnAsyncSocket;
   unsigned int mNumReceives;
   unsigned int mNumSends;
   time_t mStartTime;
};

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

      Data username = config.getConfigData("TurnServerUsername", "test", true);
      Data password = config.getConfigData("TurnServerPassword", "pwd", true);

      // Start Echo server
      TurnPeer turnPeer(localAddress, echoServerPort);
      turnPeer.run();

      asio::io_service ioService;
      MyTurnAsyncSocketHandler handler(ioService, localAddress, turnPeer.getRelayPort(), config);

      std::shared_ptr<TurnAsyncSocket> turnSocket;

#ifdef USE_SSL
      asio::ssl::context sslContext(asio::ssl::context::tlsv1);
      // Setup SSL context
      sslContext.set_verify_mode(asio::ssl::context::verify_peer);
      sslContext.load_verify_file(config.getConfigData("TLSRootCertFile", "ca.pem", true).c_str());

      if (isEqualNoCase(turnProtocol, "TLS"))
      {
         turnSocket = std::make_shared<TurnAsyncTlsSocket>(ioService, sslContext, false, &handler, asio::ip::address::from_string(localAddress.c_str()), 0);
      }
      else 
#endif
      if (isEqualNoCase(turnProtocol, "TCP"))
      {
         turnSocket = std::make_shared<TurnAsyncTcpSocket>(ioService, &handler, asio::ip::address::from_string(localAddress.c_str()), 0);
      }
      else
      {
         turnSocket = std::make_shared<TurnAsyncUdpSocket>(ioService, &handler, asio::ip::address::from_string(localAddress.c_str()), 0);
      }

      handler.setTurnAsyncSocket(turnSocket.get());

      // Connect to Stun/Turn Server
      turnSocket->connect(turnAddress.c_str(), turnPort);

      // Set the username and password
      turnSocket->setUsernameAndPassword(username.c_str(), password.c_str());

      ioService.run();

      turnPeer.shutdown();
      turnPeer.join();
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
