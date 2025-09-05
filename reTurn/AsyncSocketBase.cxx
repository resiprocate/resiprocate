#include "AsyncSocketBase.hxx"
#include "AsyncSocketBaseHandler.hxx"
#include <rutil/WinLeakCheck.hxx>
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#include <functional>

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
      // TODO !SLG! - if sending over TCP/TLS then message must be padded to be on a 4 byte boundary
      mSendDataQueue.push_back(SendData(destination, frame, data, bufferStartPos));
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

} // namespace


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

