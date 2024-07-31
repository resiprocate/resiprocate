#ifndef TURNLOADGENASYNCSOCKETHANDLER_HXX
#define TURNLOADGENASYNCSOCKETHANDLER_HXX

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "../TurnAsyncSocket.hxx"
#include "../TurnAsyncSocketHandler.hxx"
#include <rutil/ConfigParse.hxx>

using namespace reTurn;
using namespace std;
using namespace resip;

class TurnLoadGenAsyncSocketHandler : public TurnAsyncSocketHandler
{
public:
   TurnLoadGenAsyncSocketHandler(
      int clientNum, 
      asio::io_service& ioService, 
      const Data& localAddress, 
      const Data& turnServerAddress, 
      unsigned short turnServerPort, 
      unsigned short relayPort, 
      const ConfigParse& config);
   virtual ~TurnLoadGenAsyncSocketHandler();

   void setTurnAsyncSocket(std::shared_ptr<TurnAsyncSocket>& turnAsyncSocket);
   void connect();
   void sendBindRequest();
   void sendAllocationRequest();
   void startSendingPayload();
   void sendPayload();

   virtual void onConnectSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port) override;
   virtual void onConnectFailure(unsigned int socketDesc, const asio::error_code& e) override;
   virtual void onSharedSecretSuccess(unsigned int socketDesc, const char* username, unsigned int usernameSize, const char* password, unsigned int passwordSize) override;
   virtual void onSharedSecretFailure(unsigned int socketDesc, const asio::error_code& e) override;
   virtual void onBindSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple, const StunTuple& stunServerTuple) override;
   virtual void onBindFailure(unsigned int socketDesc, const asio::error_code& e, const StunTuple& stunServerTuple) override;
   virtual void onAllocationSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple, const StunTuple& relayTuple, unsigned int lifetime, unsigned int bandwidth, uint64_t reservationToken) override;
   virtual void onAllocationFailure(unsigned int socketDesc, const asio::error_code& e) override;
   virtual void onRefreshSuccess(unsigned int socketDesc, unsigned int lifetime) override;
   virtual void onRefreshFailure(unsigned int socketDesc, const asio::error_code& e) override;
   virtual void onSetActiveDestinationSuccess(unsigned int socketDesc) override;
   virtual void onSetActiveDestinationFailure(unsigned int socketDesc, const asio::error_code& e) override;
   virtual void onClearActiveDestinationSuccess(unsigned int socketDesc) override;
   virtual void onClearActiveDestinationFailure(unsigned int socketDesc, const asio::error_code& e) override;
   virtual void onChannelBindRequestSent(unsigned int socketDesc, unsigned short channelNumber) override;
   virtual void onChannelBindSuccess(unsigned int socketDesc, unsigned short channelNumber) override;
   virtual void onChannelBindFailure(unsigned int socketDesc, const asio::error_code& e) override;
   virtual void onSendSuccess(unsigned int socketDesc) override;
   virtual void onSendFailure(unsigned int socketDesc, const asio::error_code& e) override;
   virtual void onReceiveSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port, const std::shared_ptr<reTurn::DataBuffer>& data) override;
   virtual void onReceiveFailure(unsigned int socketDesc, const asio::error_code& e) override;
   virtual void onIncomingBindRequestProcessed(unsigned int socketDesc, const StunTuple& sourceTuple) override;

private:
   int mClientNum;
   asio::steady_timer mTimer;
   asio::ip::address mLocalAddress;
   resip::Data mTurnServerAddress;
   unsigned short mTurnServerPort;
   unsigned short mRelayPort;
   const ConfigParse& mConfig;
   std::shared_ptr<TurnAsyncSocket> mTurnAsyncSocket;
   unsigned int mNumSends;
   unsigned int mNumSendFailures;
   unsigned int mNumReceiveSuccesses;
   unsigned int mNumReceiveFailures;
   time_t mStartTime;

   // Config settings
   int mDelayBetweenClientStartsMs;
   int mAllocationTimeSecs;
   int mAllocationLifetimeSecs;
   int mTimeBetweenAllocationsSecs;
   int mPayloadIntervalMs;
};


#endif


/* ====================================================================

 Copyright (c) 2024 SIP Spectrum, Inc http://www.sipspectrum.com
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
