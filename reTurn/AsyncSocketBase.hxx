#ifndef ASYNC_SOCKET_BASE_HXX
#define ASYNC_SOCKET_BASE_HXX

#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif

#include "DataBuffer.hxx"
#include "StunTuple.hxx"

#include <chrono>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

constexpr size_t RECEIVE_BUFFER_SIZE = 4096; // ?slg? should we shrink this to something closer to MTU (1500 bytes)? !hbr! never actually increase it otherwise re-assembled UDP packets get lost. (was 2048)

namespace reTurn {

class AsyncSocketBaseHandler;
class AsyncSocketBaseDestroyedHandler;

class AsyncSocketBase :
   public std::enable_shared_from_this<AsyncSocketBase>
{
public:
   using BeforeClosedHandler = std::function<void(unsigned int)>;

   explicit AsyncSocketBase(asio::io_context& ioService);
   virtual ~AsyncSocketBase();

   virtual unsigned int getSocketDescriptor() = 0;

   virtual void registerAsyncSocketBaseHandler(AsyncSocketBaseHandler* handler) { mAsyncSocketBaseHandler = handler; }

   /// Note:  The following API's are thread safe and queue the request to be handled by the ioService thread
   virtual asio::error_code bind(const asio::ip::address& address, unsigned short port) = 0;
   virtual void connect(const std::string& address, unsigned short port) = 0;  
   /// Note: destination is ignored for TCP and TLS connections
   virtual void send(const StunTuple& destination, const std::shared_ptr<DataBuffer>& data);  // Send unframed data
   virtual void send(const StunTuple& destination, unsigned short channel, const std::shared_ptr<DataBuffer>& data);  // send with turn framing
   /// Overlapped calls to receive functions have no effect
   virtual void receive();  
   virtual void framedReceive();  
   virtual void close();

   bool isConnected() const noexcept { return mConnected; }
   asio::ip::address& getConnectedAddress() noexcept { return mConnectedAddress; }
   unsigned short getConnectedPort() const noexcept { return mConnectedPort; }

   virtual void setOnBeforeSocketClosedFp(BeforeClosedHandler fp) { mOnBeforeSocketCloseFp = std::move(fp); }

   /// Use these if you already operating within the ioService thread
   virtual void doSend(const StunTuple& destination, unsigned short channel, const std::shared_ptr<DataBuffer>& data, const std::size_t bufferStartPos = 0);
   virtual void doSend(const StunTuple& destination, const std::shared_ptr<DataBuffer>& data, const std::size_t bufferStartPos = 0);
   virtual void doReceive();
   virtual void doFramedReceive();

   /// Class override callbacks
   virtual void onConnectSuccess() { resip_assert(false); }
   virtual void onConnectFailure(const asio::error_code& e) { resip_assert(false); }
   virtual void onReceiveSuccess(const asio::ip::address& address, unsigned short port, const std::shared_ptr<DataBuffer>& data) = 0;
   virtual void onReceiveFailure(const asio::error_code& e) = 0;
   virtual void onSendSuccess() = 0;
   virtual void onSendFailure(const asio::error_code& e) = 0;

   /// Utility API
   static std::shared_ptr<DataBuffer> allocateBuffer(size_t size);

   /// Over TCP and TLS, a TURN ChannelData message is padded to a multiple of 4 bytes, the padding
   /// not counted in its length (RFC 5766 11.5, RFC 8656 12.5): the padding after one of length bytes
   static size_t channelDataPadding(size_t length) { return (4 - length % 4) % 4; }

   /// For a UDP socket, between open() and bind().  UDP sockets don't set SO_REUSEADDR: there's no
   /// TIME_WAIT for it to help with, and it would let the socket share a port another socket has
   /// (or, on Windows, bind over it), splitting its traffic.  On Windows this sets
   /// SO_EXCLUSIVEADDRUSE, so neither can another socket - one that sets SO_REUSEADDR - bind over
   /// this one.
   static void setExclusiveAddressUse(asio::ip::udp::socket& socket);

   // Stubbed out async handlers needed by Protocol specific Subclasses of this - the requirement for these 
   // to be in the base class all revolves around the shared_from_this() use/requirement
   virtual void start() { resip_assert(false); }
   virtual void stop() { resip_assert(false); }
   virtual void handleReadHeader(const asio::error_code& e) { resip_assert(false); }
   virtual void handleServerHandshake(const asio::error_code& e) { resip_assert(false); }
   virtual void handleTcpResolve(const asio::error_code& ec, const asio::ip::tcp::resolver::results_type& endpoints) { resip_assert(false); }
   virtual void handleUdpResolve(const asio::error_code& ec, const asio::ip::udp::resolver::results_type& endpoints) { resip_assert(false); }
   virtual void handleConnect(const asio::error_code& ec, const asio::ip::tcp::endpoint& endpoint) { resip_assert(false); }
   virtual void handleClientHandshake(const asio::error_code& ec, const asio::ip::tcp::endpoint& endpoint) { resip_assert(false); }

protected:
   /// Handle completion of a sendData operation.
   virtual void handleSend(const asio::error_code& e);
   virtual void handleReceive(const asio::error_code& e, size_t bytesTransferred);

   /// For TCP and TLS client sockets.  openAndBind opens socket and binds it to mLocalTcpEndpoint
   /// (set by bind()).  asyncConnect connects socket to each of endpoints in turn until one
   /// succeeds, then calls handleConnect - like asio::async_connect, but binding the socket before
   /// each attempt: asio::async_connect closes it first, losing the binding.
   asio::error_code openAndBind(asio::ip::tcp::socket& socket);
   void asyncConnect(asio::ip::tcp::socket& socket, const asio::ip::tcp::resolver::results_type& endpoints);
   asio::ip::tcp::endpoint mLocalTcpEndpoint;

   /// For server connections (TcpConnectionIdleTimeout): the connection is closed, with close(),
   /// once nothing has been received on it for timeoutSeconds while inUse() is false - eg. while
   /// it carries no allocation.  0 for no timeout.  Call noteReceived() for each message received,
   /// and stopIdleTimer() when the connection is stopped.
   void startIdleTimer(unsigned int timeoutSeconds, std::function<bool()> inUse);
   void noteReceived() { mLastReceived = std::chrono::steady_clock::now(); }
   void stopIdleTimer();

   /// The io_context used to perform asynchronous operations.
   asio::io_context& mIOService;

   /// Receive Buffer and state
   std::shared_ptr<DataBuffer> mReceiveBuffer;
   bool mReceiving;

   /// Connected Info and State
   asio::ip::address mConnectedAddress;
   unsigned short mConnectedPort;
   bool mConnected;

   /// Handlers
   AsyncSocketBaseHandler* mAsyncSocketBaseHandler;

   /// Provides an opportunity for the app to clean up, e.g., QoS-related data or resources
   /// just before the socket is closed
   BeforeClosedHandler mOnBeforeSocketCloseFp;

private:
   void asyncConnectFrom(asio::ip::tcp::socket& socket, asio::ip::tcp::resolver::results_type endpoints,
                         asio::ip::tcp::resolver::results_type::const_iterator next, asio::error_code lastError);

   void waitIdle(std::chrono::steady_clock::duration wait);
   std::unique_ptr<asio::steady_timer> mIdleTimer;
   std::chrono::steady_clock::duration mIdleTimeout;
   std::function<bool()> mInUse;
   std::chrono::steady_clock::time_point mLastReceived;

   virtual void transportSend(const StunTuple& destination, std::vector<asio::const_buffer>& buffers) = 0;
   virtual void transportReceive() = 0;
   virtual void transportFramedReceive() = 0;
   virtual void transportClose() = 0;

   virtual asio::ip::address getSenderEndpointAddress() = 0;
   virtual unsigned short getSenderEndpointPort() = 0;

   virtual void sendFirstQueuedData();
   class SendData
   {
   public:
      SendData(const StunTuple& destination, std::shared_ptr<DataBuffer> frameData, std::shared_ptr<DataBuffer> data, size_t bufferStartPos = 0, size_t padding = 0) :
         mDestination(destination), mFrameData(frameData), mData(data), mBufferStartPos(bufferStartPos), mPadding(padding) {}
      StunTuple mDestination;
      std::shared_ptr<DataBuffer> mFrameData;
      std::shared_ptr<DataBuffer> mData;
      size_t mBufferStartPos;
      size_t mPadding;  // bytes sent after the data: ChannelData over TCP/TLS is padded to a multiple of 4
   };
   /// Queue of data to send
   typedef std::deque<SendData> SendDataQueue;
   SendDataQueue mSendDataQueue;
};

typedef std::shared_ptr<AsyncSocketBase> ConnectionPtr;

}

#endif 


/* ====================================================================

 Copyright (c) 2008-2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
