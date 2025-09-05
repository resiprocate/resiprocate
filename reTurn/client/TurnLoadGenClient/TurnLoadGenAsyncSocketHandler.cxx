
#include <iostream>
#include <string>
#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif

#include "TurnLoadGenAsyncSocketHandler.hxx"
#include "../TurnSocket.hxx"
#include <rutil/Logger.hxx>
#include <rutil/WinLeakCheck.hxx>

#ifdef BOOST_ASIO_HAS_STD_CHRONO
using namespace std::chrono;
#else
#include <chrono>
using namespace std::chrono;
#endif

using namespace reTurn;
using namespace std;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

#define LOG_PREFIX << "[" << mClientNum << "] "

resip::Data* g_Payload = NULL;

TurnLoadGenAsyncSocketHandler::TurnLoadGenAsyncSocketHandler(
   int clientNum, 
   asio::io_context& ioService, 
   const Data& localAddress, 
   const Data& turnServerAddress, 
   unsigned short turnServerPort, 
   unsigned short relayPort, 
   const ConfigParse& config) :
      mClientNum(clientNum),
      mTimer(ioService),
      mLocalAddress(asio::ip::make_address(localAddress.c_str())),
      mTurnServerAddress(turnServerAddress),
      mTurnServerPort(turnServerPort),
      mRelayPort(relayPort),
      mConfig(config),
      mNumSends(0),
      mNumSendFailures(0),
      mNumReceiveSuccesses(0),
      mNumReceiveFailures(0),
      mDelayBetweenClientStartsMs(config.getConfigInt("DelayBetweenClientStartsMs", 2000)),
      mAllocationTimeSecs(config.getConfigInt("AllocationTimeSecs", 60)),
      mAllocationLifetimeSecs(config.getConfigInt("AllocationLifetimeSecs", TurnSocket::UnspecifiedLifetime)),
      mTimeBetweenAllocationsSecs(config.getConfigInt("TimeBetweenAllocationsSecs", 2)),
      mPayloadIntervalMs(config.getConfigInt("PayloadIntervalMs", 20))
{
}

TurnLoadGenAsyncSocketHandler::~TurnLoadGenAsyncSocketHandler()
{
   InfoLog(LOG_PREFIX << "Destructing...");
   if (mTurnAsyncSocket)
   {
      mTurnAsyncSocket->close();
   }
}

void TurnLoadGenAsyncSocketHandler::setTurnAsyncSocket(std::shared_ptr<TurnAsyncSocket>& turnAsyncSocket)
{ 
   mTurnAsyncSocket = turnAsyncSocket;

   // Connect to TurnServer based on delay setting
   if (mDelayBetweenClientStartsMs > 0)
   {
      mTimer.expires_after(milliseconds(mDelayBetweenClientStartsMs * (mClientNum-1)));
      mTimer.async_wait(std::bind(&TurnLoadGenAsyncSocketHandler::connect, this));
   }
   else
   {
      connect();
   }
}

void TurnLoadGenAsyncSocketHandler::connect()
{
   InfoLog(LOG_PREFIX << "Connecting to turn server...");
   // Connect to Stun/Turn Server
   mTurnAsyncSocket->connect(mTurnServerAddress.c_str(), mTurnServerPort);
}

void TurnLoadGenAsyncSocketHandler::sendBindRequest()
{
   mTurnAsyncSocket->bindRequest();
}

void TurnLoadGenAsyncSocketHandler::sendAllocationRequest()
{
   mNumSends = 0;
   mNumSendFailures = 0;
   mNumReceiveSuccesses = 0;
   mNumReceiveFailures = 0;

   mTurnAsyncSocket->createAllocation(mAllocationLifetimeSecs,
      TurnSocket::UnspecifiedBandwidth,
      StunMessage::PropsPortPair,
      TurnAsyncSocket::UnspecifiedToken,
      StunTuple::UDP);
}

void TurnLoadGenAsyncSocketHandler::startSendingPayload()
{
   DebugLog(LOG_PREFIX << "Sending RTP payload...");
   mStartTime = time(0);
   sendPayload();
}

