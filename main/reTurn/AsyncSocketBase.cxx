#include "AsyncSocketBase.hxx"
#include "AsyncSocketBaseHandler.hxx"
#include <boost/bind.hpp>

using namespace std;
using namespace resip;

#define NO_CHANNEL ((unsigned short)-1)

namespace reTurn {

AsyncSocketBase::AsyncSocketBase(asio::io_service& ioService) : 
  mIOService(ioService),
  mReceiving(false),
  mAsyncSocketBaseHandler(0),
  mAsyncSocketBaseDestroyedHandler(0)
{
}

AsyncSocketBase::~AsyncSocketBase()
{
   if(mAsyncSocketBaseDestroyedHandler) mAsyncSocketBaseDestroyedHandler->onSocketDestroyed();
}

void 
AsyncSocketBase::send(const StunTuple& destination, resip::SharedPtr<Data> data)
{
   mIOService.post(boost::bind(&AsyncSocketBase::doSend, this, destination, data, 0));
}

void 
AsyncSocketBase::send(const StunTuple& destination, unsigned short channel, resip::SharedPtr<Data> data)
{
   mIOService.post(boost::bind(&AsyncSocketBase::doSend, this, destination, channel, data, 0));
}

void
AsyncSocketBase::doSend(const StunTuple& destination, resip::SharedPtr<Data> data, unsigned int bufferStartPos)
{
   doSend(destination, NO_CHANNEL, data, bufferStartPos);
}

void
AsyncSocketBase::doSend(const StunTuple& destination, unsigned short channel, resip::SharedPtr<Data> data, unsigned int bufferStartPos)
{
   bool writeInProgress = !mSendDataQueue.empty();
   if(channel == NO_CHANNEL)
   {
      resip::SharedPtr<resip::Data> empty;
      mSendDataQueue.push_back(SendData(destination, empty, data, bufferStartPos));
   }
   else
   {
      // Add Turn Framing
      resip::SharedPtr<resip::Data> frame = allocateBuffer(4);
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
      mSendDataQueue.pop_front();
      if (!mSendDataQueue.empty())
      {
         sendFirstQueuedData();
      }
      if(mAsyncSocketBaseHandler) mAsyncSocketBaseHandler->onSendSuccess(getSocketDescriptor());
   }
   else
   {
      if(mAsyncSocketBaseHandler) mAsyncSocketBaseHandler->onSendFailure(getSocketDescriptor(), e);
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
   mIOService.post(boost::bind(&AsyncSocketBase::doReceive, this));
}

void
AsyncSocketBase::doReceive()
{
   assert(!mReceiving);
   mReceiving=true;
   mReceiveBuffer = allocateBuffer(RECEIVE_BUFFER_SIZE);
   transportReceive();
}

void 
AsyncSocketBase::framedReceive()
{
   mIOService.post(boost::bind(&AsyncSocketBase::doFramedReceive, this));
}

void
AsyncSocketBase::doFramedReceive()
{
   assert(!mReceiving);
   mReceiving=true;
   mReceiveBuffer = allocateBuffer(RECEIVE_BUFFER_SIZE);
   transportFramedReceive();
}

void 
AsyncSocketBase::handleReceive(const asio::error_code& e, std::size_t bytesTransferred)
{
   mReceiving = false;

   if(!e)
   {
      // Handoff received buffer to appliction, and prepare receive buffer for next call
      mReceiveBuffer->truncate(bytesTransferred);
      if(mAsyncSocketBaseHandler) mAsyncSocketBaseHandler->onReceiveSuccess(getSocketDescriptor(), getSenderEndpointAddress(), getSenderEndpointPort(), mReceiveBuffer);
   }
   else
   {
      if(mAsyncSocketBaseHandler) mAsyncSocketBaseHandler->onReceiveFailure(getSocketDescriptor(), e);
   }
}

void 
AsyncSocketBase::close()
{
   mIOService.post(boost::bind(&AsyncSocketBase::transportClose, this));
}

resip::SharedPtr<resip::Data> 
AsyncSocketBase::allocateBuffer(unsigned int size)
{
   char* buf = new char[size];
   return resip::SharedPtr<resip::Data>(new resip::Data(resip::Data::Take, buf, size));
}

} // namespace


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

