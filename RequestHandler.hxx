#ifndef REQUEST_HANDLER_HXX
#define REQUEST_HANDLER_HXX

#include <string>
#include <boost/noncopyable.hpp>

#include "StunMessage.hxx"
#include "TurnManager.hxx"

namespace reTurn {

class TurnTransportBase;

/// The common handler for all incoming requests.
class RequestHandler
  : private boost::noncopyable
{
public:
   explicit RequestHandler(TurnManager& turnManager, 
                           const asio::ip::address* prim3489Address = 0, unsigned short* prim3489Port = 0,
                           const asio::ip::address* alt3489Address = 0, unsigned short* alt3489Port = 0);

   typedef enum
   {
      NoAuthentication,
      ShortTermPassword,
      LongTermPassword
   } AuthenticationMode;

   typedef enum
   {
      NoResponseToSend,
      RespondFromReceiving,
      RespondFromAlternatePort,
      RespondFromAlternateIp,
      RespondFromAlternateIpPort
   } ProcessResult;

   /// Process a received StunMessage, and produce a reply
   /// Returns true if the response message is to be sent
   ProcessResult processStunMessage(TurnTransportBase* turnTransport, StunMessage& request, StunMessage& response);
   void processTurnData(unsigned char channelNumber, const StunTuple& localTuple, const StunTuple& remoteTuple, const char* data, unsigned int size);

private:

   TurnManager& mTurnManager;

   // RFC3489 Server List
   bool mRFC3489SupportEnabled;
   asio::ip::address mPrim3489Address;
   unsigned short mPrim3489Port;
   asio::ip::address mAlt3489Address;
   unsigned short mAlt3489Port;

   bool handleAuthentication(StunMessage& request, StunMessage& response);

   // Specific request processors
   ProcessResult processStunBindingRequest(StunMessage& request, StunMessage& response);
   ProcessResult processStunSharedSecretRequest(StunMessage& request, StunMessage& response);
   ProcessResult processTurnAllocateRequest(TurnTransportBase* turnTransport, StunMessage& request, StunMessage& response);
   ProcessResult processTurnListenPermissionRequest(StunMessage& request, StunMessage& response);

   // Specific Indication processors
   void processTurnSendIndication(StunMessage& request);
   void processTurnChannelConfirmationIndication(StunMessage& request);

   // Utility methods
   void buildErrorResponse(StunMessage& response, unsigned short errorCode, const char* msg, const char* realm = 0);
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

