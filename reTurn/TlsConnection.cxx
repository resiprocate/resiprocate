#ifdef WIN32
#pragma warning(disable : 4267)
#endif

#include "TlsConnection.hxx"
#include <vector>
#include <boost/bind.hpp>
#include "ConnectionManager.hxx"
#include "RequestHandler.hxx"
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;
using namespace resip;

namespace reTurn {

TlsConnection::TlsConnection(asio::io_service& ioService,
                             ConnectionManager& manager, 
                             RequestHandler& handler, 
                             asio::ssl::context& context)
  : AsyncTlsSocketBase(ioService, context, false /* not needed in server */),
    mConnectionManager(manager),
    mRequestHandler(handler)
{
}

TlsConnection::~TlsConnection()
{
   DebugLog(<< "TlsConnection destroyed.");
}

ssl_socket::lowest_layer_type& 
TlsConnection::socket()
{
  return mSocket.lowest_layer();
}

void 
TlsConnection::start()
{
   DebugLog(<< "TlsConnection started.");

   doHandshake();
}

void 
TlsConnection::stop()
{
  asio::error_code ec;
  //mSocket.shutdown(ec);  // !slg! note: this fn gives a stack overflow since ASIO 1.0.0 for some reason
  mSocket.lowest_layer().close(ec);
  if(ec)
  {
     WarningLog(<< "TlsConnection shutdown, error=" << ec.value() << "-" << ec.message());
  }
}

void 
TlsConnection::close()
{
   mConnectionManager.stop(shared_from_this());
}

void 
TlsConnection::onServerHandshakeSuccess()
{
   DebugLog(<< "TlsConnection handshake completed.");
   asio::error_code ec;
   mLocalAddress = mSocket.lowest_layer().local_endpoint().address();
   mLocalPort = mSocket.lowest_layer().local_endpoint().port();
   doFramedReceive();
}
 
void 
TlsConnection::onServerHandshakeFailure(const asio::error_code& e)
{
   WarningLog(<< "TlsConnection handshake failure, error=" << e.value() << "-" << e.message());
   close();
}

void 
TlsConnection::onReceiveSuccess(const asio::ip::address& address, unsigned short port, boost::shared_ptr<DataBuffer>& data)
{
   if (data->size() > 4)
   {
      /*
      std::cout << "Read " << bytesTransferred << " bytes from tls socket (" << address.to_string() << ":" << port << "): " << std::endl;
      cout << std::hex;
      for(int i = 0; i < data->size(); i++)
      {
         std::cout << (char)(*data)[i] << "(" << int((*data)[i]) << ") ";
      }
      std::cout << std::dec << std::endl;
      */

      if(((*data)[0] & 0xC0) == 0)  // Stun/Turn Messages always have bits 0 and 1 as 00 - otherwise ChannelData message
      {
         // Try to parse stun message
         StunMessage request(StunTuple(StunTuple::TLS, mLocalAddress, mLocalPort),
                             StunTuple(StunTuple::TLS, address, port),
                             (char*)&(*data)[0], data->size());
         if(request.isValid())
         {
            StunMessage response;
            RequestHandler::ProcessResult result = mRequestHandler.processStunMessage(this, mTurnAllocationManager, request, response);

            switch(result)
            {
            case RequestHandler::NoResponseToSend:
               // No response to send - just receive next message
               doFramedReceive();
               return;
            case RequestHandler::RespondFromAlternatePort:
            case RequestHandler::RespondFromAlternateIp:
            case RequestHandler::RespondFromAlternateIpPort:
               // These only happen for UDP server for RFC3489 backwards compatibility
               resip_assert(false);
               break;
            case RequestHandler::RespondFromReceiving:
            default:
               break;
            }
#define RESPONSE_BUFFER_SIZE 1024
            boost::shared_ptr<DataBuffer> buffer = allocateBuffer(RESPONSE_BUFFER_SIZE);
            unsigned int responseSize;
            responseSize = response.stunEncodeMessage((char*)buffer->data(), RESPONSE_BUFFER_SIZE);
            buffer->truncate(responseSize);  // set size to real size

            doSend(response.mRemoteTuple, buffer);
         }
         else
         {
            WarningLog(<< "Received invalid StunMessage.  Dropping.");
         }
      }
      else // ChannelData message
      {
         unsigned short channelNumber;
         memcpy(&channelNumber, &(*data)[0], 2);
         channelNumber = ntohs(channelNumber);

         mRequestHandler.processTurnData(mTurnAllocationManager,
                                         channelNumber,
                                         StunTuple(StunTuple::TLS, mLocalAddress, mLocalPort),
                                         StunTuple(StunTuple::TLS, address, port),
                                         data);
      }
   }
   else
   {
      WarningLog(<< "Not enough data for stun message or framed message.  Closing connection.");
      close();
      return;
   }

   doFramedReceive();
}

void 
TlsConnection::onReceiveFailure(const asio::error_code& e)
{
   if(e != asio::error::operation_aborted)
   {
      InfoLog(<< "TlsConnection::onReceiveFailure: " << e.value() << "-" << e.message());

      close();
   }
}

void
TlsConnection::onSendSuccess()
{
}

void
TlsConnection::onSendFailure(const asio::error_code& error)
{
   if(error != asio::error::operation_aborted)
   {
      InfoLog(<< "TlsConnection::onSendFailure: " << error.value() << "-" << error.message());
      close();
   }
}

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
