#ifndef TURNASYNCSOCKET_HXX
#define TURNASYNCSOCKET_HXX

#if defined(BOOST_MSVC) && (BOOST_MSVC >= 1400) \
  && (!defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0600) \
  && !defined(ASIO_ENABLE_CANCELIO)
#error You must define ASIO_ENABLE_CANCELIO in your build settings.
#endif

#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <rutil/Data.hxx>
#include <rutil/Mutex.hxx>

#include <map>
#include <queue>

#include "reTurn/StunTuple.hxx"
#include "reTurn/StunMessage.hxx"
#include "reTurn/ChannelManager.hxx"
#include "reTurn/AsyncSocketBase.hxx"
#include "TurnAsyncSocketHandler.hxx"

#define UDP_RT0 100  // RTO - Estimate of Roundtrip time - 100ms is recommened for fixed line transport - the initial value should be configurable
                     // Should also be calculation this on the fly
#define UDP_MAX_RETRANSMITS    7       // Defined by RFC5389 (Rc) - should be configurable
#define TCP_RESPONSE_TIME      39500   // Defined by RFC5389 (Ti) - should be configurable
#define UDP_Rm                 16      // Defined by RFC5389 - should be configurable
#define UDP_FINAL_REQUEST_TIME (UDP_RT0 * UDP_Rm)  // Defined by RFC5389
#define DEFAULT_RETRANS_INTERVAL_MS 0 // special value, indicates that exponential backoff will be used

namespace reTurn {

class TurnAsyncSocket
{
public:
   static unsigned int UnspecifiedLifetime;
   static unsigned int UnspecifiedBandwidth;
   static unsigned short UnspecifiedToken;
   static asio::ip::address UnspecifiedIpAddress;

   explicit TurnAsyncSocket(asio::io_service& ioService,
                            AsyncSocketBase& asyncSocketBase,
                            TurnAsyncSocketHandler* turnAsyncSocketHandler,
                            const asio::ip::address& address = UnspecifiedIpAddress, 
                            unsigned short port = 0);
   virtual ~TurnAsyncSocket();

   virtual void disableTurnAsyncHandler();
   virtual unsigned int getSocketDescriptor() = 0;

   // Note: Shared Secret requests have been deprecated by RFC5389, and not 
   //       widely implemented in RFC3489 - so not really needed at all
   void requestSharedSecret();

   // Set the username and password for all future requests
   void setUsernameAndPassword(const char* username, const char* password, bool shortTermAuth=false);

   // Sets the local HmacKey, used to check the integrity of incoming STUN messages
   void setLocalPassword(const char* password);

   void connect(const std::string& address, unsigned short port);

   // Stun Binding Method - use getReflexiveTuple() to get binding info
   void bindRequest();

   // ICE connectivity check
   void connectivityCheck(const StunTuple& targetAddr, UInt32 peerRflxPriority, bool setIceControlling, bool setIceControlled, unsigned int numRetransmits, unsigned int retrans_iterval_ms);

   // Turn Allocation Methods
   void createAllocation(unsigned int lifetime = UnspecifiedLifetime,
                         unsigned int bandwidth = UnspecifiedBandwidth,
                         unsigned char requestedPortProps = StunMessage::PropsNone, 
                         UInt64 reservationToken = UnspecifiedToken,
                         StunTuple::TransportType requestedTransportType = StunTuple::None);
   void refreshAllocation(unsigned int lifetime);
   void destroyAllocation();

   // Methods to control active destination
   void setActiveDestination(const asio::ip::address& address, unsigned short port);
   void clearActiveDestination();

   asio::ip::address& getConnectedAddress() { return mAsyncSocketBase.getConnectedAddress(); }
   unsigned short getConnectedPort() { return mAsyncSocketBase.getConnectedPort(); }

   // Turn Send Methods
   virtual void send(const char* buffer, unsigned int size);  // framed
   virtual void sendTo(const asio::ip::address& address, unsigned short port, const char* buffer, unsigned int size);  // framed

   virtual void sendUnframed(boost::shared_ptr<DataBuffer>& data);  // Send unframed data
   virtual void sendFramed(boost::shared_ptr<DataBuffer>& data);
   virtual void sendToUnframed(const asio::ip::address& address, unsigned short port, boost::shared_ptr<DataBuffer>& data);
   virtual void sendToFramed(const asio::ip::address& address, unsigned short port, boost::shared_ptr<DataBuffer>& data);

   // Receive Methods
   //asio::error_code receive(char* buffer, unsigned int& size, unsigned int timeout, asio::ip::address* sourceAddress=0, unsigned short* sourcePort=0);
   //asio::error_code receiveFrom(const asio::ip::address& address, unsigned short port, char* buffer, unsigned int& size, unsigned int timeout);

   virtual void close();
   virtual void turnReceive();

   virtual void setOnBeforeSocketClosedFp(boost::function<void(unsigned int)> fp);

protected:

   void handleReceivedData(const asio::ip::address& address, unsigned short port, boost::shared_ptr<DataBuffer>& data);

   asio::io_service& mIOService;
   TurnAsyncSocketHandler* mTurnAsyncSocketHandler;

   // Local Binding Info
   StunTuple mLocalBinding;

   // Authentication Info
   resip::Data mUsername;
   resip::Data mPassword;
   resip::Data mHmacKey;
   resip::Data mRealm;
   resip::Data mNonce;

   // Used to check integrity of incoming STUN messages
   resip::Data mLocalHmacKey;

   // Turn Allocation Properties used in request
   StunTuple::TransportType mRequestedTransportType;

