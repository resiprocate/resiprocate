#include "TurnTransportBase.hxx"
#include "TurnTransportHandler.hxx"

using namespace std;
using namespace resip;

namespace reTurn {

TurnTransportBase::DataToSend::DataToSend(unsigned char channelNumber, const StunTuple& destination, const char* data, unsigned int length) : 
         mDestination(destination), mData(length+4, Data::Preallocate) 
{
   // Add Turn Framing
   mData[0] = channelNumber; 
   mData[1] = 0; 
   unsigned short size = htons((unsigned short)length);
   memcpy(&mData[2], (void*)&size, 2);  // UDP doesn't need size - but shouldn't hurt to send it anyway
   memcpy(&mData[4], data, length); 
}

TurnTransportBase::TurnTransportBase(asio::io_service& ioService) : 
  mIOService(ioService),
  mTurnTransportHandler(0)
{
}

TurnTransportBase::~TurnTransportBase()
{
   if(mTurnTransportHandler)
   {
      mTurnTransportHandler->onTransportDestroyed();
   }
}

void 
TurnTransportBase::sendTurnData(const StunTuple& destination, const char* buffer, unsigned int size)
{
   bool writeInProgress = !mTurnDataQueue.empty();
   mTurnDataQueue.push_back(DataToSend(destination, buffer, size));  // Copies Data
   if (!writeInProgress)
   {
      sendData(mTurnDataQueue.front().mDestination, mTurnDataQueue.front().mData.data(), (unsigned int)mTurnDataQueue.front().mData.size());
   }
}

void 
TurnTransportBase::sendTurnFramedData(unsigned char channelNumber, const StunTuple& destination, const char* buffer, unsigned int size)
{
   bool writeInProgress = !mTurnDataQueue.empty();
   mTurnDataQueue.push_back(DataToSend(channelNumber, destination, buffer, size));  // Copies Data
   if (!writeInProgress)
   {
      sendData(mTurnDataQueue.front().mDestination, mTurnDataQueue.front().mData.data(), (unsigned int)mTurnDataQueue.front().mData.size());
   }
}

void 
TurnTransportBase::handleSendData(const asio::error_code& e)
{
   if(!e)
   {
      mTurnDataQueue.pop_front();
      if (!mTurnDataQueue.empty())
      {
         sendData(mTurnDataQueue.front().mDestination, mTurnDataQueue.front().mData.data(), (unsigned int)mTurnDataQueue.front().mData.size());
      }
   }
}

void 
TurnTransportBase::registerTurnTransportHandler(TurnTransportHandler *turnTransportHandler)
{
   mTurnTransportHandler = turnTransportHandler;
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