void TurnLoadGenAsyncSocketHandler::sendPayload()
{
   time_t secondsElapsed = time(0) - mStartTime;
   if (secondsElapsed < mAllocationTimeSecs)
   {
      mTimer.expires_after(milliseconds(mPayloadIntervalMs));
      mTimer.async_wait(std::bind(&TurnLoadGenAsyncSocketHandler::sendPayload, this));
      mTurnAsyncSocket->send(g_Payload->data(), g_Payload->size());
      ++mNumSends;
   }
   else
   {
      DebugLog(LOG_PREFIX << "Configured AllocationTimeSecs has expired, configured=" << mAllocationTimeSecs << ", secondsElapsed=" << secondsElapsed << " destroying allocation.");
      mTurnAsyncSocket->destroyAllocation();
   }
}

void TurnLoadGenAsyncSocketHandler::onConnectSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port)
{
   DebugLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onConnectSuccess: socketDest=" << socketDesc << ", address=" << address.to_string() << ", turnPort=" << port);

   if (mConfig.getConfigBool("SendInitialBind", true))
   {
      sendBindRequest();
   }
   else
   {
      sendAllocationRequest();
   }
}

void TurnLoadGenAsyncSocketHandler::onConnectFailure(unsigned int socketDesc, const asio::error_code& e)
{
   ErrLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onConnectFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
}

void TurnLoadGenAsyncSocketHandler::onSharedSecretSuccess(unsigned int socketDesc, const char* username, unsigned int usernameSize, const char* password, unsigned int passwordSize)
{
   InfoLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onSharedSecretSuccess: socketDest=" << socketDesc << ", username=" << username << ", password=" << password);
}

void TurnLoadGenAsyncSocketHandler::onSharedSecretFailure(unsigned int socketDesc, const asio::error_code& e)
{
   ErrLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onSharedSecretFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
}

void TurnLoadGenAsyncSocketHandler::onBindSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple, const StunTuple& stunServerTuple)
{
   InfoLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onBindingSuccess: socketDest=" << socketDesc << ", reflexive=" << reflexiveTuple << ", stunServerTuple=" << stunServerTuple);
   sendAllocationRequest();
}

void TurnLoadGenAsyncSocketHandler::onBindFailure(unsigned int socketDesc, const asio::error_code& e, const StunTuple& stunServerTuple)
{
   ErrLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onBindingFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << "), stunServerTuple=" << stunServerTuple);
}

void TurnLoadGenAsyncSocketHandler::onAllocationSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple, const StunTuple& relayTuple, unsigned int lifetimeSecs, unsigned int bandwidth, uint64_t reservationToken)
{
   DebugLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onAllocationSuccess: socketDest=" << socketDesc <<
      ", reflexive=" << reflexiveTuple <<
      ", relay=" << relayTuple <<
      ", lifetimeSecs=" << lifetimeSecs <<
      ", bandwidth=" << bandwidth <<
      ", reservationToken=" << reservationToken);

   DebugLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onAllocationSuccess: setting active destination to " << mLocalAddress << ":" << mRelayPort);
   mTurnAsyncSocket->setActiveDestination(mLocalAddress, mRelayPort);
}

void TurnLoadGenAsyncSocketHandler::onAllocationFailure(unsigned int socketDesc, const asio::error_code& e)
{
   ErrLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onAllocationFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");

   // Retry allocation after timer expires
   mTimer.expires_after(seconds(mTimeBetweenAllocationsSecs));
   mTimer.async_wait(std::bind(&TurnLoadGenAsyncSocketHandler::sendAllocationRequest, this));
}

