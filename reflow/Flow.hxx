#if !defined(Flow_hxx)
#define Flow_hxx

#include "client/TurnAsyncUdpSocket.hxx"
#include "client/TurnAsyncTcpSocket.hxx"
#include "client/TurnAsyncTlsSocket.hxx"
#include "client/TurnAsyncSocketHandler.hxx"
#include "StunMessage.hxx"
#include "FakeSelectSocketDescriptor.hxx"
#include "dtls_wrapper/DtlsSocket.hxx"

#include <map>
#include <rutil/TimeLimitFifo.hxx>
#include <rutil/Mutex.hxx>

#include <srtp.h>

using namespace reTurn;

namespace flowmanager
{

/**
  This class represents a Flow that is created by the Flow Manager.  A flow is a
  bi-directional stream of data for communicating with an endpoint, that may use
  UDP, TCP or TLS over TCP.  A flow may also use a Turn Allocation to transmit
  data to/from an endpoint.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/
class MediaStream;
class Flow;

class Flow : public TurnAsyncSocketHandler
{
public:

   enum FlowState
   {
      Unconnected,
      ConnectingServer,
      Connecting,
      Binding,
      Allocating,
      Connected,
      Ready
   };

   Flow(asio::io_service& ioService,
        asio::ssl::context& sslContext,
        unsigned int componentId,
        const StunTuple& localBinding, 
        MediaStream& mediaStream);
   ~Flow();

   void activateFlow(UInt8 allocationPortProps = StunMessage::PortPropsNone);
   void activateFlow(UInt8 allocationPortProps,
                     const StunTuple& requestedAllocationTuple);

   bool isReady() { return mFlowState == Ready; }

   /// Returns a socket descriptor that can be used in a select call
   /// WARNING - this descriptor should not be used for any other purpose
   ///         - do NOT set socket options, or send, receive from this descriptor,
   ///           instead use the Flow api's
   unsigned int getSelectSocketDescriptor();
   
   unsigned int getSocketDescriptor();  // returns the real socket descriptor - used to correlate callbacks

   /// Turn Send Methods
   /// WARNING - if using Secure media, then there must be room at the 
   ///           end of the passed in buffer for the SRTP HMAC code to be appended
   ///           ***It would be good to make this safer***
   void send(char* buffer, unsigned int size);
   void sendTo(const asio::ip::address& address, unsigned short port, char* buffer, unsigned int size);
   void rawSendTo(const asio::ip::address& address, unsigned short port, const char* buffer, unsigned int size);

   /// Receive Methods
   asio::error_code receive(char* buffer, unsigned int& size, unsigned int timeout, asio::ip::address* sourceAddress=0, unsigned short* sourcePort=0);
   asio::error_code receiveFrom(const asio::ip::address& address, unsigned short port, char* buffer, unsigned int& size, unsigned int timeout);

   void setActiveDestination(const char* address, unsigned short port);
   void startDtlsClient(const char* address, unsigned short port);
   void setRemoteSDPFingerprint(const resip::Data& fingerprint);
   const resip::Data getRemoteSDPFingerprint();

   const StunTuple& getLocalTuple();
   StunTuple getSessionTuple();  // returns either local, reflexive, or relay tuple depending on NatTraversalMode
   StunTuple getRelayTuple();
   StunTuple getReflexiveTuple();
   unsigned int getComponentId() { return mComponentId; }


private:
   asio::io_service& mIOService;
   asio::ssl::context& mSslContext;

   // Note: these member variables are set at creation time and never changed, thus
   //       they do not require mutex protection
   unsigned int mComponentId;
   StunTuple mLocalBinding;

   // MediaStream that this Flow belongs too
   MediaStream& mMediaStream;

   // mTurnSocket has it's own threading protection
   boost::shared_ptr<TurnAsyncSocket> mTurnSocket;

   // These are only set once, then accessed - thus they do not require mutex protection
   UInt8 mAllocationPortProps;
   StunTuple mRequestedAllocationTuple; 

   // Mutex to protect the following members that may be get/set from multiple threads
   resip::Mutex mMutex;
   StunTuple mReflexiveTuple;
   StunTuple mRelayTuple;
   resip::Data mRemoteSDPFingerprint;

   // Map to store all DtlsSockets - in forking cases there can be more than one
   std::map<reTurn::StunTuple, dtls::DtlsSocket*> mDtlsSockets;
   dtls::DtlsSocket* getDtlsSocket(const reTurn::StunTuple& endpoint);
   dtls::DtlsSocket* createDtlsSocketClient(const StunTuple& endpoint);
   dtls::DtlsSocket* createDtlsSocketServer(const StunTuple& endpoint);

   volatile FlowState mFlowState;
   void changeFlowState(FlowState newState);
   char* flowStateToString(FlowState state);

   class ReceivedData
   {
   public:
      ReceivedData(const asio::ip::address& address, unsigned short port, boost::shared_ptr<DataBuffer>& data) :
         mAddress(address), mPort(port), mData(data) {}
      ~ReceivedData() {}

      asio::ip::address mAddress;
      unsigned short mPort;
      boost::shared_ptr<DataBuffer> mData;
   };
   // FIFO for received data
   typedef resip::TimeLimitFifo<ReceivedData> ReceivedDataFifo;
   ReceivedDataFifo mReceivedDataFifo; 

   // Helpers to perform SRTP protection/unprotection
   bool processSendData(char* buffer, unsigned int& size, const asio::ip::address& address, unsigned short port);
   asio::error_code processReceivedData(char* buffer, unsigned int& size, ReceivedData* receivedData, asio::ip::address* sourceAddress=0, unsigned short* sourcePort=0);

   FakeSelectSocketDescriptor mFakeSelectSocketDescriptor;

   virtual void onConnectSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port);
   virtual void onConnectFailure(unsigned int socketDesc, const asio::error_code& e);

   virtual void onSharedSecretSuccess(unsigned int socketDesc, const char* username, unsigned int usernameSize, const char* password, unsigned int passwordSize);
   virtual void onSharedSecretFailure(unsigned int socketDesc, const asio::error_code& e);

   virtual void onBindSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple);
   virtual void onBindFailure(unsigned int socketDesc, const asio::error_code& e);

   virtual void onAllocationSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple, const StunTuple& relayTuple, unsigned int lifetime, unsigned int bandwidth);
   virtual void onAllocationFailure(unsigned int socketDesc, const asio::error_code& e);

   virtual void onRefreshSuccess(unsigned int socketDesc, unsigned int lifetime);
   virtual void onRefreshFailure(unsigned int socketDesc, const asio::error_code& e);

   virtual void onSetActiveDestinationSuccess(unsigned int socketDesc);
   virtual void onSetActiveDestinationFailure(unsigned int socketDesc, const asio::error_code &e);
   virtual void onClearActiveDestinationSuccess(unsigned int socketDesc);
   virtual void onClearActiveDestinationFailure(unsigned int socketDesc, const asio::error_code &e);

   //virtual void onReceiveSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port, const char* buffer, unsigned int size);
   virtual void onReceiveSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port, boost::shared_ptr<DataBuffer>& data);
   virtual void onReceiveFailure(unsigned int socketDesc, const asio::error_code& e);

   virtual void onSendSuccess(unsigned int socketDesc);
   virtual void onSendFailure(unsigned int socketDesc, const asio::error_code& e);
};

}

#endif


/* ====================================================================

 Original contribution Copyright (C) 2008 Plantronics, Inc.
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
