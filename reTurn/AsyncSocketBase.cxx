#include "AsyncSocketBase.hxx"
#include "AsyncSocketBaseHandler.hxx"
#include <rutil/WinLeakCheck.hxx>
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;

#define NO_CHANNEL ((unsigned short)-1)

namespace reTurn {

AsyncSocketBase::AsyncSocketBase(asio::io_service& ioService) : 
  mIOService(ioService),
  mReceiving(false),
  mConnected(false),
  mAsyncSocketBaseHandler(0)
{
}

AsyncSocketBase::~AsyncSocketBase()
{
   if(mAsyncSocketBaseHandler) mAsyncSocketBaseHandler->onSocketDestroyed();
}

void 
AsyncSocketBase::send(const StunTuple& destination, boost::shared_ptr<DataBuffer>& data)
{
   mIOService.dispatch(boost::bind(&AsyncSocketBase::doSend, shared_from_this(), destination, data, 0));
}

void 
AsyncSocketBase::send(const StunTuple& destination, unsigned short channel, boost::shared_ptr<DataBuffer>& data)
{
   mIOService.post(boost::bind(&AsyncSocketBase::doSend, shared_from_this(), destination, channel, data, 0));
}

void
AsyncSocketBase::doSend(const StunTuple& destination, boost::shared_ptr<DataBuffer>& data, unsigned int bufferStartPos)
{
   doSend(destination, NO_CHANNEL, data, bufferStartPos);
}

void
AsyncSocketBase::doSend(const StunTuple& destination, unsigned short channel, boost::shared_ptr<DataBuffer>& data, unsigned int bufferStartPos)
{
   bool writeInProgress = !mSendDataQueue.empty();
   if(channel == NO_CHANNEL)
   {
      boost::shared_ptr<DataBuffer> empty;
      mSendDataQueue.push_back(SendData(destination, empty, data, bufferStartPos));
   }
   else
   {
      // Add Turn Framing
      boost::shared_ptr<DataBuffer> frame = allocateBuffer(4);
      channel = htons(channel);
      memcpy(&(*frame)[0], &channel, 2);
      unsigned short msgsize = htons((unsigned short)data->size());
      memcpy(&(*frame)[2], (void*)&msgsize, 2);  // UDP doesn't need size - but shouldn't hurt to send it anyway

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
   if(mSendDataQueue.front().mFrameData.get() != 0) // If we have frame data
   {
      bufs.push_back(asio::buffer(mSendDataQueue.front().mFrameData->data(), mSendDataQueue.front().mFrameData->size()));
   }
   bufs.push_back(asio::buffer(mSendDataQueue.front().mData->data()+mSendDataQueue.front().mBufferStartPos, mSendDataQueue.front().mData->size()-mSendDataQueue.front().mBufferStartPos));
   transportSend(mSendDataQueue.front().mDestination, bufs);
}

void 
AsyncSocketBase::receive()
{
   mIOService.post(boost::bind(&AsyncSocketBase::doReceive, shared_from_this()));
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
   mIOService.post(boost::bind(&AsyncSocketBase::doFramedReceive, shared_from_this()));
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
AsyncSocketBase::handleReceive(const asio::error_code& e, std::size_t bytesTransferred)
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
   mIOService.post(boost::bind(&AsyncSocketBase::transportClose, shared_from_this()));
}

boost::shared_ptr<DataBuffer>  
AsyncSocketBase::allocateBuffer(unsigned int size)
{
   return boost::shared_ptr<DataBuffer>(new DataBuffer(size));
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

