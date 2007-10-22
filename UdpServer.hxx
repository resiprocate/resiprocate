#ifndef UDP_SERVER_HXX
#define UDP_SERVER_HXX

#include <asio.hpp>
#include <string>
#include <boost/noncopyable.hpp>
#include "RequestHandler.hxx"
#include "TurnTransportBase.hxx"

namespace reTurn {

class StunMessage;

class UdpServer
  : public TurnTransportBase,
    private boost::noncopyable
{
public:
   /// Create the server to listen on the specified UDP address and port
   explicit UdpServer(asio::io_service& ioService, RequestHandler& requestHandler, const std::string& address, unsigned short port);

   /// This method is used if this UdpServer supports RFC3489 operation
   void setAlternateUdpServers(UdpServer* alternatePort, UdpServer* alternateIp, UdpServer* alternateIpPort);
   bool isRFC3489BackwardsCompatServer();

   /// Returns the socket for this server
   asio::ip::udp::socket& getSocket();

   virtual void sendData(const StunTuple& destination, const char* buffer, unsigned int size);

private:
   /// Handle completion of a receive_from operation
   void handleReceiveFrom(const asio::error_code& e, std::size_t bytesTransferred);

   /// Handle completion of a sendTo operation
   void handleSendTo(const asio::error_code& error, std::size_t bytesTransferred);

   /// Socket for the connection.
   asio::ip::udp::socket mSocket;

   /// Endpoint info for current sender
   asio::ip::udp::endpoint mSenderEndpoint;

   /// The handler for all incoming requests.
   RequestHandler& mRequestHandler;

   /// The RFC3489 Alternate Server
   UdpServer* mAlternatePortUdpServer;
   UdpServer* mAlternateIpUdpServer;
   UdpServer* mAlternateIpPortUdpServer;

   /// Buffer for incoming data.
   boost::array<unsigned char, 8192> mBuffer;
};

}

#endif 


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

