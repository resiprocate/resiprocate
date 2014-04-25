#ifndef UDP_SERVER_HXX
#define UDP_SERVER_HXX

#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif
#include <string>
#include <boost/noncopyable.hpp>
#include "RequestHandler.hxx"
#include "AsyncUdpSocketBase.hxx"

namespace reTurn {

class StunMessage;

class UdpServer
  : public AsyncUdpSocketBase,
    private boost::noncopyable
{
public:
   /// Create the server to listen on the specified UDP address and port
   explicit UdpServer(asio::io_service& ioService, RequestHandler& requestHandler, const asio::ip::address& address, unsigned short port);
   ~UdpServer();

   void start();

   /// This method is used if this UdpServer supports RFC3489 operation - note turnFraming in contructor must be false
   void setAlternateUdpServers(UdpServer* alternatePort, UdpServer* alternateIp, UdpServer* alternateIpPort);
   bool isRFC3489BackwardsCompatServer();

   ///// Returns the socket for this server
   asio::ip::udp::socket& getSocket();

   void cleanupResponseMap(const asio::error_code& e, UInt128 tid);

private:
   /// Handle completion of a receive operation
   virtual void onReceiveSuccess(const asio::ip::address& address, unsigned short port, boost::shared_ptr<DataBuffer>& data);
   virtual void onReceiveFailure(const asio::error_code& e);

   /// Handle completion of a send operation
   virtual void onSendSuccess();
   virtual void onSendFailure(const asio::error_code& e);

   /// Manages turn allocations
   TurnAllocationManager mTurnAllocationManager;

   /// The handler for all incoming requests.
   RequestHandler& mRequestHandler;

   // Stores the local address and port
   asio::ip::address mLocalAddress;
   unsigned short mLocalPort;

   /// The RFC3489 Alternate Server
   UdpServer* mAlternatePortUdpServer;
   UdpServer* mAlternateIpUdpServer;
   UdpServer* mAlternateIpPortUdpServer;

   // Response map (for retransmissions)
   class ResponseEntry
   {
   public:
      ResponseEntry(UdpServer* requestUdpServer, UdpServer* responseUdpServer, StunMessage* responseMessage);
      ~ResponseEntry();

      UdpServer* mResponseUdpServer;
      StunMessage* mResponseMessage;
      asio::deadline_timer mCleanupTimer;
   };
   typedef std::map<UInt128, ResponseEntry*> ResponseMap;
   ResponseMap mResponseMap;
};

}

#endif 


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
