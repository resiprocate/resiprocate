#ifndef TURNSOCKET_HXX
#define TURNSOCKET_HXX

#if defined(BOOST_MSVC) && (BOOST_MSVC >= 1400) \
  && (!defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0600) \
  && !defined(ASIO_ENABLE_CANCELIO)
#error You must define ASIO_ENABLE_CANCELIO in your build settings.
#endif

#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif
#include <boost/bind.hpp>

#include <vector>

#include <rutil/Data.hxx>
#include <rutil/Mutex.hxx>

#include "reTurn/StunTuple.hxx"
#include "reTurn/StunMessage.hxx"
#include "reTurn/ChannelManager.hxx"

namespace reTurn {

class TurnSocket
{
public:
   static unsigned int UnspecifiedLifetime;
   static unsigned int UnspecifiedBandwidth;
   static unsigned short UnspecifiedToken;
   static asio::ip::address UnspecifiedIpAddress;

   explicit TurnSocket(const asio::ip::address& address = UnspecifiedIpAddress, 
                       unsigned short port = 0);
   virtual ~TurnSocket();

   virtual unsigned int getSocketDescriptor() = 0;
   virtual asio::error_code connect(const std::string& address, unsigned short port) = 0;  // !slg! modify port parameter later to be able to do SRV lookups

   // Note: Shared Secret requests have been deprecated by RFC5389, and not 
   //       widely implemented in RFC3489 - so not really needed at all
   asio::error_code requestSharedSecret(char* username, unsigned int usernameSize, 
                                        char* password, unsigned int passwordSize);

   // Set the username and password for all future requests
   void setUsernameAndPassword(const char* username, const char* password, bool shortTermAuth=false);

   // Stun Binding Method - use getReflexiveTuple() to get binding info
   asio::error_code bindRequest();

   // Turn Allocation Methods
   asio::error_code createAllocation(unsigned int lifetime = UnspecifiedLifetime,
                                     unsigned int bandwidth = UnspecifiedBandwidth,
                                     unsigned char requestedProps = StunMessage::PropsNone, 
                                     UInt64 reservationToken = UnspecifiedToken,
                                     StunTuple::TransportType requestedTransportType = StunTuple::None);
   asio::error_code refreshAllocation();
   asio::error_code destroyAllocation();

   // Accessors for properties associated with an allocation
   StunTuple& getRelayTuple();
   StunTuple& getReflexiveTuple();
   unsigned int getLifetime();
   unsigned int getBandwidth();

   // Methods to control active destination
   asio::error_code setActiveDestination(const asio::ip::address& address, unsigned short port);
   asio::error_code clearActiveDestination();

   // Turn Send Methods
   asio::error_code send(const char* buffer, unsigned int size);
   asio::error_code sendTo(const asio::ip::address& address, unsigned short port, const char* buffer, unsigned int size);

   // Receive Methods
   asio::error_code receive(char* buffer, unsigned int& size, unsigned int timeout, asio::ip::address* sourceAddress=0, unsigned short* sourcePort=0);
   asio::error_code receiveFrom(const asio::ip::address& address, unsigned short port, char* buffer, unsigned int& size, unsigned int timeout);

protected:
   virtual asio::error_code rawWrite(const char* buffer, unsigned int size) = 0;
   virtual asio::error_code rawWrite(const std::vector<asio::const_buffer>& buffers) = 0;
   virtual asio::error_code rawRead(unsigned int timeout, unsigned int* bytesRead, asio::ip::address* sourceAddress=0, unsigned short* sourcePort=0) = 0;
   virtual void cancelSocket() = 0;

   // Local Binding Info
   StunTuple mLocalBinding;
   StunTuple mConnectedTuple;

   // Authentication Info
   resip::Data mUsername;
   resip::Data mPassword;
   resip::Data mHmacKey;
   resip::Data mRealm;
   resip::Data mNonce;

   // Turn Allocation Properties used in request
   unsigned int mRequestedLifetime;
   unsigned int mRequestedBandwidth;
   unsigned char mRequestedProps;
   UInt64 mReservationToken;
   StunTuple::TransportType mRequestedTransportType;

   // Turn Allocation Properties from response
   bool mHaveAllocation;
   time_t mAllocationRefreshTime;
   StunTuple mRelayTuple;
   StunTuple mReflexiveTuple;
   unsigned int mLifetime;
   unsigned int mBandwidth;

   ChannelManager mChannelManager;
   typedef std::map<unsigned short, time_t> ChannelBindingRefreshTimeMap;
   ChannelBindingRefreshTimeMap mChannelBindingRefreshTimes;
   RemotePeer* mActiveDestination;

   asio::io_service mIOService;

   // handlers and timers required to do a timed out read
   asio::deadline_timer mReadTimer;
   size_t mBytesRead;
   asio::error_code mReadErrorCode;
   void startReadTimer(unsigned int timeout);
   void handleRawRead(const asio::error_code& errorCode, size_t bytesRead);
   void handleRawReadTimeout(const asio::error_code& errorCode);

   // Read/Write Buffers
   char mReadBuffer[8192];
   char mWriteBuffer[8192];
   bool mConnected;

private:
   resip::Mutex mMutex;
   asio::error_code channelBind(RemotePeer& remotePeer);
   asio::error_code checkIfAllocationRefreshRequired();
   asio::error_code checkIfChannelBindingRefreshRequired();
   StunMessage* sendRequestAndGetResponse(StunMessage& request, asio::error_code& errorCode, bool addAuthInfo=true);
   asio::error_code sendTo(RemotePeer& remotePeer, const char* buffer, unsigned int size);
   asio::error_code handleStunMessage(StunMessage& stunMessage, char* buffer, unsigned int& size, asio::ip::address* sourceAddress=0, unsigned short* sourcePort=0);
   asio::error_code handleRawData(char* data, unsigned int dataSize,  unsigned int expectedSize, char* buffer, unsigned int& bufferSize);
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