void TurnLoadGenAsyncSocketHandler::onRefreshSuccess(unsigned int socketDesc, unsigned int lifetimeSecs)
{
   DebugLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onRefreshSuccess: socketDest=" << socketDesc << ", lifetimeSecs=" << lifetimeSecs);
   if (lifetimeSecs == 0)
   {
      InfoLog(LOG_PREFIX << "Allocation was up for " << (time(0) - mStartTime) << " seconds, numSends=" << mNumSends <<
         ", numReceiveSuccesses=" << mNumReceiveSuccesses <<
         ", numSendFailures=" << mNumSendFailures <<
         ", numReceiveFailures=" << mNumReceiveFailures <<
         ", creating new allocation in " << mTimeBetweenAllocationsSecs << " secs.");

      mTimer.expires_after(seconds(mTimeBetweenAllocationsSecs));
      mTimer.async_wait(std::bind(&TurnLoadGenAsyncSocketHandler::sendAllocationRequest, this));
   }
}

void TurnLoadGenAsyncSocketHandler::onRefreshFailure(unsigned int socketDesc, const asio::error_code& e)
{
   ErrLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onRefreshFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");

   // Retry allocation after timer expires
   mTimer.expires_after(seconds(mTimeBetweenAllocationsSecs));
   mTimer.async_wait(std::bind(&TurnLoadGenAsyncSocketHandler::sendAllocationRequest, this));
}

void TurnLoadGenAsyncSocketHandler::onSetActiveDestinationSuccess(unsigned int socketDesc)
{
   DebugLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onSetActiveDestinationSuccess: socketDest=" << socketDesc);
   startSendingPayload();
}

void TurnLoadGenAsyncSocketHandler::onSetActiveDestinationFailure(unsigned int socketDesc, const asio::error_code& e)
{
   ErrLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onSetActiveDestinationFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
}

void TurnLoadGenAsyncSocketHandler::onClearActiveDestinationSuccess(unsigned int socketDesc)
{
   InfoLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onClearActiveDestinationSuccess: socketDest=" << socketDesc);
}

void TurnLoadGenAsyncSocketHandler::onClearActiveDestinationFailure(unsigned int socketDesc, const asio::error_code& e)
{
   ErrLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onClearActiveDestinationFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
}

void TurnLoadGenAsyncSocketHandler::onChannelBindRequestSent(unsigned int socketDesc, unsigned short channelNumber)
{
   DebugLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onChannelBindRequestSent: socketDest=" << socketDesc << " channelNumber=" << channelNumber);
}

void TurnLoadGenAsyncSocketHandler::onChannelBindSuccess(unsigned int socketDesc, unsigned short channelNumber)
{
   DebugLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onChannelBindSuccess: socketDest=" << socketDesc << " channelNumber=" << channelNumber);
}

void TurnLoadGenAsyncSocketHandler::onChannelBindFailure(unsigned int socketDesc, const asio::error_code& e)
{
   ErrLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onChannelBindFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
}

void TurnLoadGenAsyncSocketHandler::onSendSuccess(unsigned int socketDesc)
{
   //InfoLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onSendSuccess: socketDest=" << socketDesc);
}

void TurnLoadGenAsyncSocketHandler::onSendFailure(unsigned int socketDesc, const asio::error_code& e)
{
   ErrLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onSendFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   ++mNumSendFailures;
}

void TurnLoadGenAsyncSocketHandler::onReceiveSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port, const std::shared_ptr<reTurn::DataBuffer>& data)
{
   //InfoLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onReceiveSuccess: socketDest=" << socketDesc << ", fromAddress=" << address << ", fromPort=" << turnPort << ", size=" << data->size() << ", data=" << data->data()); 
   ++mNumReceiveSuccesses;
}

void TurnLoadGenAsyncSocketHandler::onReceiveFailure(unsigned int socketDesc, const asio::error_code& e)
{
   ErrLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onReceiveFailure: socketDest=" << socketDesc << " error=" << e.value() << "(" << e.message() << ").");
   ++mNumReceiveFailures;
}

void TurnLoadGenAsyncSocketHandler::onIncomingBindRequestProcessed(unsigned int socketDesc, const StunTuple& sourceTuple)
{
   InfoLog(LOG_PREFIX << "MyTurnAsyncSocketHandler::onIncomingBindRequestProcessed: socketDest=" << socketDesc << " sourceTuple=" << sourceTuple);
}


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
