#include "UdpServer.hxx"
#include "StunMessage.hxx"
#include <boost/bind.hpp>

using namespace std;

namespace reTurn {

UdpServer::UdpServer(asio::io_service& ioService, RequestHandler& requestHandler, const asio::ip::address& address, unsigned short port)
: TurnTransportBase(ioService),
  mSocket(ioService), //asio::ip::udp::endpoint(asio::ip::address::from_string(address), port))
  mRequestHandler(requestHandler),
  mAlternatePortUdpServer(0),
  mAlternateIpUdpServer(0),
  mAlternateIpPortUdpServer(0)
{
   // Open with the option to reuse the address (i.e. SO_REUSEADDR).
   asio::ip::udp::endpoint endpoint(address, port);

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

bool 
UdpServer::isRFC3489BackwardsCompatServer()
{
   return mAlternatePortUdpServer != 0;  // Just check that any of the alternate servers is populated - if so, we are running in back compat mode
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
                                 boost::bind(&UdpServer::handleSendData, this, asio::placeholders::error));
}

void 
UdpServer::handleReceiveFrom(const asio::error_code& e, std::size_t bytesTransferred)
{
   if (!e && bytesTransferred > 0)
   {
      char* stunMessageBuffer = 0;
      unsigned int stunMessageSize = 0;

      bool treatAsData=false;
      /*
      std::cout << "Read " << bytesTransferred << " bytes from udp socket (" << mSenderEndpoint.address().to_string() << ":" << mSenderEndpoint.port() << "): " << std::endl;
      cout << std::hex;
      for(int i = 0; i < bytesTransferred; i++)
      {
         std::cout << (char)mBuffer[i] << "(" << int(mBuffer[i]) << ") ";
      }
      std::cout << std::dec << std::endl;
      */
      unsigned char channelNumber = mBuffer[0];

      if(!isRFC3489BackwardsCompatServer())
      {
         // All Turn messaging will be framed
         if(channelNumber == 0) // Stun/Turn Request
         {
            stunMessageBuffer = (char*)&mBuffer[4];
            stunMessageSize = (unsigned int)bytesTransferred-4;
         }
         else  
         {
            // Turn Data
            treatAsData = true;
         }
      }
      else
      {
         stunMessageBuffer = (char*)&mBuffer[0];
         stunMessageSize = (unsigned int)bytesTransferred;
      }

      if(!treatAsData)
      {
         if(stunMessageBuffer && stunMessageSize)
         {
            // Try to parse stun message
            StunMessage request(StunTuple(StunTuple::UDP, mSocket.local_endpoint().address(), mSocket.local_endpoint().port()),
                              StunTuple(StunTuple::UDP, mSenderEndpoint.address(), mSenderEndpoint.port()),
                              stunMessageBuffer, stunMessageSize);
            if(request.isValid())
            {
               StunMessage* response;
               asio::ip::udp::socket* responseSocket;
               ResponseMap::iterator it = mResponseMap.find(request.mHeader.magicCookieAndTid);
               if(it == mResponseMap.end())
               {
                  response = new StunMessage;
                  RequestHandler::ProcessResult result = mRequestHandler.processStunMessage(this, request, *response);

                  switch(result)
                  {
                  case RequestHandler::NoResponseToSend:
                     // No response to send - just receive next message
                     delete response;
                     mSocket.async_receive_from(asio::buffer(mBuffer), 
                                                mSenderEndpoint,
                                                boost::bind(&UdpServer::handleReceiveFrom, 
                                                            this, 
                                                            asio::placeholders::error, 
                                                            asio::placeholders::bytes_transferred));  
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

                  // Store response in Map - to be resent if a retranmission is received
                  mResponseMap[response->mHeader.magicCookieAndTid] = new ResponseEntry(this, responseSocket, response);
               }
               else
               {
                  std::cout << "UdpServer: received retransmission of request with tid: " << request.mHeader.magicCookieAndTid << std::endl;
                  response = it->second->mResponseMessage;
                  responseSocket = it->second->mResponseSocket;
               }
               unsigned int responseSize;
               if(isRFC3489BackwardsCompatServer())
               {
                  responseSize = response->stunEncodeMessage((char*)mBuffer.c_array(), (unsigned int)mBuffer.size());
               }
               else
               {
                  responseSize = response->stunEncodeFramedMessage((char*)mBuffer.c_array(), (unsigned int)mBuffer.size());
               }

               responseSocket->async_send_to(asio::buffer(mBuffer, responseSize), 
                                             asio::ip::udp::endpoint(response->mRemoteTuple.getAddress(), response->mRemoteTuple.getPort()), 
                                             boost::bind(&UdpServer::handleSendTo, 
                                                         this, 
                                                         asio::placeholders::error, 
                                                         asio::placeholders::bytes_transferred));
            }
         }
      } 
      else
      {
         if(bytesTransferred > 4)
         {
            mRequestHandler.processTurnData(channelNumber,
                                            StunTuple(StunTuple::UDP, mSocket.local_endpoint().address(), mSocket.local_endpoint().port()),
                                            StunTuple(StunTuple::UDP, mSenderEndpoint.address(), mSenderEndpoint.port()),
                                            (char*)&mBuffer[4], (unsigned int)bytesTransferred-4);
         }
         else
         {
            cout << "UdpServer::handleReceiveFrom not enough data for framed message - discarding!" << endl;
         }

         mSocket.async_receive_from(asio::buffer(mBuffer), mSenderEndpoint,
                  boost::bind(&UdpServer::handleReceiveFrom, this, asio::placeholders::error, asio::placeholders::bytes_transferred));  
      }
   }
   else if(e != asio::error::operation_aborted)
   {
      cout << "handleReceiveFrom error: " << e.message() << endl;

      mSocket.async_receive_from(asio::buffer(mBuffer), 
                                 mSenderEndpoint,
                                 boost::bind(&UdpServer::handleReceiveFrom, 
                                             this, 
                                             asio::placeholders::error, 
                                             asio::placeholders::bytes_transferred));
   }
}

void 
UdpServer::handleSendTo(const asio::error_code& error, std::size_t bytesTransferred)
{
   mSocket.async_receive_from(asio::buffer(mBuffer), mSenderEndpoint,
      boost::bind(&UdpServer::handleReceiveFrom, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
}

UdpServer::ResponseEntry::ResponseEntry(UdpServer* udpServer, asio::ip::udp::socket* responseSocket, StunMessage* responseMessage) :
   mResponseSocket(responseSocket),
   mResponseMessage(responseMessage),
   mCleanupTimer(udpServer->mIOService)
{
   // start timer
   mCleanupTimer.expires_from_now(boost::posix_time::seconds(10));  // Transaction Responses are cahced for 10 seconds
   mCleanupTimer.async_wait(boost::bind(&UdpServer::cleanupResponseMap, udpServer, asio::placeholders::error, responseMessage->mHeader.magicCookieAndTid));
}

UdpServer::ResponseEntry::~ResponseEntry() 
{ 
   delete mResponseMessage; 
}

void 
UdpServer::cleanupResponseMap(const asio::error_code& e, UInt128 tid)
{
   ResponseMap::iterator it = mResponseMap.find(tid);
   if(it != mResponseMap.end())
   {
      cout << "UdpServer::cleanupResponseMap - removing transaction id=" << tid << endl;
      delete it->second;
      mResponseMap.erase(it);
   }
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

