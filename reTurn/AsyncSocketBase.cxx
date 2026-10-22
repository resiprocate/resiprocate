#include "AsyncSocketBase.hxx"
#include "AsyncSocketBaseHandler.hxx"
#include <rutil/WinLeakCheck.hxx>
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#include <functional>
#include <iterator>

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;

#define NO_CHANNEL ((unsigned short)-1)

namespace reTurn {

AsyncSocketBase::AsyncSocketBase(asio::io_context& ioService) : 
  mIOService(ioService),
  mReceiving(false),
  mConnected(false),
  mAsyncSocketBaseHandler(nullptr)
{
}

AsyncSocketBase::~AsyncSocketBase()
{
   if(mAsyncSocketBaseHandler) mAsyncSocketBaseHandler->onSocketDestroyed();
}

void 
AsyncSocketBase::send(const StunTuple& destination, const std::shared_ptr<DataBuffer>& data)
{
   shared_ptr<AsyncSocketBase> _this = shared_from_this();
   asio::dispatch(mIOService, std::bind([_this, destination, data](){
	   _this->doSend(destination, data, 0);
   }));
}

void 
AsyncSocketBase::send(const StunTuple& destination, unsigned short channel, const std::shared_ptr<DataBuffer>& data)
{
   shared_ptr<AsyncSocketBase> _this = shared_from_this();
   asio::dispatch(mIOService, std::bind([_this, destination, channel, data](){
      _this->doSend(destination, channel, data, 0);
   }));
}

void
AsyncSocketBase::doSend(const StunTuple& destination, const std::shared_ptr<DataBuffer>& data, const std::size_t bufferStartPos)
{
   doSend(destination, NO_CHANNEL, data, bufferStartPos);
}

void
AsyncSocketBase::doSend(const StunTuple& destination, unsigned short channel, const std::shared_ptr<DataBuffer>& data, const std::size_t bufferStartPos)
{
   bool writeInProgress = !mSendDataQueue.empty();
   if (channel == NO_CHANNEL)
   {
      mSendDataQueue.push_back(SendData(destination, nullptr, data, bufferStartPos));
   }
   else
   {
      // Add Turn Framing
      const auto frame = allocateBuffer(4);
      channel = htons(channel);
      memcpy(&(*frame)[0], &channel, 2);
      unsigned short msgsize = htons((unsigned short)data->size());
      memcpy(&(*frame)[2], (void*)&msgsize, 2);
      const size_t padding = destination.getTransportType() == StunTuple::UDP ? 0 : channelDataPadding(data->size());
      mSendDataQueue.push_back(SendData(destination, frame, data, bufferStartPos, padding));
   }
   if (!writeInProgress)
   {
      sendFirstQueuedData();
   }
}

void 
AsyncSocketBase::handleSend(const asio::error_code& e)
{
   if(!e)
   {
      onSendSuccess();
   }
   else
   {
      DebugLog(<< "handleSend with error: " << e);
      onSendFailure(e);
   }

   // TODO - check if closed here, and if so don't try and send more
   // Clear this data from the queue and see if there is more data to send
   mSendDataQueue.pop_front();
   if (!mSendDataQueue.empty())
   {
      sendFirstQueuedData();
   }
}

void 
AsyncSocketBase::sendFirstQueuedData()
{
   std::vector<asio::const_buffer> bufs;
   if (mSendDataQueue.front().mFrameData) // If we have frame data
   {
      bufs.push_back(asio::buffer(mSendDataQueue.front().mFrameData->data(), mSendDataQueue.front().mFrameData->size()));
   }
   bufs.push_back(asio::buffer(mSendDataQueue.front().mData->data()+mSendDataQueue.front().mBufferStartPos, mSendDataQueue.front().mData->size()-mSendDataQueue.front().mBufferStartPos));
   if (mSendDataQueue.front().mPadding)
   {
      static const char zeros[3] = { 0, 0, 0 };
      bufs.push_back(asio::buffer(zeros, mSendDataQueue.front().mPadding));
   }
   transportSend(mSendDataQueue.front().mDestination, bufs);
}

void 
AsyncSocketBase::receive()
{
   asio::post(mIOService, std::bind(&AsyncSocketBase::doReceive, shared_from_this()));
}

void
AsyncSocketBase::doReceive()
{
   if(!mReceiving)
   {
      mReceiving=true;
      mReceiveBuffer = allocateBuffer(RECEIVE_BUFFER_SIZE);
      transportReceive();
   }
}

void 
AsyncSocketBase::framedReceive()
{
   asio::post(mIOService, std::bind(&AsyncSocketBase::doFramedReceive, shared_from_this()));
}

void
AsyncSocketBase::doFramedReceive()
{
   if(!mReceiving)
   {
      mReceiving=true;
      mReceiveBuffer = allocateBuffer(RECEIVE_BUFFER_SIZE);
      transportFramedReceive();
   }
}

void 
AsyncSocketBase::handleReceive(const asio::error_code& e, const size_t bytesTransferred)
{
   mReceiving = false;

   if(!e)
   {
      // Handoff received buffer to appliction, and prepare receive buffer for next call
      mReceiveBuffer->truncate(bytesTransferred);
      onReceiveSuccess(getSenderEndpointAddress(), getSenderEndpointPort(), mReceiveBuffer);
   }
   else
   {
      DebugLog(<< "handleReceive with error: " << e);
      onReceiveFailure(e);
   }
}

void 
AsyncSocketBase::close()
{
   asio::post(mIOService, std::bind(&AsyncSocketBase::transportClose, shared_from_this()));
}

std::shared_ptr<DataBuffer>
AsyncSocketBase::allocateBuffer(const size_t size)
{
   return std::make_shared<DataBuffer>(size);
}

void
AsyncSocketBase::setExclusiveAddressUse(asio::ip::udp::socket& socket)
{
#ifdef _WIN32
   BOOL on = TRUE;
   if(::setsockopt(socket.native_handle(), SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&on, sizeof(on)) != 0)
   {
      WarningLog(<< "Unable to set SO_EXCLUSIVEADDRUSE on a UDP socket, error=" << WSAGetLastError());
   }
#else
   (void)socket;
#endif
}

asio::error_code
AsyncSocketBase::openAndBind(asio::ip::tcp::socket& socket)
{
   asio::error_code errorCode;
   socket.close(errorCode);
   socket.open(mLocalTcpEndpoint.protocol(), errorCode);
   if(!errorCode)
   {
      asio::error_code ignored;
      socket.set_option(asio::ip::tcp::no_delay(true), ignored); // ?slg? do we want this?
      socket.set_option(asio::ip::tcp::socket::reuse_address(true), ignored);
      socket.bind(mLocalTcpEndpoint, errorCode);
   }
   return errorCode;
}

void
AsyncSocketBase::asyncConnect(asio::ip::tcp::socket& socket, const asio::ip::tcp::resolver::results_type& endpoints)
{
   asyncConnectFrom(socket, endpoints, endpoints.begin(), asio::error::host_not_found);
}

void
AsyncSocketBase::asyncConnectFrom(asio::ip::tcp::socket& socket, asio::ip::tcp::resolver::results_type endpoints,
                                  asio::ip::tcp::resolver::results_type::const_iterator next, asio::error_code lastError)
{
   if(next == endpoints.end())
   {
      handleConnect(lastError, asio::ip::tcp::endpoint());
      return;
   }

   // Each attempt on a new socket, bound to the local endpoint
   asio::error_code errorCode = openAndBind(socket);
   if(errorCode)
   {
      handleConnect(errorCode, asio::ip::tcp::endpoint());
      return;
   }

   const asio::ip::tcp::endpoint endpoint = *next;
   std::shared_ptr<AsyncSocketBase> self = shared_from_this();
   socket.async_connect(endpoint, [self, &socket, endpoints, next, endpoint](const asio::error_code& ec)
   {
      // Done if connected, or if the socket has been closed meanwhile; otherwise try the next endpoint
      if(!ec || ec == asio::error::operation_aborted)
      {
         self->handleConnect(ec, endpoint);
      }
      else
      {
         self->asyncConnectFrom(socket, endpoints, std::next(next), ec);
      }
   });
}

void
AsyncSocketBase::startIdleTimer(unsigned int timeoutSeconds, std::function<bool()> inUse)
{
   if(timeoutSeconds == 0)
   {
      return;
   }
   mIdleTimeout = std::chrono::seconds(timeoutSeconds);
   mInUse = std::move(inUse);
   mIdleTimer.reset(new asio::steady_timer(mIOService));
   noteReceived();
   waitIdle(mIdleTimeout);
}

void
AsyncSocketBase::stopIdleTimer()
{
   mInUse = nullptr;  // in case the timer has fired already, and its handler is queued
   if(mIdleTimer)
   {
      mIdleTimer->cancel();
   }
}

void
AsyncSocketBase::waitIdle(std::chrono::steady_clock::duration wait)
{
   // Rather than being set again for each message received, the timer is set for when the
   // connection would have been idle long enough, and then checks.  It holds a weak reference: it
   // doesn't keep a stopped connection alive.
   std::weak_ptr<AsyncSocketBase> weakSelf = shared_from_this();
   mIdleTimer->expires_after(wait);
   mIdleTimer->async_wait([weakSelf](const asio::error_code& e)
   {
      std::shared_ptr<AsyncSocketBase> self = weakSelf.lock();
      if(e == asio::error::operation_aborted || !self || !self->mInUse)
      {
         return;
      }
      const std::chrono::steady_clock::duration idle = std::chrono::steady_clock::now() - self->mLastReceived;
      if(self->mInUse())
      {
         self->waitIdle(self->mIdleTimeout);  // kept for as long as it's in use
      }
      else if(idle < self->mIdleTimeout)
      {
         self->waitIdle(self->mIdleTimeout - idle);
      }
      else
      {
         InfoLog(<< "Closing the connection from " << self->mConnectedAddress.to_string() << ":" << self->mConnectedPort
                 << ": nothing received for " << std::chrono::duration_cast<std::chrono::seconds>(idle).count() << " seconds");
         self->close();
      }
   });
}

} // namespace


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

