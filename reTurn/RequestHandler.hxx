#ifndef REQUEST_HANDLER_HXX
#define REQUEST_HANDLER_HXX

#include <string>
#include <boost/noncopyable.hpp>

#include "DataBuffer.hxx"
#include "StunMessage.hxx"
#include "TurnManager.hxx"
#include "TurnAllocationManager.hxx"

namespace reTurn {

class AsyncSocketBase;

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
      NoResponseToSend,
      RespondFromReceiving,
      RespondFromAlternatePort,
      RespondFromAlternateIp,
      RespondFromAlternateIpPort
   } ProcessResult;

   /// Process a received StunMessage, and produce a reply
   /// Returns true if the response message is to be sent
   ProcessResult processStunMessage(AsyncSocketBase* turnSocket, TurnAllocationManager& turnAllocationManager, StunMessage& request, StunMessage& response, bool isRFC3489BackwardsCompatServer=false);
   void processTurnData(TurnAllocationManager& turnAllocationManager, unsigned short channelNumber, const StunTuple& localTuple, const StunTuple& remoteTuple, boost::shared_ptr<DataBuffer>& data);

   const ReTurnConfig& getConfig() { return mTurnManager.getConfig(); }

private:

   TurnManager& mTurnManager;

   // RFC3489 Server List
   bool mRFC3489SupportEnabled;
   asio::ip::address mPrim3489Address;
   unsigned short mPrim3489Port;
   asio::ip::address mAlt3489Address;
   unsigned short mAlt3489Port;

   resip::Data mPrivateNonceKey;

   // Authentication handler
   bool handleAuthentication(StunMessage& request, StunMessage& response);

   // Specific request processors
   ProcessResult processStunBindingRequest(StunMessage& request, StunMessage& response, bool isRFC3489BackwardsCompatServer);
   ProcessResult processStunSharedSecretRequest(StunMessage& request, StunMessage& response);
   ProcessResult processTurnAllocateRequest(AsyncSocketBase* turnSocket, TurnAllocationManager& turnAllocationManager, StunMessage& request, StunMessage& response);
   ProcessResult processTurnRefreshRequest(TurnAllocationManager& turnAllocationManager, StunMessage& request, StunMessage& response);
   ProcessResult processTurnCreatePermissionRequest(TurnAllocationManager& turnAllocationManager, StunMessage& request, StunMessage& response);
   ProcessResult processTurnChannelBindRequest(TurnAllocationManager& turnAllocationManager, StunMessage& request, StunMessage& response);

   // Specific Indication processors
   void processTurnSendIndication(TurnAllocationManager& turnAllocationManager, StunMessage& request);

   // Utility methods
   void buildErrorResponse(StunMessage& response, unsigned short errorCode, const char* msg, const char* realm = 0);
   void generateNonce(const resip::Data& timestamp, resip::Data& nonce);
   enum CheckNonceResult { Valid, NotValid, Expired };
   CheckNonceResult checkNonce(const resip::Data& nonce);
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
