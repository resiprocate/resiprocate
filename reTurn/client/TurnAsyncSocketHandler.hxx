#ifndef TURNASYNCSOCKETHANDLER_HXX
#define TURNASYNCSOCKETHANDLER_HXX

#include <asio.hpp>
#include "../AsyncSocketBaseHandler.hxx"
#include "../StunTuple.hxx"

namespace reTurn {

class TurnAsyncSocketHandler
{
public:
   TurnAsyncSocketHandler();
   virtual ~TurnAsyncSocketHandler();

   virtual void onConnectSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port) = 0;
   virtual void onConnectFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

   virtual void onSharedSecretSuccess(unsigned int socketDesc, const char* username, unsigned int usernameSize, const char* password, unsigned int passwordSize) = 0;  
   virtual void onSharedSecretFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

   virtual void onBindSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple) = 0; 
   virtual void onBindFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

   virtual void onAllocationSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple, const StunTuple& relayTuple, unsigned int lifetime, unsigned int bandwidth) = 0; 
   virtual void onAllocationFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

   virtual void onRefreshSuccess(unsigned int socketDesc, unsigned int lifetime) = 0;
   virtual void onRefreshFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

   virtual void onSetActiveDestinationSuccess(unsigned int socketDesc) = 0;
   virtual void onSetActiveDestinationFailure(unsigned int socketDesc, const asio::error_code &e) = 0;
   virtual void onClearActiveDestinationSuccess(unsigned int socketDesc) = 0;
   virtual void onClearActiveDestinationFailure(unsigned int socketDesc, const asio::error_code &e) = 0;

   virtual void onReceiveSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port, const char* buffer, unsigned int size) = 0;
   virtual void onReceiveFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

   virtual void onSendSuccess(unsigned int socketDesc) = 0;
   virtual void onSendFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

private:
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

