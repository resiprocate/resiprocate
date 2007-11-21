#include "UdpServer.hxx"
#include "StunMessage.hxx"
#include <boost/bind.hpp>
#include <rutil/SharedPtr.hxx>

using namespace std;
using namespace resip;

namespace reTurn {

UdpServer::UdpServer(asio::io_service& ioService, RequestHandler& requestHandler, const asio::ip::address& address, unsigned short port)
: AsyncUdpSocketBase(ioService, address, port),
  mRequestHandler(requestHandler),
  mAlternatePortUdpServer(0),
  mAlternateIpUdpServer(0),
  mAlternateIpPortUdpServer(0)
{
   std::cout << "UdpServer started.  Listening on " << address << ":" << port << std::endl;

   registerAsyncSocketBaseHandler(this);
}

UdpServer::~UdpServer()
{
   registerAsyncSocketBaseHandler(0);
}

void 
UdpServer::start()
{
   doReceive();
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
UdpServer::onReceiveSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port, resip::SharedPtr<resip::Data> data)
{
   if (data->size() > 4)
   {
      char* stunMessageBuffer = 0;
      unsigned int stunMessageSize = 0;

      bool treatAsData=false;
      /*
      std::cout << "Read " << bytesTransferred << " bytes from udp socket (" << address.to_string() << ":" << port << "): " << std::endl;
      cout << std::hex;
      for(int i = 0; i < data->size(); i++)
      {
         std::cout << (char)(*data)[i] << "(" << int((*data)[i]) << ") ";
      }
      std::cout << std::dec << std::endl;
      */
      unsigned short channelNumber;
      memcpy(&channelNumber, &(*data)[0], 2);
      channelNumber = ntohs(channelNumber);

      if(!isRFC3489BackwardsCompatServer())
      {
         // All Turn messaging will be framed
         if(channelNumber == 0) // Stun/Turn Request
         {
            stunMessageBuffer = (char*)&(*data)[4];
            stunMessageSize = (unsigned int)data->size()-4;
         }
         else  
         {
            // Turn Data
            treatAsData = true;
         }
      }
      else
      {
         stunMessageBuffer = (char*)&(*data)[0];
         stunMessageSize = data->size();
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
                     doReceive();
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
#define RESPONSE_BUFFER_SIZE 1024
               SharedPtr<Data> buffer = allocateBuffer(RESPONSE_BUFFER_SIZE);
               unsigned int responseSize;
               if(isRFC3489BackwardsCompatServer())
               {
                  responseSize = response->stunEncodeMessage((char*)buffer->data(), RESPONSE_BUFFER_SIZE);
               }
               else
               {
                  responseSize = response->stunEncodeFramedMessage((char*)buffer->data(), RESPONSE_BUFFER_SIZE);
               }
               buffer->truncate(responseSize);  // set size to real size

               doSend(response->mRemoteTuple, buffer);
            }
         }
      } 
      else
      {
         mRequestHandler.processTurnData(channelNumber,
                                         StunTuple(StunTuple::UDP, mSocket.local_endpoint().address(), mSocket.local_endpoint().port()),
                                         StunTuple(StunTuple::UDP, mSenderEndpoint.address(), mSenderEndpoint.port()),
                                         data);
      }
   }
   else
   {
      cout << "UdpServer::handleReceiveFrom not enough data for framed message - discarding!" << endl;
   }

   doReceive();
}

void 
UdpServer::onReceiveFailure(unsigned int socketDesc, const asio::error_code& e)
{
   if(e != asio::error::operation_aborted)
   {
      cout << "UdpServer::onReceiveFailure: " << e.message() << endl;

      doReceive();
   }
}

void
UdpServer::onSendSuccess(unsigned int socketDesc)
{
}

void
UdpServer::onSendFailure(unsigned int socketDesc, const asio::error_code& error)
{
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