   // Turn Allocation Properties from response
   bool mHaveAllocation;
   StunTuple::TransportType mRelayTransportType;
   unsigned int mLifetime;

   ChannelManager mChannelManager;
   RemotePeer* mActiveDestination;

private:
   AsyncSocketBase& mAsyncSocketBase;
   bool mCloseAfterDestroyAllocationFinishes;

   // Request map (for retransmissions)
   class RequestEntry : public boost::enable_shared_from_this<RequestEntry>
   {
   public:
      RequestEntry(asio::io_service& ioService, TurnAsyncSocket* turnAsyncSocket, StunMessage* requestMessage, unsigned int rc, unsigned int retrans_iterval_ms, const StunTuple* dest=NULL);
      ~RequestEntry();

      void startTimer();
      void stopTimer();
      void requestTimerExpired(const asio::error_code& e);

      asio::io_service& mIOService;
      TurnAsyncSocket* mTurnAsyncSocket;
      StunMessage* mRequestMessage;
      asio::deadline_timer mRequestTimer;
      unsigned int mRequestsSent;
      unsigned int mTimeout;
      const StunTuple* mDest;
      const unsigned int mRc;
      const unsigned int mRetransIntervalMs;
   };
   typedef std::map<UInt128, boost::shared_ptr<RequestEntry> > RequestMap;
   RequestMap mActiveRequestMap;
   friend class RequestEntry;
   void requestTimeout(UInt128 tid);
   void clearActiveRequestMap();

   // weak functor template, used to reference the parent without explicitly
   // preventing it from being garbage collected. Functor will only call the
   // parent if it is still available.
   template < typename P, typename F > class weak_bind
   {
   public:
      // !jjg! WARNING! if you are using boost::bind(..) to create the second
      // argument for this constructor BE CAREFUL that you are passing 'this' and
      // not 'shared_from_this()' to the bind(..) -- otherwise you will defeat the
      // purpose of this class holding a weak_ptr
      weak_bind<P,F>( boost::weak_ptr<P> parent, boost::function<F> func )
         : mParent( parent ), mFunction( func ) {}

      void operator()()
      {
         if ( boost::shared_ptr< P > ptr = mParent.lock() )
         {
            if ( !mFunction.empty() )
               mFunction();
         }
      }

      void operator()(const asio::error_code& e)
      {
         if ( boost::shared_ptr< P > ptr = mParent.lock() )
         {
            if ( !mFunction.empty() )
               mFunction(e);
         }
      }

   private:
      boost::weak_ptr< P > mParent;
      boost::function< F > mFunction;
   };

   asio::deadline_timer mAllocationTimer;
   void startAllocationTimer();
   void cancelAllocationTimer();
   void allocationTimerExpired(const asio::error_code& e);

   typedef std::map<unsigned short, asio::deadline_timer*> ChannelBindingTimerMap;
   ChannelBindingTimerMap mChannelBindingTimers;
   void startChannelBindingTimer(unsigned short channel);
   void cancelChannelBindingTimers();
   void channelBindingTimerExpired(const asio::error_code& e, unsigned short channel);

   void doRequestSharedSecret();
   void doSetUsernameAndPassword(resip::Data* username, resip::Data* password, bool shortTermAuth);
   void doSetLocalPassword(resip::Data* password);
   void doBindRequest();
   void doConnectivityCheck(StunTuple* targetAddr, UInt32 peerRflxPriority, bool setIceControlling, bool setIceControlled, unsigned int numRetransmits, unsigned int retrans_iterval_ms);
   void doCreateAllocation(unsigned int lifetime = UnspecifiedLifetime,
                           unsigned int bandwidth = UnspecifiedBandwidth,
                           unsigned char requestedPortProps = StunMessage::PropsNone, 
                           UInt64 reservationToken = 0,
                           StunTuple::TransportType requestedTransportType = StunTuple::None);
   void doRefreshAllocation(unsigned int lifetime);
   void doDestroyAllocation();
   void doSetActiveDestination(const asio::ip::address& address, unsigned short port);
   void doClearActiveDestination();
   void doSendFramed(boost::shared_ptr<DataBuffer>& data);
   void doSendToFramed(const asio::ip::address& address, unsigned short port, boost::shared_ptr<DataBuffer>& data);
   void doClose();
   void actualClose();
   void doChannelBinding(RemotePeer& remotePeer);

   StunMessage* createNewStunMessage(UInt16 stunclass, UInt16 method, bool addAuthInfo=true);
   void sendStunMessage(StunMessage* request, bool reTransmission=false, unsigned int numRetransmits=UDP_MAX_RETRANSMITS, unsigned int retrans_iterval_ms=DEFAULT_RETRANS_INTERVAL_MS, const StunTuple* targetAddress=NULL);
   void sendToRemotePeer(RemotePeer& remotePeer, boost::shared_ptr<DataBuffer>& data);
   void sendOverChannel(unsigned short channel, boost::shared_ptr<DataBuffer>& data);  // send with turn framing

   asio::error_code handleStunMessage(StunMessage& stunMessage);
   asio::error_code handleDataInd(StunMessage& stunMessage);
   asio::error_code handleChannelBindResponse(StunMessage &request, StunMessage &response);
   asio::error_code handleSharedSecretResponse(StunMessage &request, StunMessage &response);
   asio::error_code handleBindRequest(StunMessage& stunMessage);
   asio::error_code handleBindResponse(StunMessage &request, StunMessage &response);
   asio::error_code handleAllocateResponse(StunMessage &request, StunMessage &response);
   asio::error_code handleRefreshResponse(StunMessage &request, StunMessage &response);
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
