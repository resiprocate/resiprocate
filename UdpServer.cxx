#include "UdpServer.hxx"
#include "StunMessage.hxx"
#include <boost/bind.hpp>

using namespace std;

namespace reTurn {

UdpServer::UdpServer(asio::io_service& ioService, RequestHandler& requestHandler, const std::string& address, const std::string& port)
: TurnTransportBase(ioService),
  mSocket(ioService), // , asio::ip::udp::endpoint(asio::ip::address::from_string(address), port))
  mRequestHandler(requestHandler),
  mAlternatePortUdpServer(0),
  mAlternateIpUdpServer(0),
  mAlternateIpPortUdpServer(0)
{
   // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
   asio::ip::udp::resolver resolver(mIOService);
   asio::ip::udp::resolver::query query(address, port);
   asio::ip::udp::endpoint endpoint = *resolver.resolve(query);

   mSocket.open(endpoint.protocol());
   mSocket.set_option(asio::ip::udp::socket::reuse_address(true));
   mSocket.bind(endpoint);

   std::cout << "UdpServer started.  Listening on " << address << ":" << port << std::endl;

   mSocket.async_receive_from(asio::buffer(mBuffer), mSenderEndpoint,
      boost::bind(&UdpServer::handleReceiveFrom, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
}

void 
UdpServer::setAlternateUdpServers(UdpServer* alternatePort, UdpServer* alternateIp, UdpServer* alternateIpPort)
{
   assert(!mAlternatePortUdpServer);
   assert(!mAlternateIpUdpServer);
   assert(!mAlternateIpPortUdpServer);
   mAlternatePortUdpServer = alternatePort;
   mAlternateIpUdpServer = alternateIp;
   mAlternateIpPortUdpServer = alternateIpPort;
}

asio::ip::udp::socket& 
UdpServer::getSocket()
{
   return mSocket;
}

void 
UdpServer::sendData(const StunTuple& destination, const char* buffer, unsigned int size)
{
   mSocket.async_send_to(asio::buffer(buffer, size), 
                                 asio::ip::udp::endpoint(destination.getAddress(), destination.getPort()), 
                                 boost::bind(&TurnTransportBase::handleSendData, this, asio::placeholders::error));
}

void 
UdpServer::handleReceiveFrom(const asio::error_code& e, std::size_t bytesTransferred)
{
   if (!e && bytesTransferred > 0)
   {
      bool treatAsData=false;
      /*
      std::cout << "Read " << bytesTransferred << " bytes from udp socket (" << mSenderEndpoint.address().to_string() << ":" << mSenderEndpoint.port() << "): " << std::hex << std::endl;
      for(int i = 0; i < bytesTransferred; i++)
      {
         std::cout << (char)mBuffer[i] << "(" << int(mBuffer[i]) << ") ";
      }
      std::cout << std::dec << std::endl;
      */

      // Check if first byte is 0x00 - if so data might be STUN message
      if(bytesTransferred >= 1 && mBuffer[0] == 0x00)
      {
         // if this is a TURN relay usage UDP server - then we check upfront for existance of StunMagicCookie
         if(!mAlternatePortUdpServer && bytesTransferred > 8 )  // If alternate servers are not set, then assume this is Turn relay usage only
         {
            unsigned int magicCookie;  // Stun magic cookie is in bytes 4-8
            memcpy(&magicCookie, &mBuffer[4], sizeof(magicCookie));
            //magicCookie = ntohl(magicCookie);
            if(magicCookie != StunMessage::StunMagicCookie)
            {
               treatAsData = true;
            }
         }
      }
      else
      {
         treatAsData = true;
      }

      if(!treatAsData)
      {
         // Try to parse stun message
         StunMessage request(StunTuple(StunTuple::UDP, mSocket.local_endpoint().address(), mSocket.local_endpoint().port()),
                             StunTuple(StunTuple::UDP, mSenderEndpoint.address(), mSenderEndpoint.port()),
                             (char*)mBuffer.c_array(), (unsigned int)bytesTransferred,
                             mAlternatePortUdpServer ? &mAlternatePortUdpServer->getSocket() : 0,
                             mAlternateIpUdpServer ? &mAlternateIpUdpServer->getSocket() : 0,
                             mAlternateIpPortUdpServer ? &mAlternateIpPortUdpServer->getSocket() : 0);
         if(!request.isValid())
         {
            treatAsData = true;
         }
         else
         {
            StunMessage response;
            asio::ip::udp::socket* responseSocket;
            RequestHandler::ProcessResult result = mRequestHandler.processStunMessage(this, request, response);

            switch(result)
            {
            case RequestHandler::NoResponseToSend:
               // No response to send - just receive next message
               mSocket.async_receive_from(asio::buffer(mBuffer), mSenderEndpoint,
                  boost::bind(&UdpServer::handleReceiveFrom, this, asio::placeholders::error, asio::placeholders::bytes_transferred));  
               return;
            case RequestHandler::RespondFromAlternatePort:
               responseSocket = &mAlternatePortUdpServer->getSocket();
               break;
            case RequestHandler::RespondFromAlternateIp:
               responseSocket = &mAlternateIpUdpServer->getSocket();
               break;
            case RequestHandler::RespondFromAlternateIpPort:
               responseSocket = &mAlternateIpPortUdpServer->getSocket();
               break;
            case RequestHandler::RespondFromReceiving:
            default:
                responseSocket = &mSocket;            
                break;
            }
            unsigned int size = response.stunEncodeMessage((char*)mBuffer.c_array(), (unsigned int)mBuffer.size());

            responseSocket->async_send_to(asio::buffer(mBuffer, size), asio::ip::udp::endpoint(response.mRemoteTuple.getAddress(), response.mRemoteTuple.getPort()), 
                 boost::bind(&UdpServer::handleSendTo, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
         }
      }
      if(treatAsData)
      {
         mRequestHandler.processTurnData(StunTuple(StunTuple::UDP, mSocket.local_endpoint().address(), mSocket.local_endpoint().port()),
                                         StunTuple(StunTuple::UDP, mSenderEndpoint.address(), mSenderEndpoint.port()),
                                         (char*)mBuffer.c_array(), (unsigned int)bytesTransferred);

         mSocket.async_receive_from(asio::buffer(mBuffer), mSenderEndpoint,
                  boost::bind(&UdpServer::handleReceiveFrom, this, asio::placeholders::error, asio::placeholders::bytes_transferred));  
      }
   }
   else if(e != asio::error::operation_aborted)
   {
      mSocket.async_receive_from(asio::buffer(mBuffer), mSenderEndpoint,
         boost::bind(&UdpServer::handleReceiveFrom, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
   }
}

void 
UdpServer::handleSendTo(const asio::error_code& error, std::size_t bytesTransferred)
{
   mSocket.async_receive_from(asio::buffer(mBuffer), mSenderEndpoint,
      boost::bind(&UdpServer::handleReceiveFrom, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
}

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

