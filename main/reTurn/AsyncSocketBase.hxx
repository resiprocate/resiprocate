#ifndef ASYNC_SOCKET_BASE_HXX
#define ASYNC_SOCKET_BASE_HXX

#include <deque>
#include <asio.hpp>
#include <boost/bind.hpp>
#include <rutil/Data.hxx>
#include <rutil/SharedPtr.hxx>
#include <boost/enable_shared_from_this.hpp>

#include "StunTuple.hxx"

#define RECEIVE_BUFFER_SIZE 8192

namespace reTurn {

class AsyncSocketBaseHandler;
class AsyncSocketBaseDestroyedHandler;

class AsyncSocketBase :
   public boost::enable_shared_from_this<AsyncSocketBase>
{
public:
   AsyncSocketBase(asio::io_service& ioService);
   virtual ~AsyncSocketBase();

   virtual unsigned int getSocketDescriptor() = 0;

   virtual void registerAsyncSocketBaseHandler(AsyncSocketBaseHandler* handler) { mAsyncSocketBaseHandler = handler; }

   /// Note:  The following API's are thread safe and queue the request to be handled by the ioService thread
   virtual asio::error_code bind(const asio::ip::address& address, unsigned short port) = 0;
   virtual void connect(const std::string& address, unsigned short port) = 0;  
   /// Note: destination is ignored for TCP and TLS connections
   virtual void send(const StunTuple& destination, resip::SharedPtr<resip::Data> data);  // Send unframed data
   virtual void send(const StunTuple& destination, unsigned short channel, resip::SharedPtr<resip::Data> data);  // send with turn framing
   /// WARNING - don't overllap calls to receive functions
   virtual void receive();  
   virtual void framedReceive();  
   virtual void close();

   bool isConnected() { return mConnected; }
   asio::ip::address& getConnectedAddress() { return mConnectedAddress; }
   unsigned short getConnectedPort() { return mConnectedPort; }

   /// Use these if you already operating within the ioService thread
   virtual void doSend(const StunTuple& destination, unsigned short channel, resip::SharedPtr<resip::Data> data, unsigned int bufferStartPos=0);
   virtual void doSend(const StunTuple& destination, resip::SharedPtr<resip::Data> data, unsigned int bufferStartPos=0);
   virtual void doReceive();
   virtual void doFramedReceive();

   /// Class override callbacks
   virtual void onConnectSuccess() { assert(false); }
   virtual void onConnectFailure(const asio::error_code& e) { assert(false); }
   virtual void onReceiveSuccess(const asio::ip::address& address, unsigned short port, resip::SharedPtr<resip::Data> data) = 0;
   virtual void onReceiveFailure(const asio::error_code& e) = 0;
   virtual void onSendSuccess() = 0;
   virtual void onSendFailure(const asio::error_code& e) = 0;

   /// Utility API
   static resip::SharedPtr<resip::Data> allocateBuffer(unsigned int size);

   // Stubbed out async handlers needed by Protocol specific Subclasses of this - the requirement for these 
   // to be in the base class all revolves around the shared_from_this() use/requirement
   virtual void start() { assert(false); }
   virtual void stop() { assert(false); }
   virtual void handleReadHeader(const asio::error_code& e) { assert(false); }
   virtual void handleServerHandshake(const asio::error_code& e) { assert(false); }
   virtual void handleTcpResolve(const asio::error_code& ec, asio::ip::tcp::resolver::iterator endpoint_iterator) { assert(false); }
   virtual void handleUdpResolve(const asio::error_code& ec, asio::ip::udp::resolver::iterator endpoint_iterator) { assert(false); }
   virtual void handleConnect(const asio::error_code& ec, asio::ip::tcp::resolver::iterator endpoint_iterator) { assert(false); }
   virtual void handleClientHandshake(const asio::error_code& ec, asio::ip::tcp::resolver::iterator endpoint_iterator) { assert(false); }

protected:
   /// Handle completion of a sendData operation.
   virtual void handleSend(const asio::error_code& e);
   virtual void handleReceive(const asio::error_code& e, std::size_t bytesTransferred);

   /// The io_service used to perform asynchronous operations.
   asio::io_service& mIOService;

   /// Receive Buffer and state
   resip::SharedPtr<resip::Data> mReceiveBuffer;
   bool mReceiving;

   /// Connected Info and State
   asio::ip::address mConnectedAddress;
   unsigned short mConnectedPort;
   bool mConnected;

   /// Handlers
   AsyncSocketBaseHandler* mAsyncSocketBaseHandler;

private:
   virtual void transportSend(const StunTuple& destination, std::vector<asio::const_buffer>& buffers) = 0;
   virtual void transportReceive() = 0;
   virtual void transportFramedReceive() = 0;
   virtual void transportClose() = 0;

   virtual const asio::ip::address getSenderEndpointAddress() = 0;
   virtual unsigned short getSenderEndpointPort() = 0;

   virtual void sendFirstQueuedData();
   class SendData
   {
   public:
      SendData(const StunTuple& destination, resip::SharedPtr<resip::Data> frameData, resip::SharedPtr<resip::Data> data, unsigned int bufferStartPos = 0) :
         mDestination(destination), mFrameData(frameData), mData(data), mBufferStartPos(bufferStartPos) {}
      StunTuple mDestination;
      resip::SharedPtr<resip::Data> mFrameData;
      resip::SharedPtr<resip::Data> mData;
      unsigned int mBufferStartPos;
   };
   /// Queue of data to send
   typedef std::deque<SendData> SendDataQueue;
   SendDataQueue mSendDataQueue;
};

typedef boost::shared_ptr<AsyncSocketBase> ConnectionPtr;

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

