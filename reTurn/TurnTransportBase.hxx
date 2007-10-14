#ifndef TURN_TRANSPORT_BASE_HXX
#define TURN_TRANSPORT_BASE_HXX

#include <deque>
#include <asio.hpp>
#include <rutil/Data.hxx>

#include "StunTuple.hxx"

namespace reTurn {

class TurnTransportHandler;

class TurnTransportBase
{
public:
   TurnTransportBase(asio::io_service& ioService);
   virtual ~TurnTransportBase();

   // Note: destination is ignored for TCP and TLS connections
   virtual void sendTurnData(const StunTuple& destination, const char* buffer, unsigned int size);

   /// Handle completion of a sendData operation.
   virtual void handleSendData(const asio::error_code& e);

  /// Register a TurnTransportHandler to receive transport destroyed noticiation
  /// Note:  This is really only useful for TCP/TLS transports/connections
  void registerTurnTransportHandler(TurnTransportHandler *turnTransportHandler);

protected:
   /// The io_service used to perform asynchronous operations.
   asio::io_service& mIOService;

private:
   virtual void sendData(const StunTuple& destination, const char* buffer, unsigned int size) = 0;

   class DataToSend
   {
   public:
      DataToSend(const StunTuple& destination, const resip::Data& data) : mDestination(destination), mData(data) {}
      StunTuple mDestination;  // only used for UDP
      resip::Data mData;
   };
   /// Queue of data to send
   typedef std::deque<DataToSend> TurnDataQueue;
   TurnDataQueue mTurnDataQueue;

  /// Registered TurnTransportHandler
  TurnTransportHandler *mTurnTransportHandler;
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

