#ifndef TURNASYNCSOCKET_HXX
#define TURNASYNCSOCKET_HXX

#include <vector>
#include <asio.hpp>
#include <rutil/Data.hxx>
#include <rutil/Mutex.hxx>

#include "../StunTuple.hxx"
#include "../StunMessage.hxx"
#include "../ChannelManager.hxx"
#include "TurnAsyncSocketHandler.hxx"

namespace reTurn {

class TurnAsyncSocket
{
public:
   static unsigned int UnspecifiedLifetime;
   static unsigned int UnspecifiedBandwidth;
   static unsigned short UnspecifiedPort;
   static asio::ip::address UnspecifiedIpAddress;

   explicit TurnAsyncSocket(asio::io_service& ioService,
                            TurnAsyncSocketHandler* turnAsyncSocketHandler,
                            const asio::ip::address& address = UnspecifiedIpAddress, 
                            unsigned short port = 0, 
                            bool turnFraming = true);
   virtual ~TurnAsyncSocket();

   virtual unsigned int getSocketDescriptor() = 0;

   // Note: Shared Secret requests have been deprecated by RFC3489-bis11, and not 
   //       widely implemented in RFC3489 - so not really needed at all
   void requestSharedSecret();

   // Set the username and password for all future requests
   void setUsernameAndPassword(const char* username, const char* password);

   // Stun Binding Method - use getReflexiveTuple() to get binding info
   void bindRequest();

   // Turn Allocation Methods
   void createAllocation(unsigned int lifetime = UnspecifiedLifetime,
                         unsigned int bandwidth = UnspecifiedBandwidth,
                         unsigned short requestedPortProps = StunMessage::PortPropsNone, 
                         unsigned short requestedPort = UnspecifiedPort,
                         StunTuple::TransportType requestedTransportType = StunTuple::None, 
                         const asio::ip::address &requestedIpAddress = UnspecifiedIpAddress);
   void refreshAllocation(unsigned int lifetime);
   void destroyAllocation();

   // Methods to control active destination
   asio::error_code setActiveDestination(const asio::ip::address& address, unsigned short port);
   asio::error_code clearActiveDestination();

   // Turn Send Methods
   virtual void send(const char* buffer, unsigned int size);
   virtual void sendTo(const asio::ip::address& address, unsigned short port, const char* buffer, unsigned int size);

   // Receive Methods
   //asio::error_code receive(char* buffer, unsigned int& size, unsigned int timeout, asio::ip::address* sourceAddress=0, unsigned short* sourcePort=0);
   //asio::error_code receiveFrom(const asio::ip::address& address, unsigned short port, char* buffer, unsigned int& size, unsigned int timeout);

   virtual void close() = 0;

protected:
   // Note: destination is ignored for TCP and TLS connections
   virtual void send(resip::SharedPtr<resip::Data> data)=0;  // Send unframed data
   virtual void send(unsigned short channel, resip::SharedPtr<resip::Data> data)=0;  // send with turn framing
   virtual void receive() = 0;  

   virtual bool isConnected() const = 0;
   virtual const asio::ip::address& getConnectedAddress() const = 0;
   virtual unsigned short getConnectedPort() const = 0;

   void handleReceivedData(const asio::ip::address& address, unsigned short port, resip::SharedPtr<resip::Data> data);

   asio::io_service& mIOService;
   TurnAsyncSocketHandler* mTurnAsyncSocketHandler;
   bool mTurnFraming;

   // Local Binding Info
   StunTuple mLocalBinding;

   // Authentication Info
   resip::Data mUsername;
   resip::Data mPassword;

   // Turn Allocation Properties used in request
   StunTuple::TransportType mRequestedTransportType;

   // Turn Allocation Properties from response
   bool mHaveAllocation;
   time_t mAllocationRefreshTime;
   StunTuple::TransportType mRelayTransportType;
   unsigned int mLifetime;
   unsigned int mBandwidth;

   ChannelManager mChannelManager;
   RemotePeer* mActiveDestination;

private:
   resip::Mutex mMutex;
   void sendStunMessage(StunMessage& request);
   void sendTo(RemotePeer& remotePeer, const char* buffer, unsigned int size);
   asio::error_code handleStunMessage(StunMessage& stunMessage);
   asio::error_code handleDataInd(StunMessage& stunMessage);
   asio::error_code handleChannelConfirmation(StunMessage &stunMessage);
   asio::error_code handleSharedSecretResponse(StunMessage& stunMessage);
   asio::error_code handleBindRequest(StunMessage& stunMessage);
   asio::error_code handleBindResponse(StunMessage& stunMessage);
   asio::error_code handleAllocateResponse(StunMessage& stunMessage);
   asio::error_code handleRefreshResponse(StunMessage& stunMessage);
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

